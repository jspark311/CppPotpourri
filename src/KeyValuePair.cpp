/*
File:   KeyValuePair.cpp
Author: J. Ian Lindsay
Date:   2014.03.10

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



#include "KeyValuePair.h"

#include <PriorityQueue.h>
#if defined(CONFIG_C3P_IMG_SUPPORT)
  #include <Image/Image.h>
#endif   // CONFIG_C3P_IMG_SUPPORT

#include <Identity/Identity.h>

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
*/
KeyValuePair::KeyValuePair(void* ptr, int l, const TCode TC, uint8_t f) : _target_mem(ptr), _len(l), _flags(f), _t_code(TC) {
  _alter_flags(typeIsPointerPunned(TC), MANUVR_KVP_FLAG_DIRECT_VALUE);
  // If we can know the length with certainty, record it.
  if (typeIsFixedLength(TC)) {
    _len = sizeOfType(TC);
  }
}


/**
* Protected delegate constructor.
*/
KeyValuePair::KeyValuePair(void* ptr, int l, const TCode TC, const char* k, uint8_t f) : KeyValuePair(ptr, l, TC, f) {
  setKey(k);
}


/**
* Protected delegate constructor.
*/
KeyValuePair::KeyValuePair(void* ptr, int l, const TCode TC, char* k, uint8_t f) : KeyValuePair(ptr, l, TC, f) {
  setKey(k);
}


/**
* Destructor. Frees memory associated with this KeyValuePair.
* Recursively calls the destructor of a referenced KeyValuePair, if present.
*/
KeyValuePair::~KeyValuePair() {
  if (nullptr != _next) {
    KeyValuePair* a = _next;
    _next       = nullptr;
    delete a;
  }
  _set_new_key(nullptr);
  _set_new_value(nullptr);
  _t_code    = TCode::NONE;
  _len       = 0;
  _flags     = 0;
}


/*******************************************************************************
* Typed constructors and value accessor functions.
*******************************************************************************/

KeyValuePair::KeyValuePair(float val, const char* key) : KeyValuePair(nullptr, sizeof(float), TCode::FLOAT, key) {
  uint8_t* src = (uint8_t*) &val;
  *(((uint8_t*) &_target_mem) + 0) = *(src + 0);
  *(((uint8_t*) &_target_mem) + 1) = *(src + 1);
  *(((uint8_t*) &_target_mem) + 2) = *(src + 2);
  *(((uint8_t*) &_target_mem) + 3) = *(src + 3);
}


// TODO: We might be able to treat this as a direct value on a 64-bit system.
KeyValuePair::KeyValuePair(double val, const char* key) : KeyValuePair(malloc(sizeof(double)), sizeof(double), TCode::DOUBLE, key, (MANUVR_KVP_FLAG_REAP_VALUE)) {
  if (nullptr != _target_mem) {
    *((double*) _target_mem) = val;
  }
  else {
    _alter_flags(false, MANUVR_KVP_FLAG_REAP_VALUE);
    _alter_flags(true,  MANUVR_KVP_FLAG_ERR_MEM);
  }
}



/*******************************************************************************
* Accessors for key.
*******************************************************************************/

/*
* Take a key allocated elsewhere, and decline responsibility for it.
*/
void KeyValuePair::setKey(const char* k) {
  _set_new_key((char*) k);
  _reap_key(false);
}

/*
* Take a key allocated elsewhere, and takes responsibility for it.
*/
void KeyValuePair::setKey(char* k) {
  if (nullptr != k) {
    const int K_LEN = strlen(k)+1;
    char* nk = (char*) malloc(K_LEN);
    if (nullptr != nk) {
      memcpy(nk, k, K_LEN);
      _set_new_key(nk);
      _reap_key(true);
    }
    else {
      _alter_flags(true, MANUVR_KVP_FLAG_ERR_MEM);
    }
  }
  else {
    _set_new_key(k);
  }
}


/**
* Conditionally handles any cleanup associated with replacing the key.
* Passing nullptr will free any existing key without reassignment.
* Calling this function will reset the reapKey flag.
*
* @param  k A replacement key.
*/
void KeyValuePair::_set_new_key(char* k) {
  if ((nullptr != _key) && _reap_key()) {
    free((void*)_key);
    _reap_key(false);
  }
  _key = k;
}

