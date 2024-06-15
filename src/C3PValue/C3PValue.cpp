/*
File:   C3PValue.cpp
Author: J. Ian Lindsay
Date:   2023.07.29

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "C3PValue.h"
#include "KeyValuePair.h"
#include "../StringBuilder.h"
#include "../TimerTools/TimerTools.h"
#include "../Identity/Identity.h"
#include "../AbstractPlatform.h"   // Only needed for logging.

/* CBOR support should probably be required to parse/pack. */
#if defined(CONFIG_C3P_CBOR)
  #include "../cbor-cpp/cbor.h"
#endif

/* Image support costs code size. Don't support it unless requested. */
#if defined(CONFIG_C3P_IMG_SUPPORT)
  #include "../Image/Image.h"
#endif

const char* const LOCAL_LOG_TAG = "C3PValue";

/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

/**
* We are being asked to inflate a C3PValue object from an unknown string of a
*   known format.
*
* @param input is the buffer to parse. It will be consumed on successful parsing.
* @param FORMAT is the encoding format by which input will be parsed.
* @return A heap-allocated C3PValue object on success, or nullptr otherwise.
*/
C3PValue* C3PValue::deserialize(StringBuilder* input, const TCode FORMAT) {
  C3PValue* ret = nullptr;
  const uint32_t INPUT_LEN = input->length();
  switch (FORMAT) {
    case TCode::BINARY:
      if (INPUT_LEN > 1) {  // Need at least two bytes to be possibly valid.
        const TCode TC = (TCode) input->byteAt(0);
        C3PType* t_helper = getTypeHelper(TC);
        if (nullptr != t_helper) {
          if (t_helper->is_fixed_length()) {
            if ((INPUT_LEN-1) >= (uint32_t) input->length()) {
            }
            else {}  // We don't have enough bytes to parse this type.
          }
          else {
            // Variable-length types require that we try to parse them according
            //   to their type helper, and observe the outcome.
            //t_helper->is_punned_ptr()
            //ret = t_helper->deserialize(input, FORMAT);
          }
        }
      }
      break;

    #if defined(__BUILD_HAS_CBOR)
    case TCode::CBOR:
      {
        C3PValueDecoder decoder(input);
        ret = decoder.next();
      }
      break;
    #endif  // __BUILD_HAS_CBOR

    default:  break;
  }
  return ret;
}



/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* Protected delegate constructor.
* If _val_by_ref is false, it means there is no allocation, since the data can
*   be copied by value into the pointer space. This will also be true for types
*   that are passed as pointers anyway (Image*, Identity*, etc).
* If _val_by_ref is true, it means that _target_mem is a pointer to whatever
*   type is declared.
*/
C3PValue::C3PValue(const TCode TC, void* ptr, uint8_t mem_flgs)
  : _TCODE(TC), _mem_flgs(mem_flgs), _set_trace(1), _next(nullptr), _target_mem(ptr)
{
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    _set_flags(t_helper->is_punned_ptr(), C3PVAL_MEM_FLAG_PUNNED_PTR);
    _set_flags(t_helper->is_ptr(), C3PVAL_MEM_FLAG_VALUE_BY_REF);
    if (!t_helper->is_punned_ptr() & t_helper->value_by_copy()) {
      // Every value-by-copy type that does not fit inside a void* on the target
      //   platform will be heap-allocated and zeroed on construction. C3PType
      //   is responsible for having homogenized this handling, and all we need
      //   to do is allocate the memory if the numeric type could not be
      //   type-punned.
      // This will cover such cases as DOUBLE and INT64 on 32-bit builds.
      //
      // TODO: This is a constructor, and we thus have no clean way of
      //   handling heap-allocation failures. So we'll need a lazy-allocate
      //   arrangement eventually, in which case, this block will be redundant.
      // Default value will be nonsense. Clobber it. NOTE: This means that all
      //   value-by-copy types must be fixed-length.
      const uint32_t TYPE_STORAGE_SIZE = t_helper->length(nullptr);
      if (0 < TYPE_STORAGE_SIZE) {
        _target_mem = malloc(TYPE_STORAGE_SIZE);
        if (nullptr != _target_mem) {
          reapValue(true);  // We will free this memory upon our own destruction.
          if (nullptr == ptr) {
            memset(_target_mem, 0, TYPE_STORAGE_SIZE);
          }
          else {
            // Should never fail, because no type conversion is being done,
            //   but there is no return assurance for the situation TODO'd above.
            // TODO: Until this is resolved, it will be burdensome for the
            //   caller to detect a memory fault, and whatever initialization
            //   value was passed in will not be recorded.
            if (0 > t_helper->set_from(_target_mem, _TCODE, ptr)) {
              _set_mem_fault();
            }
          }
        }
        else _set_mem_fault();
      }
      else _set_mem_fault();
    }
    else if (t_helper->is_ptr_len()) {
      // The compound pointer-length types (BINARY, CBOR, etc) will have an
      //   indirected shim object to consolidate their parameter space into a
      //   single reference. That shim will be heap allocated.
      _target_mem = malloc(sizeof(C3PBinBinder));
      if (nullptr != _target_mem) {
        //((C3PBinBinder*) _target_mem)->tcode = TC;
        _set_flags(true, C3PVAL_MEM_FLAG_VALUE_BY_REF);
        // TODO: Is this the correct choice? We know that we are responsible for
        //   freeing the C3PBinBinder we just created, and I don't want to mix
        //   semantic layers, nor do I want to extend flags into the shim (thus
        //   indirecting them).
        _set_flags(false, C3PVAL_MEM_FLAG_REAP_VALUE);
      }
      else _set_mem_fault();
    }
    else {
      // Anything else just sets _target_mem directly.
    }
  }
  else _set_mem_fault();
}


