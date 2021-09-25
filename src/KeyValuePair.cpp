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


This class represents our type-abstraction layer. It is the means by which
  we parse from messages without copying.
*/


#if false

#include "KeyValuePair.h"

#include <PriorityQueue.h>
#if defined(CONFIG_MANUVR_IMG_SUPPORT)
  #include <Image/Image.h>
#endif   // CONFIG_MANUVR_IMG_SUPPORT

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

/*
* On a given ALU width, some types fit into the same space as a pointer.
* This function returns true if the given TCode represents such a type.
*/
static bool _is_type_copy_by_value(const TCode TC) {
  switch (TC) {
    case TCode::NONE:           // No data. So: yes.
    case TCode::INT8:           // 8-bit integer
    case TCode::INT16:          // 16-bit integer
    case TCode::INT32:          // 32-bit integer
    case TCode::UINT8:          // Unsigned 8-bit integer
    case TCode::UINT16:         // Unsigned 16-bit integer
    case TCode::UINT32:         // Unsigned 32-bit integer
    case TCode::INT64:          // 64-bit integer
    case TCode::INT128:         // 128-bit integer
    case TCode::UINT64:         // Unsigned 64-bit integer
    case TCode::UINT128:        // Unsigned 128-bit integer
    case TCode::BOOLEAN:        // A boolean
    case TCode::FLOAT:          // A float
    case TCode::COLOR8:         // Alias of UINT8. 8-bit color data
    case TCode::COLOR16:        // Alias of UINT16. 16-bit color data
    case TCode::COLOR24:        // Alias of UINT32. 24-bit color data
    case TCode::SI_UNIT:        // Alias of UINT8. An SIUnit enum value.
    case TCode::IPV4_ADDR:      // Alias of UINT32. An IP address, in network byte-order.
    case TCode::RESERVED:       // Reserved for custom extension.
      return true;
    case TCode::DOUBLE:         // A double
    default:
      return false;
  }
}


#if defined(CONFIG_MANUVR_CBOR)

int8_t KeyValuePair::encodeToCBOR(KeyValuePair* src, StringBuilder* out) {
  cbor::output_dynamic output;
  cbor::encoder encoder(output);
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
          long long x = 0;
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
          unsigned long long x = 0;
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
        // NOTE: This ought to work for any types retaining portability isn't important.
        // TODO: Gradually convert types out of this block. As much as possible should
        //   be portable. VECT_3_FLOAT ought to be an array of floats, for instance.
        encoder.write_tag(MANUVR_CBOR_VENDOR_TYPE | TcodeToInt(src->typeCode()));
        encoder.write_bytes((uint8_t*) src->pointer(), src->length());
        break;

      case TCode::IDENTITY:
        {
          Identity* ident;
          if (0 == src->getValueAs(&ident)) {
            uint16_t i_len = ident->length();
            uint8_t buf[i_len];
            if (ident->toBuffer(buf)) {
              encoder.write_tag(MANUVR_CBOR_VENDOR_TYPE | TcodeToInt(src->typeCode()));
              encoder.write_bytes(buf, i_len);
            }
          }
        }
        break;
      //case TCode::ARGUMENT:
      //  {
      //    StringBuilder intermediary;
      //    KeyValuePair *subj;
      //    if (0 == src->getValueAs(&subj)) {
      //      // NOTE: Recursion.
      //      if (KeyValuePair::encodeToCBOR(subj, &intermediary)) {
      //        encoder.write_tag(MANUVR_CBOR_VENDOR_TYPE | TcodeToInt(src->typeCode()));
      //        encoder.write_bytes(intermediary.string(), intermediary.length());
      //      }
      //    }
      //  }
      //  break;
      case TCode::IMAGE:
        #if defined(CONFIG_MANUVR_IMG_SUPPORT)
          {
            Image* img;
            if (0 == src->getValueAs(&img)) {
              uint32_t sz_buf = img->bytesUsed();
              if (sz_buf > 0) {
                uint32_t nb_buf = 0;
                uint8_t* intermediary = (uint8_t*) alloca(32);
                if (0 == img->serializeWithoutBuffer(intermediary, &nb_buf)) {
                  encoder.write_tag(MANUVR_CBOR_VENDOR_TYPE | TcodeToInt(src->typeCode()));
                  encoder.write_bytes(intermediary, nb_buf);   // TODO: This might cause two discrete CBOR objects.
                  encoder.write_bytes(img->buffer(), sz_buf);
                }
              }
            }
          }
        #endif   // CONFIG_MANUVR_IMG_SUPPORT
        break;
      //case TCode::SYS_PIPE_FXN_PTR:
      //case TCode::SYS_ARG_FXN_PTR:
      //case TCode::SYS_MANUVR_XPORT:
      //case TCode::SYS_FXN_PTR:
      //case TCode::SYS_THREAD_FXN_PTR:
      //case TCode::SYS_MANUVRMSG:
      case TCode::RESERVED:
        // Peacefully ignore the types we can't export.
        break;
      default:
        // TODO: Handle pointer types, bool
        break;
    }
    src = src->_next;
  }
  int final_size = output.size();
  if (final_size) {
    out->concat(output.data(), final_size);
  }
  return final_size;
}