/**
* Conditionally handles any cleanup associated with replacing the value.
* Passing nullptr will free any existing value without reassignment.
* Calling this function will reset the reapValue flag.
*
* @param  v A replacement value.
*/
void KeyValuePair::_set_new_value(void* v) {
  if ((nullptr != _target_mem) && reapValue()) {
    switch (_t_code) {
      // Types with destructors.
      case TCode::KVP:          delete (KeyValuePair*) _target_mem;   break;
      case TCode::STR_BUILDER:  delete (StringBuilder*) _target_mem;  break;
      case TCode::IDENTITY:     delete (Identity*) _target_mem;       break;

      #if defined(CONFIG_C3P_IMG_SUPPORT)
      case TCode::IMAGE:        delete (Image*) _target_mem;          break;
      #endif   // CONFIG_C3P_IMG_SUPPORT

      // Types that are malloc()'d.
      case TCode::INT64:
      case TCode::INT128:
      case TCode::UINT64:
      case TCode::UINT128:
      case TCode::DOUBLE:
      case TCode::BINARY:
      case TCode::STR:
      case TCode::VECT_3_FLOAT:
      case TCode::VECT_3_DOUBLE:
      case TCode::VECT_3_INT8:
      case TCode::VECT_3_UINT8:
      case TCode::VECT_3_INT16:
      case TCode::VECT_3_UINT16:
      case TCode::VECT_3_INT32:
      case TCode::VECT_3_UINT32:
        free((void*)_target_mem);
        break;

      default: break;   // Anything else is left alone.
    }
    reapValue(false);   // Reset the reap flag in all cases.
  }
  _target_mem = v;
}


/*******************************************************************************
* Accessors for linkage to parallel data.
*******************************************************************************/

/**
* Takes all the keys in the provided list chain and for any
*   that are ID'd by string keys, prints them to the provided buffer.
*
* @param  key_set A StringBuilder object that w e will write to.
* @return         The number of values written.
*/
int KeyValuePair::collectKeys(StringBuilder* key_set) {
  int return_value = 0;
  if (nullptr != _key) {
    key_set->concat(_key);
    return_value++;
  }
  if (nullptr != _next) {
    return_value += _next->collectKeys(key_set);
  }
  return return_value;
}


/**
* @return [description]
*/
KeyValuePair* KeyValuePair::retrieveByIdx(unsigned int idx) {
  switch (idx) {
    case 0:
      return this;   // Special case.
    default:
      if (nullptr != _next) {
        return _next->retrieveByIdx(idx-1);
      }
      // NOTE: No break
      // Fall-through if the index is greater than the list's cardinality.
    case 1:
      return _next;  // Terminus of recursion for all kvps but 0.
  }
}


/*
* Does an KeyValuePair in our rank have the given key?
*
* Returns nullptr if the answer is 'no'. Otherwise, ptr to the first matching key.
*/
KeyValuePair* KeyValuePair::retrieveByKey(const char* k) {
  if (nullptr != k) {
    if (nullptr != _key) {
      //if (_key == k) {
      // TODO: Awful. Hash map? pointer-comparisons?
      if (0 == strcmp(_key, k)) {
        // If pointer comparison worked, we win.
        return this;
      }
    }

    if (nullptr != _next) {
      return _next->retrieveByKey(k);
    }
  }
  return nullptr;
}


/**
* Given an KeyValuePair pointer, finds that pointer and drops it from the list.
*
* @param  drop  The KeyValuePair to drop.
* @return       0 on success. 1 on warning, -1 on "not found".
*/
int8_t KeyValuePair::drop(KeyValuePair** root, KeyValuePair* drop) {
  if (*root == drop) {
    // Re-write the root parameter.
    root = &_next; // NOTE: may be null. Who cares.
    return 0;
  }
  else if (_next && _next == drop) {
    _next = _next->_next; // NOTE: may be null. Who cares.
    return 0;
  }
  return (_next) ? _next->drop(root, drop) : -1;
}


/**
* @param kvp is the KVP to link.
*/
KeyValuePair* KeyValuePair::link(KeyValuePair* kvp) {
  if (nullptr == _next) {
    _next = kvp;
  }
  else {
    _next->link(kvp);
  }
  return kvp;
}

/**
* @return The number of KVPs in this list.
*/
int KeyValuePair::count() {
  return (1 + ((nullptr == _next) ? 0 : _next->count()));
}


/*******************************************************************************
* Accessors to type information and underpinnings.
*******************************************************************************/