/**
* Destructor. This is touchy, and will be the source of grief for someone. So
*   here are the rules....
* This object is being destroyed either because delete was called upon it, or
*   it was created on the stack and has gone out of scope. In any case, we don't
*   worry about the sizeof(C3PValue) bytes of RAM this object occupies. Only the
*   things that are (attached to) and (owned by) this object.
*
* The destructor will recursively traverse the linked-list of other data and
*   destroy everything in the ownership chain from the leaves upward.
* Then, the _target_mem will be free'd or deleted, as/if required.
* Then, the destructor will return.
*/
C3PValue::~C3PValue() {
  if (has_key()) {
    // The derived KeyValuePair class doesn't actually have a destructor
    //   that does anything. It always owns its own pointer and reap flags,
    //   which it is responsible for manipulating. So we can just set the
    //   key to some (const char*), and the class will free any existing
    //   string that it might be holding.
    ((KeyValuePair*) this)->setKey("");
  }

  // Wipe out any chain of siblings.
  if (nullptr != _next) {
    C3PValue* a = _next;       // Concurrency measures being taken here. Null
    _next       = nullptr;     //   the accessible ref prior to deleting it.
    if (a->reapContainer()) {  // If we SHOULD delete it, that is...
      delete a;
    }
  }

  // Finally, Turn our attention to the data we ourselves hold.
  // TODO: c3ptype_alloc(uint32_t free_arg) and c3ptype_free(void*) should be
  //   added to C3PType.
  if (nullptr != _target_mem) {
    if (is_ptr_len()) {
      // Some types have a shim to hold an explicit length, or other data. We
      //   construe reapValue() to refer to the data itself, and not the shim.
      //   We will always free the shim.
      if (reapValue()) {
        free(((C3PBinBinder*) _target_mem)->buf);  // We might be responsible for this.
      }
      free(_target_mem);  // We are always responsible for this.
    }
    else if (!_is_val_by_ref()) {
      if (!_is_ptr_punned()) {
        // In cases where we allocated the memory by this class for the sake of
        //   facilitating value-by-copy, we will free it, regardless of what
        //   reapValue() has to say about it.
        free(_target_mem);
      }
    }
    else if (reapValue()) {
      switch (_TCODE) {
        case TCode::BINARY:
        case TCode::CBOR:
          // TODO: These cases should all be ptr-len. Verify. But we don't want
          //   to be in the default case.
          break;
        case TCode::STR_BUILDER:  delete ((StringBuilder*) _target_mem);  break;
        case TCode::KVP:          delete ((KeyValuePair*)  _target_mem);  break;
        case TCode::STOPWATCH:    delete ((StopWatch*)     _target_mem);  break;
      #if defined(CONFIG_C3P_IDENTITY_SUPPORT)
        case TCode::IDENTITY:     delete ((Identity*)      _target_mem);  break;
      #endif
      #if defined(CONFIG_C3P_IMG_SUPPORT)
        case TCode::IMAGE:        delete ((Image*)         _target_mem);  break;
      #endif
        // No destructor semantics. Just free.
        // TODO: This is dangerous. clarify the scope of this behavior...
        default:    free(_target_mem);  break;
      }
    }
  }
  _target_mem = nullptr;
}


/*******************************************************************************
* Typed constructors that have nuanced handling, and can't be inlined.
*******************************************************************************/

// TODO: This is no longer necessary.
C3PValue::C3PValue(float val) : C3PValue(TCode::FLOAT, nullptr) {
  uint8_t* src = (uint8_t*) &val;                  // To avoid inducing bugs
  *(((uint8_t*) &_target_mem) + 0) = *(src + 0);   //   related to alignment,
  *(((uint8_t*) &_target_mem) + 1) = *(src + 1);   //   we copy the value
  *(((uint8_t*) &_target_mem) + 2) = *(src + 2);   //   byte-wise into our own
  *(((uint8_t*) &_target_mem) + 3) = *(src + 3);   //   storage.
}