KeyValuePair* KeyValuePair::decodeFromCBOR(uint8_t* src, unsigned int len) {
  KeyValuePair* return_value = nullptr;
  CBORArgListener listener(&return_value);
  cbor::input input(src, len);
  cbor::decoder decoder(input, listener);
  decoder.run();
  return return_value;
}
#endif  // CONFIG_MANUVR_CBOR


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* TCode-only constructor. Does no allocation, and assigns length if knowable.
*/
KeyValuePair::KeyValuePair(TCode code) : KeyValuePair() {
  _t_code  = code;
  _alter_flags(_is_type_copy_by_value((const TCode) code), MANUVR_ARG_FLAG_DIRECT_VALUE);
  // If we can know the length with certainty, record it.
  if (typeIsFixedLength(_t_code)) {
    len = sizeOfType(_t_code);
  }
}

/*
* Protected delegate constructor.
*/
KeyValuePair::KeyValuePair(void* ptr, int l, TCode code) : KeyValuePair(code) {
  len        = l;
  target_mem = ptr;
}

KeyValuePair::KeyValuePair(double val) : KeyValuePair(malloc(sizeof(double)), sizeof(double), TCode::DOUBLE) {
  if (nullptr != target_mem) {
    *((double*) target_mem) = val;
    _alter_flags(true, MANUVR_ARG_FLAG_REAP_VALUE);
  }
}


KeyValuePair::~KeyValuePair() {
  wipe();
}


void KeyValuePair::wipe() {
  if (nullptr != _next) {
    KeyValuePair* a = _next;
    _next       = nullptr;
    delete a;
  }
  if (nullptr != target_mem) {
    void* p = target_mem;
    target_mem = nullptr;
    if (reapValue()) free(p);
  }
  if (nullptr != _key) {
    char* k = (char*) _key;
    _key = nullptr;
    if (reapKey()) free(k);
  }
  _t_code    = TCode::NONE;
  len        = 0;
  _flags     = 0;
}


/**
* Given an KeyValuePair pointer, finds that pointer and drops it from the list.
*
* @param  drop  The KeyValuePair to drop.
* @return       0 on success. 1 on warning, -1 on "not found".
*/
int8_t KeyValuePair::dropArg(KeyValuePair** root, KeyValuePair* drop) {
  if (*root == drop) {
    // Re-write the root parameter.
    root = &_next; // NOTE: may be null. Who cares.
    return 0;
  }
  else if (_next && _next == drop) {
    _next = _next->_next; // NOTE: may be null. Who cares.
    return 0;
  }
  return (_next) ? _next->dropArg(root, drop) : -1;
}