/**
* @return [description]
*/
int8_t KeyValuePair::setValue(void* trg_buf, int len, TCode tc) {
  int8_t return_value = -1;
  if (typeCode() != tc) {
    return -2;
  }
  switch (tc) {
    case TCode::INT8:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT8:   // This frightens the compiler. Its fears are unfounded.
    case TCode::BOOLEAN:
      return_value = 0;
      *((uint8_t*)&_target_mem) = *((uint8_t*) trg_buf);
      break;
    case TCode::INT16:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT16:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint16_t*)&_target_mem) = *((uint16_t*) trg_buf);
      break;
    case TCode::INT32:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT32:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint32_t*)&_target_mem) = *((uint32_t*) trg_buf);
      break;
    case TCode::FLOAT:    // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *(((uint8_t*) &_target_mem) + 0) = *((uint8_t*) trg_buf + 0);
      *(((uint8_t*) &_target_mem) + 1) = *((uint8_t*) trg_buf + 1);
      *(((uint8_t*) &_target_mem) + 2) = *((uint8_t*) trg_buf + 2);
      *(((uint8_t*) &_target_mem) + 3) = *((uint8_t*) trg_buf + 3);
      break;

    case TCode::DOUBLE:
    case TCode::VECT_4_FLOAT:
    case TCode::VECT_3_FLOAT:
    case TCode::VECT_3_UINT32:
    case TCode::VECT_3_UINT16:
    case TCode::VECT_3_UINT8:
    case TCode::VECT_3_INT32:
    case TCode::VECT_3_INT16:
    case TCode::VECT_3_INT8:
      // TODO: This is probably wrong.
      return_value = 0;
      for (int i = 0; i < _len; i++) {
        *((uint8_t*) _target_mem + i) = *((uint8_t*) trg_buf + i);
      }
      break;
    case TCode::STR_BUILDER:     // A pointer to some StringBuilder.
    case TCode::STR:             // A pointer to a string constant.
    case TCode::IMAGE:           // A pointer to an Image.
    case TCode::KVP:             // A pointer to another KVP.
    case TCode::IDENTITY:        // A pointer to an Identity.
    default:
      return_value = 0;
      _target_mem = trg_buf;  // TODO: Need to do an allocation check and possible cleanup.
      break;
  }
  return return_value;
}


/**
*
* @param  idx      The KeyValuePair position
* @param  trg_buf  A pointer to the place where we should write the result.
* @return 0 on success or appropriate failure code.
*/
int8_t KeyValuePair::valueWithIdx(uint8_t idx, void* trg_buf) {
  int8_t return_value = -1;
  if (0 < idx) {
    if (nullptr != _next) {
      return_value = _next->valueWithIdx(--idx, trg_buf);
    }
  }
  else {
    return_value = getValueAs(trg_buf);
  }
  return return_value;
}

/**
* Get a value by its key.
*
* @param  k        The KeyValuePair key
* @param  trg_buf  A pointer to the place where we should write the result.
* @return 0 on success or appropriate failure code.
*/
int8_t KeyValuePair::valueWithKey(const char* k, void* trg_buf) {
  if (nullptr != k) {
    if (nullptr != _key) {
      //if (_key == k) {
      // TODO: Awful. Hash map? pointer-comparisons?
      if (0 == strcmp(_key, k)) {
        // If pointer comparison worked, return the value.
        return getValueAs(trg_buf);
      }
    }

    if (nullptr != _next) {
      return _next->valueWithKey(k, trg_buf);
    }
  }
  return -1;
}


/**
* All of the type-specialized getValueAs() fxns boil down to this. Which is private.
*
* @param  trg_buf  A pointer to the place where we should write the result.
* @return 0 on success or appropriate failure code.
*/
int8_t KeyValuePair::getValueAs(void* trg_buf) {
  int8_t return_value = -1;
  switch (typeCode()) {
    case TCode::INT8:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT8:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint8_t*) trg_buf) = *((uint8_t*)&_target_mem);
      break;
    case TCode::INT16:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT16:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint16_t*) trg_buf) = *((uint16_t*)&_target_mem);
      break;
    case TCode::INT32:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT32:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint32_t*) trg_buf) = *((uint32_t*)&_target_mem);
      break;
    case TCode::FLOAT:    // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint8_t*) trg_buf + 0) = *(((uint8_t*) &_target_mem) + 0);
      *((uint8_t*) trg_buf + 1) = *(((uint8_t*) &_target_mem) + 1);
      *((uint8_t*) trg_buf + 2) = *(((uint8_t*) &_target_mem) + 2);
      *((uint8_t*) trg_buf + 3) = *(((uint8_t*) &_target_mem) + 3);
      break;

    case TCode::DOUBLE:
      // TODO: This is probably wrong.
      return_value = 0;
      *((double*) trg_buf) = *((double*) _target_mem);
      break;
    case TCode::VECT_4_FLOAT:
    case TCode::VECT_3_FLOAT:
    case TCode::VECT_3_UINT32:
    case TCode::VECT_3_UINT16:
    case TCode::VECT_3_UINT8:
    case TCode::VECT_3_INT32:
    case TCode::VECT_3_INT16:
    case TCode::VECT_3_INT8:
      // TODO: This is probably wrong.
      return_value = 0;
      for (int i = 0; i < _len; i++) {
        *((uint8_t*) trg_buf + i) = *((uint8_t*) _target_mem + i);
      }
      break;

    case TCode::STR_BUILDER:     // A pointer to some StringBuilder.
    case TCode::STR:             // A pointer to a string constant.
    case TCode::IMAGE:           // A pointer to an Image.
    case TCode::KVP:             // A pointer to another KVP.
    case TCode::IDENTITY:        // A pointer to an Identity.
    default:
      return_value = 0;
      *((uintptr_t*) trg_buf) = ((uintptr_t) _target_mem);
      break;
  }
  return return_value;
}



/*******************************************************************************
* String processing and debug.
*******************************************************************************/

