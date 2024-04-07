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

KeyValuePair::KeyValuePair(const char* key, C3PValue* v, uint8_t flags) {
  setKey(key);
  _set_new_value(v);
  _alter_flags(true, flags);
}


KeyValuePair::KeyValuePair(char* key, C3PValue* v, uint8_t flags) {
  setKey(key);
  _set_new_value(v);
  _alter_flags(true, flags);
}


/**
* Destructor. Frees memory associated with this KeyValuePair.
* Recursively calls the destructor of a referenced KeyValuePair, if present.
*/
KeyValuePair::~KeyValuePair() {
  _set_new_key(nullptr);
  _set_new_value(nullptr);
  if (nullptr != _next) {
    // Wipe out any chain of siblings.
    KeyValuePair* a = _next;
    _next       = nullptr;
    if (a->reapKVP()) {
      delete a;
    }
  }
  _flags = 0;
}



/*******************************************************************************
* Accessors for key.
*******************************************************************************/

/*
* Take a key allocated elsewhere, and decline responsibility for it.
*/
int8_t KeyValuePair::setKey(const char* k) {
  _set_new_key((char*) k);
  _reap_key(false);
  return 0;
}

/*
* Take a key allocated elsewhere, and deep-copy it, taking responsibility for
*   the copy.
*/
int8_t KeyValuePair::setKey(char* k) {
  int8_t ret = 0;
  if (nullptr != k) {
    ret--;
    int K_LEN = strlen(k);
    char* nk = (char*) malloc(K_LEN + 1);
    if (nullptr != nk) {
      memcpy(nk, k, K_LEN);
      *(nk + K_LEN) = 0;
      _set_new_key(nk);
      _reap_key(true);
      ret = 0;
    }
    else {
      _alter_flags(true, C3P_KVP_FLAG_ERR_MEM);
    }
  }
  else {
    _set_new_key(k);
    _reap_key(false);
  }
  return ret;
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
void KeyValuePair::_set_new_value(C3PValue* v) {
  if ((nullptr != _value) && reapContainer()) {
    C3PValue* tmp_value = _value;
    _value = nullptr;
    delete tmp_value;
  }
  _value = v;
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
      if (0 == strcmp(_key, k)) {
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
KeyValuePair* KeyValuePair::link(KeyValuePair* kvp, bool reap_kvp) {
  if (nullptr == _next) {
    if (nullptr != kvp) {
      kvp->reapKVP(reap_kvp);
    }
    _next = kvp;
  }
  else {
    _next->link(kvp, reap_kvp);
  }
  return kvp;
}

/**
* @return The number of KVPs in this list.
*/
uint32_t KeyValuePair::count() {
  return (1 + ((nullptr == _next) ? 0 : _next->count()));
}


/*******************************************************************************
* Accessors to type information and underpinnings.
*******************************************************************************/

/**
* Get a value container by its index.
*
* @param  idx      The sibling position
* @return The value container for the KVP at the given index.
*/
C3PValue* KeyValuePair::valueWithIdx(uint32_t idx) {
  C3PValue* ret = nullptr;
  if (0 < idx) {
    if (nullptr != _next) {
      ret = _next->valueWithIdx(--idx);
    }
  }
  else {
    ret = _value;
  }
  return ret;
}

/**
* Get a value container by its key.
*
* @param  k        The desired key
* @return The value container for the KVP with the given key.
*/
C3PValue* KeyValuePair::valueWithKey(const char* k) {
  C3PValue* ret = nullptr;
  if (nullptr != k) {
    if (nullptr != _key) {
      if (0 == strcmp(_key, k)) {
        ret = _value;
      }
    }

    if ((nullptr == ret) & (nullptr != _next)) {
      return _next->valueWithKey(k);
    }
  }
  return ret;
}


/**
*
* @param  idx      The KeyValuePair position
* @param  trg_buf  A pointer to the place where we should write the result.
* @return 0 on success or appropriate failure code.
*/
int8_t KeyValuePair::valueWithIdx(uint32_t idx, void* trg_buf) {
  int8_t ret = -1;
  C3PValue* val_container = valueWithIdx(idx);
  if (nullptr != val_container) {
    ret = val_container->get_as(val_container->tcode(), trg_buf);
  }
  return ret;
}

/**
* Get a value by its key.
*
* @param  k        The KeyValuePair key
* @param  trg_buf  A pointer to the place where we should write the result.
* @return 0 on success or appropriate failure code.
*/
int8_t KeyValuePair::valueWithKey(const char* k, void* trg_buf) {
  int8_t ret = -1;
  C3PValue* val_container = valueWithKey(k);
  if (nullptr != val_container) {
    ret = val_container->get_as(val_container->tcode(), trg_buf);
  }
  return ret;
}


int8_t KeyValuePair::convertToType(const TCode TC) {
  int8_t ret = -1;
  if (nullptr != _value) {
    // We are experiencing a request to convert an existing value into a new
    //   type. Sometimes this is done to upscale numeric types, or impart a
    //   semantic alias to a more-basal type. In any case, the TCode in a
    //   C3PValue, and is const. Thus, this will probably imply memory shuffle.
    if (TC == _value->tcode()) {
      ret = 0;
    }
    else {
      if (reapContainer()) {
        // TODO: Safe conversion.
        C3PValue* replacement = new C3PValue(TC);
        if (nullptr != replacement) {
          if (0 == replacement->set(_value)) {
            if (_value->reapValue()) {
              // TODO: If this container was the owner of the data it held, and
              //   it wasn't a deep-copy, we might need to take ownership of it?
              //   Other assurances might make this check useless.
            }
            delete _value;
            _value = replacement;
            replacement = nullptr;
            ret = 0;
          }
          else {
            // Somehow we failed to set the value from our existing container.
            // Don't leak memory...
            delete replacement;
          }
        }
      }
      else {
        // If the existing container needs to be replaced, but is not ours to
        //   reap, we can't do anything about it. Not for leak reasons, but
        //   because we have no means to update the pointers held by whatever
        //   DOES has ownership.
      }
    }
  }
  else {
    // This is a request to allocate a container for an (until now) unspecififed
    //   type. Note that no value is imparted. So the container will initialize
    //   to T(0) until set.
    _set_new_value(new C3PValue(TC));
    reapContainer(true);
    ret = (nullptr != _value) ? 0 : -1;
  }
  return ret;
}



/*******************************************************************************
* String processing and debug.
*******************************************************************************/

void KeyValuePair::valToString(StringBuilder* out) {
  if (nullptr != _value) {  _value->toString(out);   }
  else {                    out->concat("(nullptr)"); }
}


/*
* Warning: call is propagated across entire list.
*/
void KeyValuePair::printDebug(StringBuilder* out) {
  StringBuilder tmp;
  tmp.concatf("\t%10s %5s %5s %5s %5s\t",
    (nullptr == _key ? "" : _key),
    (_reap_key()     ? "(key)" : ""),
    (reapKVP()       ? "(kvp)" : ""),
    (reapContainer() ? "(con)" : ""),
    (reapValue()     ? "(val)" : "")
  );
  if (nullptr != _value) {
    _value->toString(&tmp, true);
  }
  tmp.concat('\n');
  tmp.string();
  out->concatHandoff(&tmp);

  if (nullptr != _next) _next->printDebug(out);
}


/**
* Return the RAM use of this KVP.
* By passing true to deep, the return value will also factor in concealed heap
*   overhead of the containers themselves.
* Return value accounts for padding due to alignment constraints.
*
* @param deep will also factor in heap overhead of the containers.
* @return 0 on success, or negative on failure.
*/
int KeyValuePair::memoryCost(bool deep) {
  // TODO: sizeof(intptr_t) for OVERHEAD_PER_MALLOC is an assumption based on a
  //   specific build of newlib. Find a way to discover it from the build.
  const uint32_t OVERHEAD_PER_CLASS  = (deep ? sizeof(KeyValuePair) : 0);
  const uint32_t OVERHEAD_PER_MALLOC = (deep ? sizeof(intptr_t) : 0);

  int32_t ret = OVERHEAD_PER_CLASS;
  ret += OVERHEAD_PER_MALLOC;
  ret += length();
  if (nullptr != _next) {
    ret += _next->memoryCost(deep);
  }
  return ret;
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
    case TCode::BINARY:  ret = (0 == _encode_to_bin(out)) ? 0 : -2;        break;
    case TCode::STR:     ret = (0 == _encode_to_printable(out)) ? 0 : -2;  break;
    #if defined(__BUILD_HAS_CBOR)
    case TCode::CBOR:    ret = (0 == _encode_to_cbor(out)) ? 0 : -2;       break;
    #endif  // __BUILD_HAS_CBOR
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
    #if defined(__BUILD_HAS_CBOR)
    case TCode::CBOR:
      {
        CBORArgListener listener(&ret);
        cbor::input_static input(src, len);
        cbor::decoder decoder(input, listener);
        decoder.run();
      }
      break;
    #endif  // __BUILD_HAS_CBOR
    #if defined(CONFIG_C3P_JSON)
    #endif  // CONFIG_C3P_JSON
    #if defined(CONFIG_C3P_BASE64)
    #endif  // CONFIG_C3P_BASE64
  }
  return ret;
}


/*******************************************************************************
* Parse-Pack: String
*******************************************************************************/

/**
* The purpose of this fxn is to pack up this KeyValuePair into something that
*   can be rendered to a console. We write only the things that are important
*   to a human wanting to see the content.
*
* @return 0 on success. Non-zero otherwise.
*/
int8_t KeyValuePair::_encode_to_printable(StringBuilder* out, const unsigned int LEVEL) {
  KeyValuePair* src = this;
  int8_t ret = 0;
  // Decide if this is an array, or an object, or a singleton KVP.
  StringBuilder indent;
  for (unsigned int i = 0; i < LEVEL; i++) {  indent.concat("  ");  }

  while (nullptr != src) {
    C3PValue* val_container = src->getValue();
    const unsigned int SIBS = src->count();  // Count the siblings.
    if (nullptr != val_container) {
      out->concat(&indent);
      if (nullptr != src->getKey()) {
        out->concatf("%s: ", src->getKey());
      }
      else if (SIBS > 0) {
        // No key means that this is being used to store an array of values.
        // TODO: Presently, there is no other way.
        // Render them in a way that makes that clear.
        out->concat("[");
      }

      switch (val_container->tcode()) {
        case TCode::KVP:
          {
            KeyValuePair* tmp = nullptr;
            val_container->get_as(&tmp);
            if (nullptr != tmp) {
              out->concat("{\n");
              tmp->_encode_to_printable(out, (LEVEL+1));
              out->concat("}");
            }
          }
          break;

        case TCode::STR:
          out->concat("\"");
          val_container->toString(out);
          out->concat("\"");
          break;

        default:
          val_container->toString(out);
          break;
      }
      out->concat((nullptr != src->_next) ? ",\n" : "\n");
    }
    else {
      // Peacefully ignore KVPs without value containers.
    }
    src = src->_next;
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
  KeyValuePair* src = this;
  int8_t ret = 0;
  while (nullptr != src) {
    C3PValue* val_container = src->getValue();
    if (nullptr != val_container) {
      if (nullptr != src->getKey()) {
        // This is a map.
        // TODO: Write TCode::KVP, followed by an optional string.
      }
      val_container->serialize(out, TCode::CBOR);
    }
    else {
      // Peacefully ignore KVPs without value containers.
    }
    src = src->_next;
  }

  // TODO: I very much prefer recursion... Dig...
  //if (_next) {
  //  ret = _next->_encode_to_bin(out);
  //}
  return ret;
}



/*******************************************************************************
* Parse-Pack: CBOR support
*******************************************************************************/

#if defined(__BUILD_HAS_CBOR)

int8_t KeyValuePair::_encode_to_cbor(StringBuilder* out) {
  // NOTE: This function exhibits concurrent use of two seperate CBOR encoder
  //   objects on the same StringBuilder memory pool. This ought to be safe, as
  //   long as all downstream CBOR encoder arrangements also are children of
  //   output_stringbuilder (they ought to be).
  // TODO: StringBuilder local_output;
  cbor::output_stringbuilder output(out);
  cbor::encoder encoder(output);
  KeyValuePair* src = this;
  int8_t ret = 0;
  encoder.write_map(count());  // This is a map.
  while (nullptr != src) {
    C3PValue* val_container = src->getValue();
    if (nullptr != val_container) {
      if (nullptr != src->getKey()) {
        encoder.write_string(src->getKey());
        val_container->serialize(out, TCode::CBOR);
      }
      else {
        // Peacefully ignore KVPs without keys.
      }
    }
    else {
      // Peacefully ignore KVPs without value containers.
    }
    src = src->_next;
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
  if (TCode::NONE != _pending_c3p_tag) {
    // If we've seen our vendor code in a tag, we interpret the first byte as a
    //   C3P Typecode, and build an KeyValuePair the hard way.
    const TCode TC = (const TCode) _pending_c3p_tag;
    KeyValuePair* temp_kvp = _inflate_c3p_type(data, size, TC);
    if (nullptr != temp_kvp) {
      _caaa(temp_kvp);
    }
    _pending_c3p_tag = TCode::NONE;
  }
  else {
    _caaa(new KeyValuePair(data, size));
  }
};

void CBORArgListener::on_integer(int8_t v) {           _caaa(new KeyValuePair(v));               };
void CBORArgListener::on_integer(int16_t v) {          _caaa(new KeyValuePair(v));               };
void CBORArgListener::on_integer(int32_t v) {          _caaa(new KeyValuePair(v));               };
void CBORArgListener::on_integer(int64_t v) {          _caaa(new KeyValuePair(v));               };
void CBORArgListener::on_integer(uint8_t v) {          _caaa(new KeyValuePair(v));               };
void CBORArgListener::on_integer(uint16_t v) {         _caaa(new KeyValuePair(v));               };
void CBORArgListener::on_integer(uint32_t v) {         _caaa(new KeyValuePair(v));               };
void CBORArgListener::on_integer(uint64_t v) {         _caaa(new KeyValuePair(v));               };
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
      _pending_c3p_tag = IntToTcode(tag & 0x000000FF);
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


KeyValuePair* CBORArgListener::_inflate_c3p_type(uint8_t* data, int size, const TCode TC) {
  KeyValuePair* ret = nullptr;
  if (typeIsFixedLength(TC)) {
    if (size != sizeOfType(TC)) {
      return ret;
    }
  }

  switch (TC) {
    case TCode::NONE:            break;
    case TCode::INT8:            ret = new KeyValuePair(*((int8_t*)data));    break;
    case TCode::INT16:           ret = new KeyValuePair(*((int16_t*)data));   break;
    case TCode::INT32:           ret = new KeyValuePair(*((int32_t*)data));   break;
    case TCode::UINT8:           ret = new KeyValuePair(*((uint8_t*)data));   break;
    case TCode::UINT16:          ret = new KeyValuePair(*((uint16_t*)data));  break;
    case TCode::UINT32:          ret = new KeyValuePair(*((uint32_t*)data));  break;
    case TCode::INT64:           ret = new KeyValuePair(*((int64_t*)data));   break;
    case TCode::INT128:          break;
    case TCode::UINT64:          ret = new KeyValuePair(*((uint64_t*)data));  break;
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
      break;
    case TCode::VECT_3_DOUBLE:
      ret = new KeyValuePair(new Vector3<double>(*((double*)(data+0)), *((double*)(data+8)), *((double*)(data+16))));
      break;
    case TCode::VECT_3_INT8:
      ret = new KeyValuePair(new Vector3<int8_t>(*((int8_t*)(data+0)), *((int8_t*)(data+1)), *((int8_t*)(data+2))));
      break;
    case TCode::VECT_3_UINT8:
      ret = new KeyValuePair(new Vector3<uint8_t>(*((uint8_t*)(data+0)), *((uint8_t*)(data+1)), *((uint8_t*)(data+2))));
      break;
    case TCode::VECT_3_INT16:
      ret = new KeyValuePair(new Vector3<int16_t>(*((int16_t*)(data+0)), *((int16_t*)(data+2)), *((int16_t*)(data+4))));
      break;
    case TCode::VECT_3_UINT16:
      ret = new KeyValuePair(new Vector3<uint16_t>(*((uint16_t*)(data+0)), *((uint16_t*)(data+2)), *((uint16_t*)(data+4))));
      break;
    case TCode::VECT_3_INT32:
      ret = new KeyValuePair(new Vector3<int32_t>(*((int32_t*)(data+0)), *((int32_t*)(data+4)), *((int32_t*)(data+8))));
      break;
    case TCode::VECT_3_UINT32:
      ret = new KeyValuePair(new Vector3<uint32_t>(*((uint32_t*)(data+0)), *((uint32_t*)(data+4)), *((uint32_t*)(data+8))));
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
        }
      }
      break;

    #if defined(CONFIG_C3P_IDENTITY_SUPPORT)
      // Inflation of Identities.
      case TCode::IDENTITY:
        {
          Identity* tmp = Identity::fromBuffer(data, size);
          if (tmp) {
            ret = new KeyValuePair(tmp);
          }
        }
        break;
    #endif   // CONFIG_C3P_IDENTITY_SUPPORT

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
          }
        }
        break;
    #endif   // CONFIG_C3P_IMG_SUPPORT

    // Any other TCodes will either be handled by a CBOR native type, or should
    //   never have been serialized in the first place.
    default:
      break;
  }

  // If we can't fit the value into the KVP class, it means we new'd it.
  if (nullptr != ret) {
    ret->reapKVP(true);
    ret->reapValue(!typeIsPointerPunned(TC));
  }
  return ret;
}

#endif // __BUILD_HAS_CBOR
