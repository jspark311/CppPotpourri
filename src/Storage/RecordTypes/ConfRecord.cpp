/*
* File:   ConfRecord.cpp
* Author: J. Ian Lindsay
* Date:   2023.10.04
*
* This is a generic configuration record (which many programs implement in some
*   way or another). This class can be used naked (intent), or extended to
*   support configuration handling in a specific or differential manner, if
*   needed. See the examples and unit_tests directories for guidance.
*
* This record type is implemented as a template against whatever set of
*   enumerated values are stored in the EnumDefList.
*/

#include "ConfRecord.h"


/*******************************************************************************
* ConfRecord
*******************************************************************************/

/*
* Destructor will free any allocated KVP.
*/
ConfRecord::~ConfRecord() {
  _discard_allocations();
}


/*
* Free's any memory associated with this ConfRecord.
*/
int8_t ConfRecord::_discard_allocations() {
  _config_is_complete = false;
  if (nullptr != _kvp) {
    delete _kvp;
    _kvp = nullptr;
  }
  return 0;
}


/*******************************************************************************
* Value accessors
*******************************************************************************/

/**
* Gets a value for the given ConfKey.
*
* @param KEY is the enum-controlled conf key to set.
* @param TC_ARG is the type code for the value pointer.
* @param dest is a pointer for the value.
* @return 0 on success, -1 for incomplete allocation, -2 key not found, -3 for type conversion failure.
*/
int8_t ConfRecord::_get_conf(const char* KEY, const TCode TC_ARG, void* dest) {
  int8_t ret = -1;
  if (allocated(true)) {
    ret--;
    //const TCode TC_KEY = _key_tcode(KEY);
    C3PValue* container_of_interest = _kvp->valueWithKey(KEY);
    if (nullptr != container_of_interest) {
      ret--;
      if (0 == container_of_interest->get_as(TC_ARG, dest)) {
        ret = 0;
      }
    }
  }
  return ret;
}


/**
* Gets a value for the given ConfKey.
*
* @param KEY is the enum-controlled conf key to set.
* @param TC_ARG is the type code for the value pointer.
* @param dest_ptr is a pointer for the memory to receive the value.
* @param dest_len is the length of the value.
* @return 0 on success, -1 for incomplete allocation, -2 key not found, -3 for type conversion failure.
*/
int8_t ConfRecord::_get_conf(const char* KEY, const TCode TC_ARG, uint8_t** dest_ptr, uint32_t* dest_len) {
  int8_t ret = -1;
  if (allocated(true)) {
    ret--;
    //const TCode TC_KEY = _key_tcode(KEY);
    C3PValue* container_of_interest = _kvp->valueWithKey(KEY);
    if (nullptr != container_of_interest) {
      ret--;
      if (0 == container_of_interest->get_as(dest_ptr, dest_len)) {
        ret = 0;
      }
    }
  }
  return ret;
}


/**
* Sets a value for the given ConfKey.
*
* @param KEY is the enum-controlled conf key to set.
* @param TC_ARG is the type code for the value pointer.
* @param src is a pointer for the value.
* @return 0 on success, -1 for incomplete allocation, -2 key not found, -3 for type conversion failure.
*/
int8_t ConfRecord::_set_conf(const char* KEY, const TCode TC_ARG, void* src) {
  int8_t ret = -1;
  if (allocated(true)) {
    ret--;
    //const TCode TC_KEY = _key_tcode(KEY);
    C3PValue* container_of_interest = _kvp->valueWithKey(KEY);
    if (nullptr != container_of_interest) {
      ret--;
      if (0 == container_of_interest->set_from(TC_ARG, src)) {
        ret = 0;
      }
    }
  }
  return ret;
}



/*
* Print the named conf key to the given buffer.
*/
void ConfRecord::printConfRecord(StringBuilder* output, const char* spec_key) {
  if (allocated()) {
    if (nullptr != spec_key) {
      KeyValuePair* obj = _kvp->retrieveByKey(spec_key);
      if (nullptr != obj) {
        char* current_key = obj->getKey();
        if (nullptr != current_key) {
          StringBuilder tmp;
          tmp.concatf("%24s (%s)\t= ", current_key, typecodeToStr(obj->tcode()));
          obj->valToString(&tmp);
          tmp.concat("\n");
          output->concatHandoff(&tmp);
        }
      }
    }
    else {   // Dump all records.
      StringBuilder::styleHeader2(output, _list_name());
      StringBuilder key_export;
      uint32_t key_sizes = 0;
      uint32_t dat_sizes = 0;
      uint32_t tot_sizes = _data_size(&key_sizes, &dat_sizes);
      _key_list(&key_export);
      output->concatf("\tStorage requirement:  %u bytes (%u for keys) (%u for values)\n", tot_sizes, key_sizes, dat_sizes);
      output->concatf("\tKey count:            %u\n", key_export.count());
      while (0 < key_export.count()) {
        char* current_key = key_export.position(0);
        if (nullptr != current_key) {
          printConfRecord(output, current_key);  // Single-depth recursion.
        }
        key_export.drop_position(0);
      }
    }
  }
  else {
    output->concat("\tRecord is not ready for use.\n");
  }
}