/**
* Takes all the keys in the provided argument chain and for any
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
*
* @param  idx      The KeyValuePair position
* @param  trg_buf  A pointer to the place where we should write the result.
* @return 0 on success or appropriate failure code.
*/
int8_t KeyValuePair::getValueAs(uint8_t idx, void* trg_buf) {
  int8_t return_value = -1;
  if (0 < idx) {
    if (nullptr != _next) {
      return_value = _next->getValueAs(--idx, trg_buf);
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
* @param  idx      The KeyValuePair position
* @param  trg_buf  A pointer to the place where we should write the result.
* @return 0 on success or appropriate failure code.
*/
int8_t KeyValuePair::getValueAs(const char* k, void* trg_buf) {
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
      return _next->getValueAs(k, trg_buf);
    }
  }
  return -1;
}


/**
* All of the type-specialized getValueAs() fxns boil down to this. Which is private.
* The boolean preserve parameter will leave the argument attached (if true), or destroy it (if false).
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
      *((uint8_t*) trg_buf) = *((uint8_t*)&target_mem);
      break;
    case TCode::INT16:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT16:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint16_t*) trg_buf) = *((uint16_t*)&target_mem);
      break;
    case TCode::INT32:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT32:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint32_t*) trg_buf) = *((uint32_t*)&target_mem);
      break;
    case TCode::FLOAT:    // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint8_t*) trg_buf + 0) = *(((uint8_t*) &target_mem) + 0);
      *((uint8_t*) trg_buf + 1) = *(((uint8_t*) &target_mem) + 1);
      *((uint8_t*) trg_buf + 2) = *(((uint8_t*) &target_mem) + 2);
      *((uint8_t*) trg_buf + 3) = *(((uint8_t*) &target_mem) + 3);
      break;

    case TCode::DOUBLE:         // These are fixed-length allocated data.
    case TCode::VECT_4_FLOAT:   //
    case TCode::VECT_3_FLOAT:   //
    case TCode::VECT_3_UINT16:
    case TCode::VECT_3_INT16:
      return_value = 0;
      memcpy(trg_buf, target_mem, len);
      break;

    //case TCode::UINT32_PTR:  // These are *pointers* to the indicated types. They
    //case TCode::UINT16_PTR:  //   therefore take the whole 4 bytes of memory allocated
    //case TCode::UINT8_PTR:   //   and can be returned as such.
    //case TCode::INT32_PTR:
    //case TCode::INT16_PTR:
    //case TCode::INT8_PTR:

    case TCode::STR_BUILDER:          // This is a pointer to some StringBuilder. Presumably this is on the heap.
    case TCode::STR:                  // This is a pointer to a string constant. Presumably this is stored in flash.
    case TCode::IMAGE:                // This is a pointer to an Image.
    //case TCode::BUFFERPIPE:           // This is a pointer to a BufferPipe/.
    //case TCode::SYS_MANUVRMSG:        // This is a pointer to ManuvrMsg.
    //case TCode::SYS_EVENTRECEIVER:    // This is a pointer to an EventReceiver.
    //case TCode::SYS_MANUVR_XPORT:     // This is a pointer to a transport.
    default:
      return_value = 0;
      *((uintptr_t*) trg_buf) = *((uintptr_t*)&target_mem);
      break;
  }
  return return_value;
}


/*
* The purpose of this fxn is to pack up this KeyValuePair into something that can be stored or sent
*   over a wire.
* This is the point at which we will have to translate any pointer types into something concrete.
*
* Returns 0 (0) on success.
*/
int8_t KeyValuePair::serialize(StringBuilder *out) {
  unsigned char *temp_str = (unsigned char *) target_mem;   // Just a general use pointer.

  unsigned char *scratchpad = (unsigned char *) alloca(258);  // This is the maximum size for an argument.
  unsigned char *sp_index   = (scratchpad + 2);
  uint8_t arg_bin_len       = len;

  switch (_t_code) {
    /* These are hard types that we can send as-is. */
    case TCode::INT8:
    case TCode::UINT8:   // This frightens the compiler. Its fears are unfounded.
      arg_bin_len = 1;
      *((uint8_t*) sp_index) = *((uint8_t*)& target_mem);
      break;
    case TCode::INT16:
    case TCode::UINT16:   // This frightens the compiler. Its fears are unfounded.
      arg_bin_len = 2;
      *((uint16_t*) sp_index) = *((uint16_t*)& target_mem);
      break;
    case TCode::INT32:
    case TCode::UINT32:   // This frightens the compiler. Its fears are unfounded.
    case TCode::FLOAT:   // This frightens the compiler. Its fears are unfounded.
      arg_bin_len = 4;
      *((uint32_t*) sp_index) = *((uint32_t*)& target_mem);
      break;

    /* These are pointer types to data that can be sent as-is. Remember: LITTLE ENDIAN */
    //case TCode::INT8_PTR:
    //case TCode::UINT8_PTR:
    //  arg_bin_len = 1;
    //  *((uint8_t*) sp_index) = *((uint8_t*) target_mem);
    //  break;
    //case TCode::INT16_PTR:
    //case TCode::UINT16_PTR:
    //  arg_bin_len = 2;
    //  *((uint16_t*) sp_index) = *((uint16_t*) target_mem);
    //  break;
    //case TCode::INT32_PTR:
    //case TCode::UINT32_PTR:
    //case TCode::FLOAT_PTR:
    //  arg_bin_len = 4;
    //  //*((uint32_t*) sp_index) = *((uint32_t*) target_mem);
    //  for (int i = 0; i < 4; i++) {
    //    *((uint8_t*) sp_index + i) = *((uint8_t*) target_mem + i);
    //  }
    //  break;

    /* These are pointer types that require conversion. */
    case TCode::STR_BUILDER:     // This is a pointer to some StringBuilder. Presumably this is on the heap.
    case TCode::URL:             // This is a pointer to some StringBuilder. Presumably this is on the heap.
      temp_str    = ((StringBuilder*) target_mem)->string();
      arg_bin_len = ((StringBuilder*) target_mem)->length();
    case TCode::DOUBLE:
    case TCode::VECT_4_FLOAT:  // NOTE!!! This only works for Vectors because of the template layout. FRAGILE!!!
    case TCode::VECT_3_FLOAT:  // NOTE!!! This only works for Vectors because of the template layout. FRAGILE!!!
    case TCode::VECT_3_UINT16: // NOTE!!! This only works for Vectors because of the template layout. FRAGILE!!!
    case TCode::VECT_3_INT16:  // NOTE!!! This only works for Vectors because of the template layout. FRAGILE!!!
    case TCode::BINARY:
      for (int i = 0; i < arg_bin_len; i++) {
        *(sp_index + i) = *(temp_str + i);
      }
      break;

    /* Anything else should be dropped. */
    default:
      break;
  }

  *(scratchpad + 0) = (uint8_t) _t_code;
  *(scratchpad + 1) = arg_bin_len;
  out->concat(scratchpad, (arg_bin_len+2));
  if (_next) {
    _next->serialize(out);
  }
  return 0;
}


/*
* The purpose of this fxn is to pack up this KeyValuePair into something that can sent over a wire
*   with a minimum of overhead. We write only the bytes that *are* the data, and not the metadata
*   because we are relying on the parser at the other side to know what the type is.
* We still have to translate any pointer types into something concrete.
*
* Returns 0 on success. Also updates the length of data in the offset argument.
*
*/
int8_t KeyValuePair::serialize_raw(StringBuilder *out) {
  if (out == nullptr) return -1;

  switch (_t_code) {
    /* These are hard types that we can send as-is. */
    case TCode::INT8:
    case TCode::UINT8:
      out->concat((unsigned char*) &target_mem, 1);
      break;
    case TCode::INT16:
    case TCode::UINT16:
      out->concat((unsigned char*) &target_mem, 2);
      break;
    case TCode::INT32:
    case TCode::UINT32:
    case TCode::FLOAT:
      out->concat((unsigned char*) &target_mem, 4);
      break;

    /* These are pointer types to data that can be sent as-is. Remember: LITTLE ENDIAN */
    //case TCode::INT8_PTR:
    //case TCode::UINT8_PTR:
    //  out->concat((unsigned char*) *((unsigned char**)target_mem), 1);
    //  break;
    //case TCode::INT16_PTR:
    //case TCode::UINT16_PTR:
    //  out->concat((unsigned char*) *((unsigned char**)target_mem), 2);
    //  break;
    //case TCode::INT32_PTR:
    //case TCode::UINT32_PTR:
    //case TCode::FLOAT_PTR:
    //  out->concat((unsigned char*) *((unsigned char**)target_mem), 4);
    //  break;

    /* These are pointer types that require conversion. */
    case TCode::STR_BUILDER:     // This is a pointer to some StringBuilder. Presumably this is on the heap.
    case TCode::URL:             // This is a pointer to some StringBuilder. Presumably this is on the heap.
      out->concat((StringBuilder*) target_mem);
      break;
    case TCode::STR:
    case TCode::DOUBLE:
    case TCode::VECT_4_FLOAT:
    case TCode::VECT_3_FLOAT:
    case TCode::VECT_3_UINT16:
    case TCode::VECT_3_INT16:
    case TCode::BINARY:     // This is a pointer to a big binary blob.
      out->concat((unsigned char*) target_mem, len);
      break;

    case TCode::IMAGE:      // This is a pointer to an Image.
      #if defined(CONFIG_MANUVR_IMG_SUPPORT)
        {
          Image* img = (Image*) target_mem;
          uint32_t sz_buf = img->bytesUsed();
          if (sz_buf > 0) {
            if (0 != img->serialize(out)) {
              // Failure
            }
          }
        }
      #endif   // CONFIG_MANUVR_IMG_SUPPORT
      break;

    /* Anything else should be dropped. */
    default:
      break;
  }

  if (_next) {
    _next->serialize_raw(out);
  }
  return 0;
}


/**
* @return A pointer to the linked argument.
*/
KeyValuePair* KeyValuePair::link(KeyValuePair* arg) {
  if (nullptr == _next) {
    _next = arg;
    return arg;
  }
  return _next->link(arg);
}


/**
* @return The number of arguments in this list.
*/
int KeyValuePair::argCount() {
  return (1 + ((nullptr == _next) ? 0 : _next->argCount()));
}


/**
* @return [description]
*/
int KeyValuePair::sumAllLengths() {
  return (len + ((nullptr == _next) ? 0 : _next->sumAllLengths()));
}

/**
* @return [description]
*/
KeyValuePair* KeyValuePair::retrieveArgByIdx(unsigned int idx) {
  switch (idx) {
    case 0:
      return this;   // Special case.
    default:
      if (nullptr != _next) {
        return _next->retrieveArgByIdx(idx-1);
      }
      // NOTE: No break
      // Fall-through if the index is greater than the list's cardinality.
    case 1:
      return _next;  // Terminus of recursion for all args but 0.
  }
}


/*
* Does an KeyValuePair in our rank have the given key?
*
* Returns nullptr if the answer is 'no'. Otherwise, ptr to the first matching key.
*/
KeyValuePair* KeyValuePair::retrieveArgByKey(const char* k) {
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
      return _next->retrieveArgByKey(k);
    }
  }
  return nullptr;
}


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
    case TCode::VECT_3_FLOAT:
      {
        Vector3<float>* v = (Vector3<float>*) pointer();
        out->concatf("(%.4f, %.4f, %.4f)", (double)(v->x), (double)(v->y), (double)(v->z));
      }
      break;
    case TCode::VECT_4_FLOAT:
      {
        Vector4f* v = (Vector4f*) pointer();
        out->concatf("(%.4f, %.4f, %.4f, %.4f)", (double)(v->w), (double)(v->x), (double)(v->y), (double)(v->z));
      }
      break;
    default:
      {
        int l_ender = (len < 16) ? len : 16;
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
  out->concatf("\t%s\t%s\t%s",
    (nullptr == _key ? "" : _key),
    getTypeCodeString((TCode) typeCode()),
    (reapValue() ? "(reap)" : "\t")
  );
  valToString(out);
  out->concat("\n");

  if (nullptr != _next) _next->printDebug(out);
}