/**
* This function prints the KVP's value to the provided buffer.
*
* @param out is the buffer to receive the printed value.
*/
void KeyValuePair::valToString(StringBuilder* out) {
  uint8_t* buf = (uint8_t*) pointer();
  switch (_t_code) {
    case TCode::INT8:
    case TCode::INT16:
    case TCode::INT32:
    case TCode::INT64:
    case TCode::INT128:
      out->concatf("%d", (uintptr_t) pointer());
      break;
    case TCode::UINT8:
    case TCode::UINT16:
    case TCode::UINT32:
    case TCode::UINT64:
    case TCode::UINT128:
      out->concatf("%u", (uintptr_t) pointer());
      // TODO: Newlib-nano will not have support for 64-bit ints.
      //output->concatf("0x%08x%08x\n", ((uint32_t) (val >> 32)), ((uint32_t) (val & 0xFFFFFFFFULL)));
      break;
    case TCode::FLOAT:
      {
        float tmp;
        memcpy((void*) &tmp, &buf, 4);
        out->concatf("%.4f", (double) tmp);
      }
      break;
    case TCode::DOUBLE:
      {
        double tmp;
        getValueAs((void*) &tmp);
        out->concatf("%.6f", tmp);
      }
      break;

    case TCode::BOOLEAN:
      out->concatf("%s", ((uintptr_t) pointer() ? "true" : "false"));
      break;
    case TCode::STR_BUILDER:
      out->concat((StringBuilder*) pointer());
      break;
    case TCode::STR:
      out->concatf("%s", (const char*) pointer());
      break;
    case TCode::VECT_3_FLOAT:
      {
        Vector3<float>* v = (Vector3<float>*) pointer();
        out->concatf("(%.4f, %.4f, %.4f)", (double)(v->x), (double)(v->y), (double)(v->z));
      }
      break;
    case TCode::VECT_3_UINT32:
      {
        Vector3<uint32_t>* v = (Vector3<uint32_t>*) pointer();
        out->concatf("(%u, %u, %u)", v->x, v->y, v->z);
      }
      break;
    //case TCode::VECT_4_FLOAT:
    //  {
    //    Vector4f* v = (Vector4f*) pointer();
    //    out->concatf("(%.4f, %.4f, %.4f, %.4f)", (double)(v->w), (double)(v->x), (double)(v->y), (double)(v->z));
    //  }
    //  break;
    case TCode::KVP:
      if (nullptr != _target_mem) ((KeyValuePair*) _target_mem)->printDebug(out);
      break;
    case TCode::IDENTITY:
      if (nullptr != _target_mem) ((Identity*) _target_mem)->toString(out);
      break;
    default:
      {
        int l_ender = (_len < 16) ? _len : 16;
        for (int n = 0; n < l_ender; n++) {
          out->concatf("%02x ", *((uint8_t*) buf + n));
        }
      }
      break;
  }
}


/*
* Warning: call is propagated across entire list.
*/
void KeyValuePair::printDebug(StringBuilder* out) {
  out->concatf("\t%s\t%s\t%6s %6s ",
    (nullptr == _key ? "" : _key),
    typecodeToStr(_t_code),
    (_reap_key() ? "(rkey)" : ""),
    (reapValue() ? "(rval)" : "")
  );
  valToString(out);
  out->concat("\n");

  if (nullptr != _next) _next->printDebug(out);
}


/**
* This is a type-controlled branch-point for selecting the proper serializer for
*   the given TCode.
*
* @param out is the buffer to receive the serializer's output.
* @param TC is the desired encoding of the buffer.
* @return 0 on success. -1 on bad target TCode. -2 on packer failure.
*/
int8_t KeyValuePair::serialize(StringBuilder* out, TCode TC) {
  int8_t ret = -1;
  switch (TC) {
    default:  break;
    case TCode::BINARY:  ret = (0 == _encode_to_bin(out)) ? 0 : -2;     break;
    //case TCode::STR:     ret = (0 == _encode_to_string(out)) ? 0 : -2;  break;
    #if defined(CONFIG_C3P_CBOR)
    case TCode::CBOR:    ret = (0 == _encode_to_cbor(out)) ? 0 : -2;    break;
    #endif  // CONFIG_C3P_CBOR
    #if defined(CONFIG_C3P_JSON)
    #endif  // CONFIG_C3P_JSON
    #if defined(CONFIG_C3P_BASE64)
    #endif  // CONFIG_C3P_BASE64
  }
  return ret;
}


KeyValuePair* KeyValuePair::unserialize(uint8_t* src, unsigned int len, const TCode TC) {
  KeyValuePair* ret = nullptr;
  switch (TC) {
    default:  break;
    //case TCode::BINARY:  ret = (0 == _encode_to_bin(out)) ? 0 : -2;     break;
    //case TCode::STR:     ret = (0 == _encode_to_string(out)) ? 0 : -2;  break;
    #if defined(CONFIG_C3P_CBOR)
    case TCode::CBOR:
      {
        CBORArgListener listener(&ret);
        cbor::input input(src, len);
        cbor::decoder decoder(input, listener);
        decoder.run();
      }
      break;
    #endif  // CONFIG_C3P_CBOR
    #if defined(CONFIG_C3P_JSON)
    #endif  // CONFIG_C3P_JSON
    #if defined(CONFIG_C3P_BASE64)
    #endif  // CONFIG_C3P_BASE64
  }
  return ret;
}


