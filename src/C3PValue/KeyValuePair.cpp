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
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

KeyValuePair::KeyValuePair(const TCode TC, const char* key, uint8_t flags) : C3PValue(TC) {
  setKey(key);
  _set_flags(true, (flags | C3PVAL_MEM_FLAG_HAS_KEY));
}


KeyValuePair::KeyValuePair(const TCode TC, char* key, uint8_t flags) : C3PValue(TC) {
  setKey(key);
  _set_flags(true, (flags | C3PVAL_MEM_FLAG_HAS_KEY));
}


KeyValuePair::KeyValuePair(const char* key, uint8_t* v, uint32_t l, uint8_t flags) : C3PValue(v, l) {
  setKey(key);
  _set_flags(true, (flags | C3PVAL_MEM_FLAG_HAS_KEY));
}


KeyValuePair::KeyValuePair(char* key, uint8_t* v, uint32_t l, uint8_t flags) : C3PValue(v, l) {
  setKey(key);
  _set_flags(true, (flags | C3PVAL_MEM_FLAG_HAS_KEY));
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
      _set_flags(true, C3PVAL_MEM_FLAG_ERR_MEM);
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

// /**
// * Conditionally handles any cleanup associated with replacing the value.
// * Passing nullptr will free any existing value without reassignment.
// * Calling this function will reset the reapValue flag.
// *
// * @param  v A replacement value.
// */
// void KeyValuePair::_set_new_value(C3PValue* v) {
//   if ((nullptr != _value) && reapContainer()) {
//     C3PValue* tmp_value = _value;
//     _value = nullptr;
//     delete tmp_value;
//   }
//   _value = v;
// }


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
  KeyValuePair* nxt = _next_sib_with_key();
  if (nullptr != nxt) {
    return_value += nxt->collectKeys(key_set);
  }
  return return_value;
}


/*
* Does an KeyValuePair in our rank have the given key?
*
* Returns nullptr if the answer is 'no'. Otherwise, ptr to the first matching key.
*/
KeyValuePair* KeyValuePair::valueWithKey(const char* k) {
  if (nullptr != k) {
    if (nullptr != _key) {
      if (0 == strcmp(_key, k)) {
        return this;
      }
    }

    KeyValuePair* nxt = _next_sib_with_key();
    if (nullptr != nxt) {
      return nxt->valueWithKey(k);
    }
  }
  return nullptr;
}


/*******************************************************************************
* Accessors to type information and underpinnings.
*******************************************************************************/

/**
* Get a value by its key.
*
* @param  k        The KeyValuePair key
* @param  trg_buf  A pointer to the place where we should write the result.
* @return 0 on success or appropriate failure code.
*/
int8_t KeyValuePair::valueWithKey(const char* k, void* trg_buf) {
  int8_t ret = -1;
  KeyValuePair* val_container = valueWithKey(k);
  if (nullptr != val_container) {
    ret = val_container->get_as(val_container->tcode(), trg_buf);
  }
  return ret;
}


// int8_t KeyValuePair::convertToType(const TCode TC) {
//   int8_t ret = -1;
//   if (nullptr != _value) {
//     // We are experiencing a request to convert an existing value into a new
//     //   type. Sometimes this is done to upscale numeric types, or impart a
//     //   semantic alias to a more-basal type. In any case, the TCode in a
//     //   C3PValue, and is const. Thus, this will probably imply memory shuffle.
//     if (TC == tcode()) {
//       ret = 0;
//     }
//     else {
//       if (reapContainer()) {
//         // TODO: Safe conversion.
//         C3PValue* replacement = new C3PValue(TC);
//         if (nullptr != replacement) {
//           if (0 == replacement->set(_value)) {
//             if (reapValue()) {
//               // TODO: If this container was the owner of the data it held, and
//               //   it wasn't a deep-copy, we might need to take ownership of it?
//               //   Other assurances might make this check useless.
//             }
//             delete _value;
//             _value = replacement;
//             replacement = nullptr;
//             ret = 0;
//           }
//           else {
//             // Somehow we failed to set the value from our existing container.
//             // Don't leak memory...
//             delete replacement;
//           }
//         }
//       }
//       else {
//         // If the existing container needs to be replaced, but is not ours to
//         //   reap, we can't do anything about it. Not for leak reasons, but
//         //   because we have no means to update the pointers held by whatever
//         //   DOES has ownership.
//       }
//     }
//   }
//   else {
//     // This is a request to allocate a container for an (until now) unspecififed
//     //   type. Note that no value is imparted. So the container will initialize
//     //   to T(0) until set.
//     _set_new_value(new C3PValue(TC));
//     reapContainer(true);
//     ret = (nullptr != _value) ? 0 : -1;
//   }
//   return ret;
// }