int8_t KeyValuePair::setValue(void* trg_buf, int len, TCode tc) {
  int8_t return_value = -1;
  if (typeCode() != tc) {
    return -2;
  }
  switch (tc) {
    case TCode::INT8:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT8:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint8_t*)&target_mem) = *((uint8_t*) trg_buf);
      break;
    case TCode::INT16:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT16:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint16_t*)&target_mem) = *((uint16_t*) trg_buf);
      break;
    case TCode::INT32:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT32:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint32_t*)&target_mem) = *((uint32_t*) trg_buf);
      break;
    case TCode::FLOAT:    // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *(((uint8_t*) &target_mem) + 0) = *((uint8_t*) trg_buf + 0);
      *(((uint8_t*) &target_mem) + 1) = *((uint8_t*) trg_buf + 1);
      *(((uint8_t*) &target_mem) + 2) = *((uint8_t*) trg_buf + 2);
      *(((uint8_t*) &target_mem) + 3) = *((uint8_t*) trg_buf + 3);
      break;
    //case TCode::UINT32_PTR:  // These are *pointers* to the indicated types. They
    //case TCode::UINT16_PTR:  //   therefore take the whole 4 bytes of memory allocated
    //case TCode::UINT8_PTR:   //   and can be returned as such.
    //case TCode::INT32_PTR:
    //case TCode::INT16_PTR:
    //case TCode::INT8_PTR:
    //case TCode::FLOAT_PTR:
    case TCode::DOUBLE:
    case TCode::VECT_4_FLOAT:
    case TCode::VECT_3_FLOAT:
    case TCode::VECT_3_UINT16:
    case TCode::VECT_3_INT16:
      return_value = 0;
      for (int i = 0; i < len; i++) {
        *((uint8_t*) target_mem + i) = *((uint8_t*) trg_buf + i);
      }
      break;

    case TCode::STR_BUILDER:          // This is a pointer to some StringBuilder. Presumably this is on the heap.
    case TCode::STR:                  // This is a pointer to a string constant. Presumably this is stored in flash.
    case TCode::IMAGE:                // This is a pointer to an Image.
    //case TCode::BUFFERPIPE:           // This is a pointer to a BufferPipe/.
    //case TCode::SYS_MANUVRMSG:        // This is a pointer to ManuvrMsg.
    //case TCode::SYS_EVENTRECEIVER:    // This is a pointer to an EventReceiver.
    //case TCode::SYS_MANUVR_XPORT:     // This is a pointer to a transport.
    default:
      return_value = 0;
      target_mem = trg_buf;  // TODO: Need to do an allocation check and possible cleanup.
      break;
  }
  return return_value;
}