/*******************************************************************************
* Parse-Pack: BIN
*******************************************************************************/

/**
* The purpose of this fxn is to pack up this KeyValuePair into something that can sent over a wire
*   with a minimum of overhead. We write only the bytes that *are* the data, and not the metadata
*   because we are relying on the parser at the other side to know what the type is.
* We still have to translate any pointer types into something concrete.
*
* @return 0 on success. Non-zero otherwise.
*/
int8_t KeyValuePair::_encode_to_bin(StringBuilder *out) {
  int8_t ret = 0;

  switch (_t_code) {
    case TCode::INT8:
    case TCode::UINT8:
    case TCode::INT16:
    case TCode::UINT16:
    case TCode::INT32:
    case TCode::UINT32:
    case TCode::FLOAT:
    case TCode::STR:
    case TCode::DOUBLE:
    case TCode::VECT_4_FLOAT:
    case TCode::VECT_3_FLOAT:
    case TCode::VECT_3_UINT32:
    case TCode::VECT_3_UINT16:
    case TCode::VECT_3_UINT8:
    case TCode::VECT_3_INT32:
    case TCode::VECT_3_INT16:
    case TCode::VECT_3_INT8:
    case TCode::BINARY:     // This is a pointer to a big binary blob.
      if (typeIsPointerPunned(_t_code)) {
        out->concat((unsigned char*) &_target_mem, _len);
      }
      else {
        out->concat((unsigned char*) _target_mem, _len);
      }
      break;

    case TCode::IDENTITY:
      {
        Identity* ident = (Identity*) _target_mem;
        uint16_t i_len = ident->length();
        uint8_t buf[i_len];
        if (ident->toBuffer(buf)) {
          out->concat(&buf[0], i_len);
        }
        //StringBuilder id_serial;
        //if (ident->toBuffer(&id_serial)) {
        //  StringBuilder tmp_log("Serialized an Identity: \n");
        //  id_serial.printDebug(&tmp_log);
        //  printf("%s\n", tmp_log.string());
        //  out->concatHandoff(&id_serial);
        //}
      }
      break;

    case TCode::KVP:
      {
        KeyValuePair* subj = (KeyValuePair*) _target_mem;
        StringBuilder intermediary;
        // NOTE: Recursion.
        if (0 == subj->_encode_to_bin(&intermediary)) {
          intermediary.string();  // Cause the buffer to be made contiguous.
          out->concatHandoff(&intermediary);
        }
      }
      break;

    /* These are pointer types that require conversion. */
    case TCode::STR_BUILDER:     // This is a pointer to some StringBuilder.
      out->concat((StringBuilder*) _target_mem);
      break;

    #if defined(CONFIG_C3P_IMG_SUPPORT)
    case TCode::IMAGE:      // This is a pointer to an Image.
      {
        Image* img = (Image*) _target_mem;
        uint32_t sz_buf = img->bytesUsed();
        if (sz_buf > 0) {
          if (0 != img->serialize(out)) {
            // Failure
          }
        }
      }
      break;
    #endif   // CONFIG_C3P_IMG_SUPPORT

    /* Anything else should be dropped. */
    default:
      break;
  }

  if (_next) {
    ret = _next->_encode_to_bin(out);
  }
  return ret;
}



/*******************************************************************************
* Parse-Pack: CBOR support
*******************************************************************************/

#if defined(CONFIG_C3P_CBOR)