/*******************************************************************************
* String processing and debug.
*******************************************************************************/
/**
* This is a type-controlled branch-point for selecting the proper serializer for
*   the given TCode.
*
* @param out is the buffer to receive the serializer's output.
* @param TC is the desired encoding of the buffer.
* @return 0 on success. -1 on bad target TCode. -2 on packer failure.
*/
int8_t KeyValuePair::serialize(StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  // Use an intermediary StringBuilder so we can collapse the strings a
  //   bit more neatly.
  StringBuilder local_output;
  KeyValuePair* src = this;
  switch (FORMAT) {
    case TCode::STR:
      src->_encode_to_printable(&local_output);
      break;
    case TCode::BINARY:
      break;

    #if defined(__BUILD_HAS_CBOR)
    case TCode::CBOR:
      {
        uint32_t kvp_count = 0;
        cbor::output_stringbuilder output(&local_output);
        cbor::encoder local_encoder(output);
        while (nullptr != src) {
          C3PType* t_helper = getTypeHelper(src->tcode());
          if (nullptr != t_helper) {
            char* tmp_key = src->getKey();
            if ((nullptr != tmp_key) && (strlen(tmp_key) > 0)) {
              local_encoder.write_string(tmp_key);
              ret = t_helper->serialize(src->_type_pun_get(), &local_output, FORMAT);
              kvp_count++;
            }
            else {
              // Peacefully ignore KVPs without keys.
            }
          }
          src = src->_next_sib_with_key();
        }
        if (kvp_count > 0) {
          cbor::output_stringbuilder top_output(out);
          cbor::encoder top_encoder(top_output);
          top_encoder.write_map(kvp_count);  // This is a map.
        }
      }
      break;
    #endif  // __BUILD_HAS_CBOR

    default:  break;
  }

  if (!local_output.isEmpty()) {
    local_output.string();  // Consolidate the heap.
    out->concatHandoff(&local_output);
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
    const unsigned int SIBS = src->count();  // Count the siblings.
    out->concat(&indent);
    if (nullptr != src->getKey()) {
      out->concat(src->getKey());
      out->concat(": ");
    }
    //else if (SIBS > 0) {
    //  // No key means that this is being used to store an array of values.
    //  // TODO: Presently, there is no other way.
    //  // Render them in a way that makes that clear.
    //  out->concat("[");
    //}

    switch (src->tcode()) {
      case TCode::KVP:
        {
          // If the value is a KVP, recurse with a deeper indentation to
          //   render it.
          KeyValuePair* tmp = (KeyValuePair*) _type_pun_get();
          if (nullptr != tmp) {
            out->concat("{\n");
            tmp->_encode_to_printable(out, (LEVEL+1));
            out->concat("}");
          }
        }
        break;

      case TCode::STR:
        out->concat("\"");
        src->toString(out);
        out->concat("\"");
        break;

      default:
        src->toString(out);
        break;
    }
    src = src->_next_sib_with_key();
    out->concat((nullptr != src) ? ",\n" : "\n");
  }
  return ret;
}


/*******************************************************************************
* Parse-Pack: CBOR support
*******************************************************************************/

#if defined(__BUILD_HAS_CBOR)

/*******************************************************************************
* CBORArgListener
*
* TODO: This has value for cases where memory should be consumed as it can be,
*   and object creation can happen in steps. But that behavior should be an
*   option in C3PValueDecoder, and this class should be entirely subsumed into
*   it. Possibly the CBOR library will follow, since C3PValueDecoder contains
*   its own parallel implementation that was derived from it.
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
    nu->set(_wait);
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
    ret->reapContainer(true);
    ret->reapValue(!typeIsPointerPunned(TC));
  }
  return ret;
}

#endif // __BUILD_HAS_CBOR
