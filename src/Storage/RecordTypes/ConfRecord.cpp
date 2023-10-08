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

int8_t ConfRecord::_get_conf(const char* key, const TCode TC_ARG, void* val) {
  int8_t ret = -1;
  const TCode TC_KEY = _key_tcode(key);
  return ret;
}


/**
* Sets a value for the given ConfKey.
*
* @param key is the enum-controlled conf key to set.
* @param val is a pointer to the string containing the data to be parsed.
* @return 0 on success, -1 for invalid key, -2 for type mismatch, -3 for unhandled type.
*/
int8_t ConfRecord::_set_conf(const char* key, const TCode TC_ARG, void* val) {
  int8_t ret = -1;
  const TCode TC_KEY = _key_tcode(key);
  switch ((TCode) TC_KEY) {
      case TCode::BOOLEAN:
        {
          //bool tmp_val = (0 != val_obj.position_as_int(0));
        }
        break;

      case TCode::INT8:
        {
          //int8_t tmp_val = (int8_t) val_obj.position_as_int(0);
        }
        break;

      case TCode::UINT64:
        {
          //uint64_t tmp_val = val_obj.position_as_uint64(0);
        }
        break;

      case TCode::INT32:
        {
          //int tmp_val = val_obj.position_as_int(0);
        }
        break;

      case TCode::UINT8:
        {
          //uint8_t tmp_val = (uint8_t) val_obj.position_as_int(0);
        }
        break;

      case TCode::UINT16:
        {
          //uint16_t tmp_val = (uint16_t) val_obj.position_as_int(0);
        }
        break;

      case TCode::UINT32:
        {
          //uint32_t tmp_val = val_obj.position_as_int(0);
        }
        break;

      case TCode::FLOAT:
        {
          //float tmp_val = val_obj.position_as_double(0);
        }
        break;

      case TCode::STR:
        {
          //char* tmp_val = val_obj.position_trimmed(0);
        }
        break;

      case TCode::VECT_3_FLOAT:
        //if (3 == val_obj.split(",")) {
        //  float x = val_obj.position_as_double(0);
        //  float y = val_obj.position_as_double(1);
        //  float z = val_obj.position_as_double(2);
        //}
        break;
    default:   // Unhandled type.
      ret = -3;
      break;
  }
  return ret;
}



/*
* Print the named conf key to the given buffer.
*/
void ConfRecord::printConfRecord(StringBuilder* output, const char* spec_key) {
  if (0 == _allocated()) {
    if (nullptr != spec_key) {
      int8_t ret = 0;
      KeyValuePair* obj = _kvp->retrieveByKey(spec_key);
      if (nullptr != obj) {
        StringBuilder tmp;
        tmp.concatf("%24s (%s)\t= ", obj->getKey(), typecodeToStr(obj->typeCode()));
        obj->valToString(&tmp);
        tmp.concat("\n");
        tmp.string();
        output->concatHandoff(&tmp);
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
        printConfRecord(output, key_export.position(0));  // Single-depth recursion.
        key_export.drop_position(0);
      }
    }
  }
  else {
    output->concat("\tRecord is not ready for use.\n");
  }
}



/*
* This checks the object's local KVP against what the enum implies, and sets it
*   according to plan. If the local memory is unallocated, try to do so.
*
* @return 0 on success, or the differential in items that need allocation on failure.
*/
int32_t ConfRecord::_allocated() {
  uint32_t alloc_count = 0;
  StringBuilder key_export;
  _key_list(&key_export);
  const uint32_t EXPORT_COUNT_CHECK = _key_count();
  while (0 < key_export.count()) {
    // NOTE: Do not use a const char* here, considering that the values are
    //   mutable. KeyValuePair actually cares.
    char* key_str = key_export.position(0);
    if (nullptr == _kvp) {
      _kvp = new KeyValuePair((uint8_t) 0);
      if (nullptr != _kvp) {
        alloc_count++;
        _kvp->setKey(key_str);
      }
    }
    else {
      KeyValuePair* _tmp = _kvp->retrieveByKey(key_str);
      if (nullptr == _tmp) {
        _tmp = _kvp->append((uint8_t) 0);
        if (nullptr != _tmp) {
          alloc_count++;
          _tmp->setKey(key_str);
        }
      }
      else {
        // The key was already found in the data. Make sure that it is properly
        //   type-constrained.
        alloc_count++;
        // TODO: This is probably better done by the KVP API, but it doesn't
        //   presently have a flexible-enough parser/packer to cope with this
        //   task. It needs to store TCodes in its map to make the code below
        //   unnecessary.
        const TCode CONSTRAINED_TCODE = _key_tcode(key_str);
        if (_tmp->typeCode() != CONSTRAINED_TCODE) {
          // Convert the existing data to the constrained limits.
          _tmp->convertToType(CONSTRAINED_TCODE);
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
  if (0 != _allocated()) {
    return ret;  // TODO: Return contract spec'd where?
  }
  switch (format) {
    case TCode::CBOR:
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
  if (0 != _allocated()) {
    return ret;  // TODO: Return contract spec'd where?
  }
  switch (format) {
    case TCode::CBOR:
      {
        CBORArgListener cl(&_kvp);
        cbor::input input(raw->string(), raw->length());
        cbor::decoder decoder(input, cl);
        decoder.run();
        ret = (decoder.failed() ? -2 : 0);
      }
      break;

    case TCode::JSON:     // TODO
    case TCode::BINARY:   // TODO: Unsupported. Perhaps forever.
    default:
      break;
  }
  return ret;  // TODO: Return contract spec'd where?
}