int8_t KeyValuePair::_encode_to_cbor(StringBuilder* out) {
  cbor::output_stringbuilder output(out);
  cbor::encoder encoder(output);
  KeyValuePair* src = this;
  int8_t ret = 0;
  while (nullptr != src) {
    if (nullptr != src->getKey()) {
      // This is a map.
      encoder.write_map(1);
      encoder.write_string(src->getKey());
    }
    switch(src->typeCode()) {
      case TCode::INT8:
        {
          int8_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int((int) x);
          }
        }
        break;
      case TCode::INT16:
        {
          int16_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int((int) x);
          }
        }
        break;
      case TCode::INT32:
        {
          int32_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int((int) x);
          }
        }
        break;
      case TCode::INT64:
        {
          int64_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int(x);
          }
        }
        break;
      case TCode::UINT8:
        {
          uint8_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int((unsigned int) x);
          }
        }
        break;
      case TCode::UINT16:
        {
          uint16_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int((unsigned int) x);
          }
        }
        break;
      case TCode::UINT32:
        {
          uint32_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int((unsigned int) x);
          }
        }
        break;
      case TCode::UINT64:
        {
          uint64_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int(x);
          }
        }
        break;
      case TCode::FLOAT:
        {
          float x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_float(x);
          }
        }
        break;

      case TCode::DOUBLE:
        {
          double x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_double(x);
          }
        }
        break;

      case TCode::STR:
        {
          char* buf;
          if (0 == src->getValueAs(&buf)) {
            encoder.write_string(buf);
          }
        }
        break;
      case TCode::STR_BUILDER:
        {
          StringBuilder* buf;
          if (0 == src->getValueAs(&buf)) {
            encoder.write_string((char*) buf->string());
          }
        }
        break;

      case TCode::BINARY:
      case TCode::VECT_3_FLOAT:
      case TCode::VECT_4_FLOAT:
      case TCode::VECT_3_UINT32:
      case TCode::VECT_3_UINT16:
      case TCode::VECT_3_UINT8:
      case TCode::VECT_3_INT32:
      case TCode::VECT_3_INT16:
      case TCode::VECT_3_INT8:
        // NOTE: This ought to work for any types retaining portability isn't important.
        // TODO: Gradually convert types out of this block. As much as possible should
        //   be portable. VECT_3_FLOAT ought to be an array of floats, for instance.
        encoder.write_tag(C3P_CBOR_VENDOR_CODE | TcodeToInt(src->typeCode()));
        encoder.write_bytes((uint8_t*) src->pointer(), src->length());
        break;

      case TCode::IDENTITY:
        {
          Identity* ident;
          if (0 == src->getValueAs(&ident)) {
            uint16_t i_len = ident->length();
            uint8_t buf[i_len];
            if (ident->toBuffer(buf)) {
              encoder.write_tag(C3P_CBOR_VENDOR_CODE | TcodeToInt(src->typeCode()));
              encoder.write_bytes(buf, i_len);
            }
          }
        }
        break;
      case TCode::KVP:
        {
          StringBuilder intermediary;
          KeyValuePair* subj;
          if (0 == src->getValueAs(&subj)) {
            // NOTE: Recursion.
            if (0 == subj->_encode_to_cbor(&intermediary)) {
              encoder.write_tag(C3P_CBOR_VENDOR_CODE | TcodeToInt(src->typeCode()));
              encoder.write_bytes(intermediary.string(), intermediary.length());
            }
          }
        }
        break;
      case TCode::IMAGE:
        #if defined(CONFIG_C3P_IMG_SUPPORT)
          {
            Image* img;
            if (0 == src->getValueAs(&img)) {
              uint32_t sz_buf = img->bytesUsed();
              if (sz_buf > 0) {
                uint32_t nb_buf = 0;
                uint8_t intermediary[32] = {0, };
                if (0 == img->serializeWithoutBuffer(intermediary, &nb_buf)) {
                  encoder.write_tag(C3P_CBOR_VENDOR_CODE | TcodeToInt(src->typeCode()));
                  encoder.write_bytes(intermediary, nb_buf);   // TODO: This might cause two discrete CBOR objects.
                  //encoder.write_bytes(img->buffer(), sz_buf);
                }
              }
            }
          }
        #endif   // CONFIG_C3P_IMG_SUPPORT
        break;
      case TCode::RESERVED:
        // Peacefully ignore the types we can't export.
        break;
      default:
        // TODO: Handle pointer types, bool
        break;
    }
    src = src->_next;
  }
  return ret;
}


/**
* Return the RAM use of this string.
* By passing true to deep, the return value will also factor in concealed heap
*   overhead and the StringBuilder itself.
* Return value accounts for padding due to alignment constraints.
*
* @param deep will also factor in heap overhead, and the StringBuilder itself.
* @return 0 on success, or negative on failure.
*/
int KeyValuePair::memoryCost(bool deep) {
  // TODO: sizeof(intptr_t) for OVERHEAD_PER_MALLOC is an assumption based on a
  //   specific build of newlib. Find a way to discover it from the build.
  const uint32_t OVERHEAD_PER_CLASS  = (deep ? sizeof(KeyValuePair) : 0);
  const uint32_t OVERHEAD_PER_MALLOC = (deep ? sizeof(intptr_t) : 0);

  int32_t ret = OVERHEAD_PER_CLASS;
  ret += (_direct_value() ? 0 : OVERHEAD_PER_MALLOC);
  ret += length();
  if (nullptr != _next) {
    ret += _next->memoryCost(deep);
  }
  return ret;
}



/*******************************************************************************
* CBORArgListener
*
*
*******************************************************************************/

CBORArgListener::CBORArgListener(KeyValuePair** target) {    built = target;    }

CBORArgListener::~CBORArgListener() {
  // JIC...
  if (nullptr != _wait) {
    free(_wait);
    _wait = nullptr;
  }
}