/*
* Construction from char* has semantics that imply the source buffer is
*   ephemeral, and ought to be copied into a memory region allocated and
*   owned by this class.
*/
C3PValue::C3PValue(char* val) : C3PValue(TCode::STR, nullptr) {
  if (nullptr != val) {
    const uint32_t VAL_LENGTH = strlen(val);
    _target_mem = malloc(VAL_LENGTH + 1);
    if (nullptr != _target_mem) {
      memcpy(_target_mem, val, VAL_LENGTH);
      *((char*)_target_mem + VAL_LENGTH) = 0;
      reapValue(true);
    }
    else {  _set_mem_fault();  }
  }
}


/*
* Construction from ptr-len will never deep-copy. So if that is desirable, the caller
*   should pass in a pointer it has ownership of and optionally call
*   reapValue(true) to relinquishing ownership to this class.
*/
C3PValue::C3PValue(uint8_t* val, uint32_t len) : C3PValue(TCode::BINARY, nullptr) {
  // The main constructor will have handled allocating the C3PBinBinder struct,
  //   which we will always be responsible for.
  if (nullptr != _target_mem) {
    ((C3PBinBinder*) _target_mem)->buf   = (uint8_t*) val;
    ((C3PBinBinder*) _target_mem)->len   = len;
  }
}



/*******************************************************************************
* Type glue
*******************************************************************************/
bool C3PValue::is_ptr_len() {
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    return (t_helper->is_ptr_len());
  }
  return false;
}


// TODO: Until the rules are settled, this function translates _target_mem into
//   the expected reference for the given type. This function should ultimately
//   reduce into simple flag comparisons.
void* C3PValue::_type_pun_get() {
  if (_chk_flags(C3PVAL_MEM_FLAG_PUNNED_PTR) & !_chk_flags(C3PVAL_MEM_FLAG_VALUE_BY_REF)) {
    return &_target_mem;
  }
  else {
    return _target_mem;
  }
}

// TODO: Until the rules are settled, this function translates _target_mem into
//   the expected reference for the given type. This function should ultimately
//   reduce into simple flag comparisons.
void* C3PValue::_type_pun_set() {
  switch (_TCODE) {
    case TCode::VECT_3_INT8:
    case TCode::VECT_3_INT16:
    case TCode::VECT_3_INT32:
    case TCode::VECT_3_UINT8:
    case TCode::VECT_3_UINT16:
    case TCode::VECT_3_UINT32:
    case TCode::VECT_3_FLOAT:
    case TCode::VECT_3_DOUBLE:
      return _target_mem;
    default:  break;
  }

  if (_chk_flags(C3PVAL_MEM_FLAG_PUNNED_PTR)) {
    // Any data that fits into the pointer itself will want a pointer to
    //   _target_mem as its dest argument to a set() fxn.
    return &_target_mem;
  }
  else if (!_is_val_by_ref()) {
    // Value-by-copy data that didn't fit inside void* will have been indirected
    //   into a separate allocation. Return that pointer.
    return _target_mem;
  }
  else {
    return &_target_mem;
  }
}



/*******************************************************************************
* Basal accessors
* These functions ultimately wrap the C3PTypeConstraint template that handles
*   the type conversion matrix.
*******************************************************************************/
int8_t C3PValue::set_from(const TCode SRC_TYPE, void* src) {
  int8_t ret = -1;
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    ret = t_helper->set_from(_type_pun_set(), SRC_TYPE, src);
  }
  if (0 == ret) {
    _set_trace++;
  }
  return ret;
}


int8_t C3PValue::set(C3PValue* src) {
  return ((nullptr == src) ? -2 : set_from(src->tcode(), src->_type_pun_get()));
}

int8_t C3PValue::get_as(const TCode DEST_TYPE, void* dest) {
  int8_t ret = -1;
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    ret = t_helper->get_as(_type_pun_get(), DEST_TYPE, dest);
  }
  return ret;
}


// TODO: Audit. Might be wrong-headed.
int8_t C3PValue::get_as(uint8_t** v, uint32_t* l) {
  int8_t ret = -1;
  if ((nullptr != v) & (nullptr != l)) {
    ret--;
    if (nullptr != _target_mem) {
      *v = ((C3PBinBinder*) _target_mem)->buf;
      *l = ((C3PBinBinder*) _target_mem)->len;
      ret = 0;
    }
  }
  return ret;
}

// TODO: Audit. Might be wrong-headed.
int8_t C3PValue::set(uint8_t* src, uint32_t l, const TCode SRC_TYPE) {
  int8_t ret = -1;
  if (nullptr != _target_mem) {
    ((C3PBinBinder*) _target_mem)->buf = src;
    ((C3PBinBinder*) _target_mem)->len = l;
    _set_trace++;
    ret = 0;
  }
  return ret;
}


/*
* This optional accessor will allow the caller to discover if the value has
*   changed since its last check, and to update the check value if so.
*/
bool C3PValue::dirty(uint16_t* x) {
  bool ret = ((nullptr != x) && (*x != _set_trace));
  if (ret) {  *x = _set_trace;  }
  return ret;
}