#if defined(CONFIG_MANUVR_CBOR)
CBORArgListener::CBORArgListener(KeyValuePair** target) {    built = target;    }

CBORArgListener::~CBORArgListener() {
  // JIC...
  if (nullptr != _wait) {
    free(_wait);
    _wait = nullptr;
  }
}

void CBORArgListener::_caaa(KeyValuePair* nu) {
  if (nullptr != _wait) {
    nu->setKey(_wait);
    _wait = nullptr;
    nu->reapKey(true);
  }

  if ((nullptr != built) && (nullptr != *built)) {
    (*built)->link(nu);
  }
  else {
    *built = nu;
  }

  if (0 < _wait_map)   _wait_map--;
  if (0 < _wait_array) _wait_array--;
}


void CBORArgListener::on_string(char* val) {
  if (nullptr == _wait) {
    // We need to copy the string. It will be the key for the KeyValuePair
    //   who's value is forthcoming.
    int len = strlen(val);
    _wait = (char*) malloc(len+1);
    if (nullptr != _wait) {
      memcpy(_wait, val, len+1);
    }
  }
  else {
    // There is a key assignment waiting. This must be the value.
    _caaa(new KeyValuePair(val));
  }
};

void CBORArgListener::on_bytes(uint8_t* data, int size) {
  if (TCode::NONE != _pending_manuvr_tag) {
    // If we've seen our vendor code in a tag, we interpret the first byte as a Manuvr
    //   Typecode, and build an KeyValuePair the hard way.
    const TypeCodeDef* const m_type_def = getManuvrTypeDef(_pending_manuvr_tag) ;
    if (m_type_def) {
      if (m_type_def->fixed_len) {
        if (size == (m_type_def->fixed_len)) {
          _caaa(new KeyValuePair(data, (m_type_def->fixed_len), m_type_def->type_code));
        }
      }
      else {
        // We will have to pass validation to the KeyValuePair class.
        _caaa(new KeyValuePair((data+1), (size-1), m_type_def->type_code));
      }
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

// NOTE: IANA gives of _some_ guidance....
// https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml
void CBORArgListener::on_tag(unsigned int tag) {
  switch (tag & 0xFFFFFF00) {
    case MANUVR_CBOR_VENDOR_TYPE:
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

#endif // CONFIG_MANUVR_CBOR

#endif // false