/*
* Causes the KVP given as the argument to be added to the existing data.
*/
void CBORArgListener::_caaa(KeyValuePair* nu) {
  if (0 < _wait_map) {
    _wait_map--;
    if (nullptr != _wait) {
      nu->setKey(_wait);
      _wait = nullptr;
    }
  }
  else {
    nu->setValue(_wait);
  }

  if (0 < _wait_array) _wait_array--;

  if ((nullptr != built) && (nullptr != *built)) {
    (*built)->link(nu);
  }
  else {
    *built = nu;
  }
}


void CBORArgListener::on_string(char* val) {
  // Strings need special handling, because they might be used for map keys.
  int len = strlen(val);
  char* temp = (char*) malloc(len+1);
  if (nullptr != temp) {
    memcpy(temp, val, len+1);
    KeyValuePair* nu_kvp = nullptr;
    if (0 < _wait_map) {
      if (nullptr == _wait) {
        // We need to copy the string. It will be the key for the KeyValuePair
        //   who's value is forthcoming.
        _wait = temp;
      }
      else {
        // There is a key assignment waiting. This must be the value.
        nu_kvp = new KeyValuePair(temp);
      }
    }
    else {
      nu_kvp = new KeyValuePair(temp);
    }
    if (nullptr != nu_kvp) {
      nu_kvp->reapValue(true);
      _caaa(nu_kvp);
    }
  }
};

void CBORArgListener::on_bytes(uint8_t* data, int size) {
  if (TCode::NONE != _pending_manuvr_tag) {
    // If we've seen our vendor code in a tag, we interpret the first byte as a Manuvr
    //   Typecode, and build an KeyValuePair the hard way.
    const TCode TC = (const TCode) _pending_manuvr_tag;
    KeyValuePair* temp_kvp = _inflate_manuvr_type(data, size, TC);
    if (nullptr != temp_kvp) {
      _caaa(temp_kvp);
    }
    _pending_manuvr_tag = TCode::NONE;
  }
  else {
    _caaa(new KeyValuePair(data, size));
  }
};

void CBORArgListener::on_integer(int8_t v) {           _caaa(new KeyValuePair(v));               };
void CBORArgListener::on_integer(int16_t v) {          _caaa(new KeyValuePair(v));               };
void CBORArgListener::on_integer(int32_t v) {          _caaa(new KeyValuePair(v));               };
void CBORArgListener::on_integer(uint8_t v) {          _caaa(new KeyValuePair(v));               };
void CBORArgListener::on_integer(uint16_t v) {         _caaa(new KeyValuePair(v));               };
void CBORArgListener::on_integer(uint32_t v) {         _caaa(new KeyValuePair(v));               };
void CBORArgListener::on_float32(float f) {            _caaa(new KeyValuePair(f));               };
void CBORArgListener::on_double(double f) {            _caaa(new KeyValuePair(f));               };
void CBORArgListener::on_special(unsigned int code) {  _caaa(new KeyValuePair((uint32_t) code)); };
void CBORArgListener::on_error(const char* error) {    _caaa(new KeyValuePair(error));           };

void CBORArgListener::on_undefined() {   _caaa(new KeyValuePair("<UNDEF>"));   };
void CBORArgListener::on_null() {        _caaa(new KeyValuePair("<NULL>"));    };
void CBORArgListener::on_bool(bool x) {  _caaa(new KeyValuePair(x));           };

// NOTE: IANA gives us _some_ guidance....
// https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml
void CBORArgListener::on_tag(unsigned int tag) {
  switch (tag & 0xFFFFFF00) {
    case C3P_CBOR_VENDOR_CODE:
      _pending_manuvr_tag = IntToTcode(tag & 0x000000FF);
      break;
    default:
      break;
  }
};

void CBORArgListener::on_array(int size) {
  _wait_array = (int32_t) size;
};

void CBORArgListener::on_map(int size) {
  _wait_map   = (int32_t) size;
  if (nullptr != _wait) {
    // Flush so we can discover problems.
    free(_wait);
    _wait = nullptr;
  }
};

void CBORArgListener::on_extra_integer(unsigned long long value, int sign) {}
void CBORArgListener::on_extra_integer(long long value, int sign) {}
void CBORArgListener::on_extra_tag(unsigned long long tag) {}
void CBORArgListener::on_extra_special(unsigned long long tag) {}