/*******************************************************************************
* Type coercion functions.
* These functions will do their best to get or set the value after the implied
*   translation.
* NOTE: A call to get() that results in a loss of precision or truncation should
*   return the failure value. This may be construed as a true value by the
*   caller, and represents an API gap. The caller should check types, or use a
*   deliberately too-large type if there is doubt about a specific value's size.
* NOTE: A similar discipline applies to set(). If a truncation or LoP would
*   result in a differnt value than intended, set() should change nothing and
*   return -1.
*******************************************************************************/
unsigned int C3PValue::get_as_uint(int8_t* success) {
  uint32_t ret = 0;
  int8_t suc = get_as(TCode::UINT32, (void*) &ret) + 1;
  if (success) {  *success = suc;  }
  return ret;
}

int C3PValue::get_as_int(int8_t* success) {
  int32_t ret = 0;
  int8_t suc = get_as(TCode::INT32, (void*) &ret) + 1;
  if (success) {  *success = suc;  }
  return ret;
}

uint64_t C3PValue::get_as_uint64(int8_t* success) {
  uint64_t ret = 0;
  int8_t suc = get_as(TCode::UINT64, (void*) &ret) + 1;
  if (success) {  *success = suc;  }
  return ret;
}

int64_t C3PValue::get_as_int64(int8_t* success) {
  int64_t ret = 0;
  int8_t suc = get_as(TCode::INT64, (void*) &ret) + 1;
  if (success) {  *success = suc;  }
  return ret;
}

/* Casts the value into (!/= 0). */
bool C3PValue::get_as_bool(int8_t* success) {
  if (success) {  *success = true;  }
  return (0 != _target_mem);
}


float C3PValue::get_as_float(int8_t* success) {
  float ret = 0.0f;
  int8_t suc = get_as(TCode::FLOAT, (void*) &ret) + 1;
  if (success) {  *success = suc;  }
  return ret;
}


double C3PValue::get_as_double(int8_t* success) {
  double ret = 0.0d;
  int8_t suc = get_as(TCode::DOUBLE, (void*) &ret) + 1;
  if (success) {  *success = suc;  }
  return ret;
}

C3PBinBinder C3PValue::get_as_ptr_len(int8_t* success) {
  C3PBinBinder ret;
  int8_t suc = 1;
  if (is_ptr_len()) {
    suc = get_as(TCode::BINARY, (void*) &ret) + 1;
  }
  else {
    // TODO: This ought to properly coerce any not ptr/len into such a
    //   representation. But it is untested.
    ret.buf = (uint8_t*) _type_pun_get();
    ret.len = length();
  }

  if (success) {  *success = suc;  }
  return ret;
}

KeyValuePair* C3PValue::get_as_kvp(int8_t* success) {
  KeyValuePair* ret = nullptr;
  int8_t suc = get_as(TCode::KVP, (void*) &ret) + 1;
  if (success) {  *success = suc;  }
  return ret;
}


/*******************************************************************************
* Accessors for linkage to parallel data.
*******************************************************************************/

/**
* @return [description]
*/
C3PValue* C3PValue::valueWithIdx(uint32_t idx) {
  switch (idx) {
    case 0:
      return this;   // Terminus of recursion for valid parameters.
    default:
      if (nullptr != _next) {
        return _next->valueWithIdx(idx-1);
      }
      return nullptr;  // Terminus of recursion for parameters greater than the list's cardinality.
  }
}


/**
*
* @param  idx      The KeyValuePair position
* @param  trg_buf  A pointer to the place where we should write the result.
* @return 0 on success or appropriate failure code.
*/
int8_t C3PValue::valueWithIdx(uint32_t idx, void* trg_buf) {
  int8_t ret = -1;
  C3PValue* val_container = valueWithIdx(idx);
  if (nullptr != val_container) {
    ret = val_container->get_as(val_container->tcode(), trg_buf);
  }
  return ret;
}


/* Return the next sibling object that is a KVP. */
KeyValuePair* C3PValue::_next_sib_with_key() {
  C3PValue* tmp_val = _next;
  while (nullptr != tmp_val) {
    // Skip anything that doesn't have a key.
    if (tmp_val->has_key()) {
      return (KeyValuePair*) tmp_val;  // Bimodal type hack.
    }
    tmp_val = tmp_val->_next;
  }
  return nullptr;
}



/**
* @param linked_val is the value to link as a sibling.
*/
C3PValue* C3PValue::link(C3PValue* linked_val, bool reap_cont) {
  if (nullptr == _next) {
    if (nullptr != linked_val) {
      linked_val->reapContainer(reap_cont);
    }
    _next = linked_val;
  }
  else {
    _next->link(linked_val, reap_cont);
  }
  return linked_val;
}

/**
* @return The number of sibling values.
*/
uint32_t C3PValue::count() {
  return (1 + ((nullptr == _next) ? 0 : _next->count()));
}


/**
* Given an C3PValue pointer, finds that pointer and drops it from the list.
* TODO: There is a potential for a memory leak here. Need to explicitly call
*   destructors if reapContainer() is true.
*
* @param  drop  The C3PValue to drop.
* @return       0 on success. 1 on warning, -1 on "not found".
*/
int8_t C3PValue::drop(C3PValue** prior, C3PValue* drop, bool destruct) {
  if (this == drop) {
    // Re-write the prior parameter.
    *prior = _next;
    _next = nullptr;
    if (destruct & reapContainer()) {
      delete this;
    }
    return 0;
  }
  return (_next) ? _next->drop(&_next, drop) : -1;
}