KeyValuePair* ConfRecord::getKVP() {
  if (allocated(true)) {
    return _kvp;
  }
  return nullptr;
}


/*******************************************************************************
* Memory management
*******************************************************************************/

/**
* This is a fast check of the enum plan against the KVP memory conditions. If
*   the force parameter is true, the check will also attempt allocation of any
*   missing keys, and the type-coercion of any values under existing keys named
*   in the plan.
*
* TODO: Might-should break coercion of existing keys into its own function, and
*   run it regardless of existing allocation condition.
*
* @param force_allocate will attempt allocation if it isn't already satisfied.
* @return 0 on success, or the differential in items that need allocation on failure.
*/
bool ConfRecord::allocated(bool force_allocate) {
  bool ret = ((nullptr != _kvp) && (_key_count() == _kvp->count()));
  if (!ret & force_allocate) {
    ret = (0 == _allocate_kvp());
  }
  return ret;
}


/**
* This checks the object's local KVP against what the enum implies, and sets it
*   according to plan. If the local memory is unallocated, try to do so.
*
* @return 0 on success, or the differential in items that need allocation on failure.
*/
int32_t ConfRecord::_allocate_kvp() {
  uint32_t alloc_count = 0;
  StringBuilder key_export;
  _key_list(&key_export);
  const uint32_t EXPORT_COUNT_CHECK = _key_count();
  while (0 < key_export.count()) {
    // NOTE: Do not use a const char* here, considering that the values are
    //   mutable. KeyValuePair actually cares.
    char* key_str = key_export.position(0);
    const TCode CONSTRAINED_TCODE = _key_tcode(key_str);
    if (nullptr == _kvp) {
      // First key, and the KVP doesn't exist. Create it...
      C3PValue* tmp_container = new C3PValue(CONSTRAINED_TCODE);
      if (nullptr != tmp_container) {
        _kvp = new KeyValuePair("", tmp_container, (C3P_KVP_FLAG_REAP_KVP | C3P_KVP_FLAG_REAP_CNTNR));
        if (nullptr != _kvp) {
          _kvp->setKey(key_str);
          alloc_count++;
        }
        else {
          delete tmp_container;
        }
      }
    }
    else {
      KeyValuePair* tmp = _kvp->retrieveByKey(key_str);
      if (nullptr == tmp) {
        tmp = new KeyValuePair(key_str, CONSTRAINED_TCODE, (C3P_KVP_FLAG_REAP_KVP));
        if (nullptr != tmp) {
          tmp = _kvp->link(tmp);
          alloc_count++;
        }
      }
      else {
        // The key was already found in the data. Make sure that it is properly
        //   type-constrained. If the types match, this call will do nothing and
        //   return success.
        if (0 == tmp->convertToType(CONSTRAINED_TCODE)) {
          alloc_count++;
        }
      }
    }
    key_export.drop_position(0);
  }

  return (alloc_count - EXPORT_COUNT_CHECK);
}




/*******************************************************************************
* Parsing and packing
*******************************************************************************/

int8_t ConfRecord::serialize(StringBuilder* out, TCode format) {
  int8_t ret = -1;
  if (!allocated()) {
    return ret;  // TODO: Return contract spec'd where?
  }
  switch (format) {
    case TCode::CBOR:
      #if defined(__BUILD_HAS_CBOR)
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_map(2);
          serialize_cbor_kvp_for_record(&encoder);  // Accounts for the first KVP.
          encoder.write_string(_list_name());    // Accounts for the second KVP.
          encoder.write_array(_key_count());
          _kvp->serialize(out, format);

        ret = 0;  // TODO: Error handling?
      }
      #endif
      break;

    case TCode::JSON:     // TODO
    case TCode::BINARY:   // TODO: Unsupported. Perhaps forever.
    default:
      break;
  }
  return ret;  // TODO: Return contract spec'd where?
}


int8_t ConfRecord::deserialize(StringBuilder* raw, TCode format) {
  int8_t ret = -1;
  if (!allocated()) {
    return ret;  // TODO: Return contract spec'd where?
  }
  switch (format) {
    case TCode::CBOR:
      #if defined(__BUILD_HAS_CBOR)
      {
        CBORArgListener cl(&_kvp);
        cbor::input_static input(raw->string(), raw->length());
        cbor::decoder decoder(input, cl);
        decoder.run();
        ret = (decoder.failed() ? -2 : 0);
      }
      #endif
      break;

    case TCode::JSON:     // TODO
    case TCode::BINARY:   // TODO: Unsupported. Perhaps forever.
    default:
      break;
  }
  return ret;  // TODO: Return contract spec'd where?
}