KeyValuePair* CBORArgListener::_inflate_manuvr_type(uint8_t* data, int size, const TCode TC) {
  KeyValuePair* ret = nullptr;
  if (typeIsFixedLength(TC)) {
    if (size != sizeOfType(TC)) {
      return ret;
    }
  }

  if (nullptr != ret) {
    // If we can't fit the value into the KVP class, it means we new'd it.
    ret->reapValue(!typeIsPointerPunned(TC));
  }

  switch (TC) {
    case TCode::NONE:            break;
    case TCode::INT8:            ret = new KeyValuePair(*((int8_t*)data));    break;
    case TCode::INT16:           ret = new KeyValuePair(*((int16_t*)data));   break;
    case TCode::INT32:           ret = new KeyValuePair(*((int32_t*)data));   break;
    case TCode::UINT8:           ret = new KeyValuePair(*((uint8_t*)data));   break;
    case TCode::UINT16:          ret = new KeyValuePair(*((uint16_t*)data));  break;
    case TCode::UINT32:          ret = new KeyValuePair(*((uint32_t*)data));  break;
    case TCode::INT64:           break;
    case TCode::INT128:          break;
    case TCode::UINT64:          break;
    case TCode::UINT128:         break;
    case TCode::BOOLEAN:         ret = new KeyValuePair((0 != *data));  break;
    case TCode::FLOAT:           ret = new KeyValuePair(*((float*)data));   break;
    case TCode::DOUBLE:          ret = new KeyValuePair(*((double*)data));  break;
    //case TCode::VECT_2_FLOAT:
    //case TCode::VECT_2_DOUBLE:
    //case TCode::VECT_2_INT8:
    //case TCode::VECT_2_UINT8:
    //case TCode::VECT_2_INT16:
    //case TCode::VECT_2_UINT16:
    //case TCode::VECT_2_INT32:
    //case TCode::VECT_2_UINT32:
    case TCode::VECT_3_FLOAT:
      ret = new KeyValuePair(new Vector3<float>(*((float*)(data+0)), *((float*)(data+4)), *((float*)(data+8))));
      if (nullptr != ret) ret->reapValue(true);
      break;
    //case TCode::VECT_3_DOUBLE:
    case TCode::VECT_3_INT8:
      ret = new KeyValuePair(new Vector3<int8_t>(*((int8_t*)(data+0)), *((int8_t*)(data+1)), *((int8_t*)(data+2))));
      if (nullptr != ret) ret->reapValue(true);
      break;
    case TCode::VECT_3_UINT8:
      ret = new KeyValuePair(new Vector3<uint8_t>(*((uint8_t*)(data+0)), *((uint8_t*)(data+1)), *((uint8_t*)(data+2))));
      if (nullptr != ret) ret->reapValue(true);
      break;
    case TCode::VECT_3_INT16:
      ret = new KeyValuePair(new Vector3<int16_t>(*((int16_t*)(data+0)), *((int16_t*)(data+2)), *((int16_t*)(data+4))));
      if (nullptr != ret) ret->reapValue(true);
      break;
    case TCode::VECT_3_UINT16:
      ret = new KeyValuePair(new Vector3<uint16_t>(*((uint16_t*)(data+0)), *((uint16_t*)(data+2)), *((uint16_t*)(data+4))));
      if (nullptr != ret) ret->reapValue(true);
      break;
    case TCode::VECT_3_INT32:
      ret = new KeyValuePair(new Vector3<int32_t>(*((int32_t*)(data+0)), *((int32_t*)(data+4)), *((int32_t*)(data+8))));
      if (nullptr != ret) ret->reapValue(true);
      break;
    case TCode::VECT_3_UINT32:
      ret = new KeyValuePair(new Vector3<uint32_t>(*((uint32_t*)(data+0)), *((uint32_t*)(data+4)), *((uint32_t*)(data+8))));
      if (nullptr != ret) ret->reapValue(true);
      break;
    //case TCode::VECT_4_FLOAT:
    //case TCode::URL:
    //case TCode::JSON:
    //case TCode::CBOR:
    //case TCode::LATLON:
    //case TCode::COLOR8:
    //case TCode::COLOR16:
    //case TCode::COLOR24:
    //case TCode::SI_UNIT:
    //case TCode::BASE64:
    //case TCode::IPV4_ADDR:
    //case TCode::AUDIO:

    // Inflation of KVPs.
    case TCode::KVP:
      {
        KeyValuePair* tmp = KeyValuePair::unserialize(data, size, TCode::CBOR);
        if (tmp) {
          ret = new KeyValuePair(tmp);
          if (nullptr != ret) ret->reapValue(true);
        }
      }
      break;

    // Inflation of Identities.
    case TCode::IDENTITY:
      {
        Identity* tmp = Identity::fromBuffer(data, size);
        if (tmp) {
          ret = new KeyValuePair(tmp);
          if (nullptr != ret) ret->reapValue(true);
        }
      }
      break;

    #if defined(CONFIG_C3P_IMG_SUPPORT)
      // Inflation of Images.
      case TCode::IMAGE:
        {
          Image* tmp = new Image();
          if (tmp) {
            tmp->deserialize(data, size);
            if (tmp->allocated()) {
            }
            ret = new KeyValuePair(tmp);
            if (nullptr != ret) ret->reapValue(true);
          }
        }
        break;
    #endif   // CONFIG_C3P_IMG_SUPPORT

    // Any other TCodes will either be handled by a CBOR native type, or should
    //   never have been serialized in the first place.
    default:
      break;
  }
  return ret;
}

#endif // CONFIG_C3P_CBOR