/**
* Return the RAM use of this KVP.
* By passing true to deep, the return value will also factor in concealed heap
*   overhead of the containers themselves.
* Return value accounts for padding due to alignment constraints.
* TODO: Does not account for keys.
*
* @param deep will also factor in heap overhead of the containers.
* @return 0 on success, or negative on failure.
*/
int C3PValue::memoryCost(bool deep) {
  // TODO: sizeof(intptr_t) for OVERHEAD_PER_MALLOC is an assumption based on a
  //   specific build of newlib. Find a way to discover it from the build.
  const uint32_t OVERHEAD_PER_CLASS  = (has_key() ? sizeof(KeyValuePair) : sizeof(C3PValue));
  const uint32_t OVERHEAD_EFFECTIVE  = (deep ? OVERHEAD_PER_CLASS : 0);
  const uint32_t OVERHEAD_PER_MALLOC = (deep ? sizeof(intptr_t) : 0);

  int32_t ret = OVERHEAD_PER_CLASS;
  ret += OVERHEAD_PER_MALLOC;
  ret += length();
  if (nullptr != _next) {
    ret += _next->memoryCost(deep);
  }
  return ret;
}



/*******************************************************************************
* Parsing/Packing
*******************************************************************************/
/*
* This is the choke point for all serialization calls on data held by C3P's type
*   system.
*/
int8_t C3PValue::serialize(StringBuilder* output, const TCode FORMAT) {
  int8_t ret = -1;
  // Use an intermediary StringBuilder so we can collapse the strings a
  //   bit more neatly.
  StringBuilder local_out_buffer;

  // This object might have a number of attributes that will impact how it is
  //   serialized.
  // 1. It might have a key. In which case, let KVP do the work.
  if (has_key()) {
    ret = ((KeyValuePair*) this)->serialize(output, FORMAT);
  }
  else {
    C3PType* t_helper = getTypeHelper(_TCODE);
    if (nullptr != t_helper) {
      ret = t_helper->serialize(_type_pun_get(), output, FORMAT);
    }
  }

  // 2. It might be a compound value. This distinction is meaningless to the
  //      memory layout, but is the difference between it being communicated
  //      as a sequence of unrelated values, or a related group.
  if (isCompound()) {

  }

  if (!local_out_buffer.isEmpty()) {
    local_out_buffer.string();  // Consolidate the heap.
    output->concatHandoff(&local_out_buffer);
  }
  return ret;
}


/*
*/
uint32_t C3PValue::length() {
  uint32_t ret = 0;
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    // NOTE: This works reliably because any types copied into _target_mem that
    //   are NOT pointers will also be fixed length.
    ret = t_helper->length(_target_mem);
  }
  return ret;
}



/**
* This function prints the value to the provided buffer.
*
* @param out is the buffer to receive the printed value.
*/
void C3PValue::toString(StringBuilder* out, bool include_type) {
  if (include_type) {
    out->concatf("(%s) ", typecodeToStr(_TCODE));
  }
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    t_helper->to_string(_type_pun_get(), out);
    //out->concatf("%08x", _type_pun_get());
  }
  else {
    const uint32_t L_ENDER = length();
    for (uint32_t n = 0; n < L_ENDER; n++) {
      out->concatf("%02x ", *((uint8_t*) _target_mem + n));
    }
  }
}



/*
* Warning: call is propagated across entire list.
*/
void C3PValue::printDebug(StringBuilder* out) {
  StringBuilder tmp;
  tmp.concatf("\t%10s %5s %5s %5s\t",
    (has_key() ?((KeyValuePair*) this)->getKey() : ""),
    "",   //(_reap_key()     ? "(key)" : ""),
    (reapContainer() ? "(con)" : ""),
    (reapValue()     ? "(val)" : "")
  );
  toString(&tmp, true);
  tmp.concat('\n');
  tmp.string();
  out->concatHandoff(&tmp);
  if (nullptr !=  _next)  _next->printDebug(out);
}





/*******************************************************************************
* C3PValueDecoder
*
* TODO: This class should be purpose-merged with its sibling class
*   CBORArgListener, which does the same thing for KeyValuePair.
*   This will eventually lower the global complexity of making a tighter
*   integration between C3PValue and KeyValuePair.
*   Some of this might be promoted to C3PType during that effort.
*******************************************************************************/

bool C3PValueDecoder::_get_length_field(uint32_t* offset_ptr, uint64_t* val_ret, uint8_t minorType) {
  if (minorType < 24) {
    *val_ret = minorType;
    return true;
  }
  uint64_t value = 0;
  uint32_t offset = *offset_ptr;
  uint8_t buf[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t len_len = (1 << (minorType - 24));
  switch (len_len) {
    case 1:
    case 2:
    case 4:
    case 8:
      if (len_len == _in->copyToBuffer(buf, len_len, offset)) {
        value |= ((uint64_t) buf[0] << 56);
        value |= ((uint64_t) buf[1] << 48);
        value |= ((uint64_t) buf[2] << 40);
        value |= ((uint64_t) buf[3] << 32);
        value |= ((uint64_t) buf[4] << 24);
        value |= ((uint64_t) buf[5] << 16);
        value |= ((uint64_t) buf[6] << 8);
        value |= ((uint64_t) buf[7]);
        *val_ret = (value >> ((8 - len_len) << 3));  // Shift the buffer into the right orientation.
        *offset_ptr = (offset + len_len);
        return true;
      }
      break;
    default:
      c3p_log(LOG_LEV_ERROR, LOCAL_LOG_TAG, "Unsupported length field length (%u)", len_len);
      break;
  }
  return false;
}


/*
* Calling this function will return the next complete C3PValue that can be
*   parsed and consumed from the input.
*
* @param consume_unparsable will cause this function to consume bytes that it cannot parse.
*/
C3PValue* C3PValueDecoder::next(bool consume_unparsable) {
  if (nullptr == _in) {  return nullptr;  }   // Bailout
  uint32_t length_taken = 0;
  C3PValue* value = _next(&length_taken);
  const bool SHOULD_CULL = ((nullptr != value) | consume_unparsable);
  if (SHOULD_CULL) {
    _in->cull(length_taken);
  }
  return value;
}



/*
* Private counterpart to next() that isolates our recursion concerns.
*/
C3PValue* C3PValueDecoder::_next(uint32_t* offset) {
  const uint32_t INPUT_LEN = _in->length();
  C3PValue* value = nullptr;
  uint32_t local_offset  = *offset;
  bool consume_without_value = false;

  if (INPUT_LEN > 0) {
    uint64_t _length_extra  = 0;   // Length of data specified by the length field above.
    const uint8_t C_TYPE    = _in->byteAt(local_offset++);   // The first byte
    const uint8_t MAJORTYPE = (uint8_t) (C_TYPE >> 5);
    const uint8_t MINORTYPE = (uint8_t) (C_TYPE & 31);

    // First pass. Sometimes, we can get a full parse from the first byte, but
    //   for multibyte types, we usually only get a secondary length field.
    switch (MINORTYPE) {
      case 24:  case 25:  case 26:  case 27:
        if (!_get_length_field(&local_offset, &_length_extra, MINORTYPE)) {
          // If we didn't have enough bytes to get the attached integer field,
          //   there is no hope, and for clarity's sake, we'll bail out here.
          return nullptr;
        }
        break;
      default:
        if (MINORTYPE < 24) {
          _length_extra = MINORTYPE;
        }
        else {
          c3p_log(LOG_LEV_WARN, LOCAL_LOG_TAG, "Invalid MAJ/MIN combination (0x%02x/0x%02x)", MAJORTYPE, MINORTYPE);
        }
        break;
    }
    const uint32_t _length_extra32 = (uint32_t) _length_extra;  // If truly a length, it will be 32-bit.

    // For types that observe _length_extra, they will generally also want to
    //   know if there are enough bytes to satisfy the parse.
    const bool HAVE_EXTRA_LEN = ((_length_extra > 0) & (INPUT_LEN >= (local_offset + _length_extra32)));

    // Second pass. Form a return object, if possible.
    switch (MAJORTYPE) {
      case 0:  // positive integer
        switch (MINORTYPE) {
          case 24:  value = new C3PValue((uint8_t)  _length_extra32);  break;
          case 25:  value = new C3PValue((uint16_t) _length_extra32);  break;
          case 26:  value = new C3PValue((uint32_t) _length_extra32);  break;
          case 27:  value = new C3PValue((uint64_t) _length_extra);    break;
          default:
            if (MINORTYPE < 24) {
              value = new C3PValue((uint8_t) MINORTYPE);
            }
            else c3p_log(LOG_LEV_WARN, LOCAL_LOG_TAG, "Invalid PINT type (0x%02x)", MINORTYPE);
            break;
        }
        break;

      case 1:   // negative integer
        switch (MINORTYPE) {
          case 24:
            if (_length_extra32 < INT8_MAX) {       value = new C3PValue((int8_t)  -(_length_extra32+1));  }
            else if(_length_extra32 == INT8_MAX) {  value = new C3PValue((int8_t) INT8_MIN);           }
            else {                                  value = new C3PValue((int16_t) -(_length_extra32+1));  }
            break;
          case 25:
            if (_length_extra32 < INT16_MAX) {       value = new C3PValue((int16_t) -(_length_extra32+1));  }
            else if(_length_extra32 == INT16_MAX) {  value = new C3PValue((int32_t) INT16_MIN);           }
            else {                                   value = new C3PValue((int32_t) -(_length_extra32+1));  }
            break;
          case 26:
            if (_length_extra < INT32_MAX) {       value = new C3PValue((int32_t) -(_length_extra+1));  }
            else if(_length_extra == INT32_MAX) {  value = new C3PValue((int32_t) INT32_MIN);           }
            else {                                 value = new C3PValue((int64_t) -(_length_extra+1));  }
            break;
          case 27:
            value = new C3PValue((int64_t) -(_length_extra+1));
            break;
          default:
            if (MINORTYPE < 24) {
              value = new C3PValue((int8_t) (0xFF - MINORTYPE));
            }
            else c3p_log(LOG_LEV_WARN, LOCAL_LOG_TAG, "Invalid NINT type (0x%02x)", MINORTYPE);
            break;
        }
        break;

      case 2:  // Raw bytes
        if (HAVE_EXTRA_LEN) {
          uint8_t* new_buf = (uint8_t*) malloc(_length_extra32);
          if (nullptr != new_buf) {
            if ((int32_t) _length_extra32 == _in->copyToBuffer(new_buf, _length_extra32, local_offset)) {
              value = new C3PValue(new_buf, _length_extra32);
              if (nullptr != value) {
                value->reapValue(true);
                local_offset += _length_extra32;
              }
            }
            if (nullptr == value) {
              free(new_buf);  // Clean up any malloc() mess.
            }
          }
        }
        break;

      case 3:  // String
        if (HAVE_EXTRA_LEN) {
          // If there are enough bytes to satisfy the parse...
          uint8_t new_buf[_length_extra32+1];
          *(new_buf + _length_extra32) = '\0';
          if ((int32_t) _length_extra32 == _in->copyToBuffer(new_buf, _length_extra32, local_offset)) {
            value = new C3PValue((char*) new_buf);
            if (nullptr != value) {
              local_offset += _length_extra32;
            }
          }
        }
        break;

      case 4:  value = _handle_array(&local_offset, _length_extra32);  break;
      case 5:
        // Only bother parsing further if we have enough bytes to satisfy the
        //   minimum length requirement for a map of the stated length, assuming
        //   two bytes per map entry. TODO: Are 0-length string allowed here?
        if (INPUT_LEN >= (local_offset + (_length_extra32 * 2))) {
          value = _handle_map(&local_offset, _length_extra32);
        }
        break;
      case 6:
        if (C3P_CBOR_VENDOR_CODE == (_length_extra32 & 0xFFFFFF00)) {
          // We noticed our vendor code, mask it off and see if see have a type
          //   helper for it.
          const TCode TC = IntToTcode(_length_extra32 & 0x000000FF);
          C3PType* t_helper = getTypeHelper(TC);
          if (nullptr != t_helper) {
            // We have a type that matches what was encoded. Try to deserialize.
            value = _handle_tag(&local_offset, t_helper);
          }
          else {
            c3p_log(LOG_LEV_WARN, LOCAL_LOG_TAG, "No C3PType for TCode (0x%02x)", (uint8_t) TC);
          }
        }
        break;

      case 7:  // special
        switch (MINORTYPE) {
          case 20:  value = new C3PValue(false);        break;
          case 21:  value = new C3PValue(true);         break;
          case 22:  value = new C3PValue(TCode::NONE);  break;  // CBOR "null"
          case 23:  value = new C3PValue(TCode::NONE);  break;  // CBOR "undefined"

          case 24:  case 25:
            if (HAVE_EXTRA_LEN) {
              consume_without_value = true;  //_listener->on_special(_length_extra);
            }
            break;
          case 26:  value = new C3PValue(*((float*)(void*) &_length_extra32));  break;
          case 27:  value = new C3PValue(*((double*)(void*) &_length_extra));   break;
          default:
            //if (MINORTYPE < 20) {
            //  _listener->on_special(MINORTYPE);
            //}
            consume_without_value = true;
            c3p_log(LOG_LEV_WARN, LOCAL_LOG_TAG, "Unhandled MINOR for special (0x%02x)", MINORTYPE);
            break;
        }
        break;
    }
  }

  if (consume_without_value | (nullptr != value)) {
    *offset = local_offset;
  }

  return value;
}



C3PValue* C3PValueDecoder::_handle_array(uint32_t* offset, uint32_t len) {
  C3PValue* value = nullptr;
  c3p_log(LOG_LEV_ERROR, LOCAL_LOG_TAG, "CBOR arrays aren't yet supported.");
  return value;
}


/*
* Extract the given number of string-key, and <?>-values.
*/
C3PValue* C3PValueDecoder::_handle_map(uint32_t* offset, uint32_t count) {
  const uint32_t INPUT_LEN = _in->length();
  KeyValuePair* ret = nullptr;
  uint32_t local_offset    = *offset;
  uint32_t len_key         = 0;
  bool bailout = false;

  while (!bailout & (count > 0)) {
    bailout = true;
    const uint8_t  CTYPE = _in->byteAt(local_offset++);
    const uint8_t  MAJOR = (CTYPE >> 5);
    const uint8_t  MINOR = (CTYPE & 31);
    if (3 == MAJOR) {  // The next byte must be a proper CBOR string.
      uint64_t len_key64 = 0;
      if (_get_length_field(&local_offset, &len_key64, MINOR)) {
        len_key = (uint32_t) len_key64;
        if (INPUT_LEN >= (local_offset + len_key + 1)) {  // +1 for the value byte.
          uint8_t buf_key[len_key+1];
          *(buf_key + len_key) = '\0';
          if ((int32_t) len_key == _in->copyToBuffer(buf_key, len_key, local_offset)) {
            local_offset += len_key;
            // Now the dangerous part. Recurse into next(), and parse out the
            //  next C3PValue.
            C3PValue* value = _next(&local_offset);
            if (nullptr != value) {
              // If it came back non-null, then it means the buffer was consumed
              //   up-to the point where the object became fully-defined.
              KeyValuePair* tmp_kvp = nullptr;
              if (!value->memError()) {
                // TODO: Implement isCompound() instead of this mess.
                if (value->has_key()) {
                  // We have to reallocate the container to allow for a key.
                  tmp_kvp = new KeyValuePair((char*) buf_key, (KeyValuePair*) value);
                  if (tmp_kvp) {
                    tmp_kvp->reapContainer(true);
                    tmp_kvp->reapValue(true);
                    value = nullptr;  // We claim the object.
                  }
                }
                else {
                  tmp_kvp = new KeyValuePair(
                    value->tcode(), (char*) buf_key,
                    (C3PVAL_MEM_FLAG_REAP_VALUE | C3PVAL_MEM_FLAG_REAP_KEY | C3PVAL_MEM_FLAG_REAP_CNTNR)
                  );
                  if (tmp_kvp) {
                    const int8_t SET_RET = tmp_kvp->set(value);
                    if (0 == SET_RET) {
                      // We might have just taken something with its own
                      //   life-cycle from the parser. We don't want it to be
                      //   free'd when its container is reaped.
                      value->reapValue(false);
                    }
                  }
                }
              }

              if (nullptr != tmp_kvp) {
                if (nullptr == ret) {
                  ret = tmp_kvp;
                }
                else {
                  ret->link(tmp_kvp, true);
                }
                bailout = false;
              }

              if (nullptr != value) {
                delete value;
              }
            }
          }
        }
      }
    }
    count--;
    //c3p_log(LOG_LEV_DEBUG, LOCAL_LOG_TAG, "Bailout: %c with %u left to go.\n", (bailout ? 'y':'n'), count);
  }

  if (bailout) {
    if (nullptr != ret) {  delete ret;  }
    ret = nullptr;
  }
  else {
    if (nullptr != ret) {
      *offset = local_offset;    // Indicate success by updating the offset.
    }
    else {
      c3p_log(LOG_LEV_DEBUG, LOCAL_LOG_TAG, "_handle_map(%u, %u) didn't bail out, but also didn't return a KVP.", *offset, count);
    }
  }
  return (C3PValue*) ret;
}


C3PValue* C3PValueDecoder::_handle_tag(uint32_t* offset, C3PType* t_helper) {
  const uint32_t INPUT_LEN = _in->length();
  uint32_t local_offset    = *offset;
  uint32_t len_key         = 0;
  C3PValue* ret            = nullptr;

  if ((INPUT_LEN - local_offset) > 1) {
    const uint8_t  CTYPE = _in->byteAt(local_offset);  // The next byte must be CBOR map.
    const uint8_t  MAJOR = (CTYPE >> 5);
    if (5 == MAJOR) {  // If so, invest the time reading it.
      C3PValue* kvp_val = _next(&local_offset);
      if (nullptr != kvp_val) {
        if (kvp_val->has_key()) {
          // We got a KVP.
          KeyValuePair* kvp = (KeyValuePair*) kvp_val;
          void* obj_ret = nullptr;
          if (0 == t_helper->construct(&obj_ret, (KeyValuePair*) kvp_val)) {
            // Any C3PValue objects created by this function will be heap-resident, and
            //   should be marked as such.
            const uint8_t MEM_FLGS = (C3PVAL_MEM_FLAG_REAP_CNTNR | C3PVAL_MEM_FLAG_REAP_VALUE);
            ret = new C3PValue(t_helper->TCODE, obj_ret, MEM_FLGS);
          }
          else {
            c3p_log(LOG_LEV_WARN, LOCAL_LOG_TAG, "KVP construction failed for type (%s).", t_helper->NAME);
          }
        }
        else {
          c3p_log(LOG_LEV_WARN, LOCAL_LOG_TAG, "Unhandled encoding for %s (%s).", t_helper->NAME, typecodeToStr(kvp_val->tcode()));
        }

        if (nullptr != kvp_val) {
          delete kvp_val;
        }
      }
      // Not enough bytes of input, presumably...
    }
    else {
      c3p_log(LOG_LEV_WARN, LOCAL_LOG_TAG, "Unhandled encoding for %s (0x%02).", t_helper->NAME, CTYPE);
    }
  }

  if (nullptr != ret) {
    *offset = local_offset;  // Indicate success by updating the offset.
  }
  return ret;
}
