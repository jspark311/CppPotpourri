/*
* File:   ConfRecord.h
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

#include "StringBuilder.h"
#include "EnumWrapper.h"
#include "../Storage.h"

#ifndef __C3P_DATAREC_CONFRECORD_H
#define __C3P_DATAREC_CONFRECORD_H

/*
* ConfRecord is the base class for all records that pertain to runtime
*   configuration of the program.
*/
template <class T> class ConfRecord : public SimpleDataRecord {
  public:
    ConfRecord(uint32_t storage_tag, const EnumDefList<T>* const KDEFS) :
      SimpleDataRecord(storage_tag, StorageRecordType::CONFIG_OBJ),
      _KEY_DEFS(KDEFS), _kvp(nullptr) {};
    ~ConfRecord() {};

    int8_t setConf(T, const char*);
    int8_t setConf(T, uint8_t*,  uint32_t  len);
    int8_t getConf(T, uint8_t**, uint32_t* len);
    int8_t getConf(T, uint8_t*);
    int8_t getConf(T, int*);
    int8_t getConf(T, int64_t*);
    int8_t getConf(T, uint32_t*);
    int8_t getConf(T, uint64_t*);
    int8_t getConf(T, Vector3f*);
    int8_t getConf(T, float*);
    int8_t getConf(T, bool*);

    /* Convenience inlines. TODO: This might be a use-case for return type 'auto'. */
    inline int8_t setConf(const char* key, const char* val) {   return setConf(getConfKeyByString(key), val);  };
    inline int8_t getConf(const char* key, uint8_t* val) {      return getConf(getConfKeyByString(key), val);  };
    inline int8_t getConf(const char* key, int* val) {          return getConf(getConfKeyByString(key), val);  };
    inline int8_t getConf(const char* key, int64_t* val) {      return getConf(getConfKeyByString(key), val);  };
    inline int8_t getConf(const char* key, uint32_t* val) {     return getConf(getConfKeyByString(key), val);  };
    inline int8_t getConf(const char* key, uint64_t* val) {     return getConf(getConfKeyByString(key), val);  };
    inline int8_t getConf(const char* key, Vector3f* val) {     return getConf(getConfKeyByString(key), val);  };
    inline int8_t getConf(const char* key, float* val) {        return getConf(getConfKeyByString(key), val);  };
    inline int8_t getConf(const char* key, bool* val) {         return getConf(getConfKeyByString(key), val);  };
    inline T getConfKeyByString(const char* KEY_STR) {          return _KEY_DEFS->getEnumByStr(KEY_STR);       };

    void printConf(StringBuilder*, const char* key = nullptr);

    /* Obligate overrides from DataRecord. */
    int8_t serialize(StringBuilder*, TCode);
    int8_t deserialize(StringBuilder*, TCode);


  protected:
    const EnumDefList<T>* const _KEY_DEFS;   // A list of possible value keys, and their types.
    KeyValuePair* _kvp;

    int serialize_cbor_kvp_for_conf(cbor::encoder*);
};




/*******************************************************************************
* ConfRecord implementation
* TODO: Some of this should be migrated out of the template when practical for
*   reasons pertaining to build-size.
*******************************************************************************/
/**
* Sets a value for the given ConfKey.
*
* @param key is the enum-controlled conf key to set.
* @param val is a pointer to the string containing the data to be parsed.
* @return 0 on success, -1 for invalid key, -2 for type mismatch, -3 for unhandled type.
*/
template <class T> int8_t ConfRecord<T>::setConf(T key, const char* val) {
  int8_t ret = -1;
  const EnumDef<T>* const C_DEF = _KEY_DEFS->enumDef(key);
  if (nullptr != C_DEF) {
    StringBuilder val_obj(val);
    switch ((TCode) C_DEF->CONTEXT) {
      case TCode::BOOLEAN:
        {
          bool tmp_val = (0 != val_obj.position_as_int(0));
        }
        break;

      case TCode::INT8:
        {
          int8_t tmp_val = (int8_t) val_obj.position_as_int(0);
        }
        break;

      case TCode::UINT64:
        {
          uint64_t tmp_val = val_obj.position_as_uint64(0);
        }
        break;

      case TCode::INT32:
        {
          int tmp_val = val_obj.position_as_int(0);
        }
        break;

      case TCode::UINT8:
        {
          uint8_t tmp_val = (uint8_t) val_obj.position_as_int(0);
        }
        break;

      case TCode::UINT16:
        {
          uint16_t tmp_val = (uint16_t) val_obj.position_as_int(0);
        }
        break;

      case TCode::UINT32:
        {
          uint32_t tmp_val = val_obj.position_as_int(0);
        }
        break;

      case TCode::FLOAT:
        {
          float tmp_val = val_obj.position_as_double(0);
        }
        break;

      case TCode::STR:
        {
          char* tmp_val = val_obj.position_trimmed(0);
        }
        break;

      case TCode::VECT_3_FLOAT:
        if (3 == val_obj.split(",")) {
          float x = val_obj.position_as_double(0);
          float y = val_obj.position_as_double(1);
          float z = val_obj.position_as_double(2);
        }
        break;
      default:   // Unhandled type.
        ret = -3;
        break;
    }
  }
  return ret;
}


/**
* Sets a value for the given ConfKey.
*
* @param key is the enum-controlled conf key to set.
* @param val is a pointer to the buffer containing the data.
* @param buf_len is the length of that data.
* @return int8_t 0 if successful, -1 if incorrect type, -2 if unhandled ConfKey
*/
template <class T> int8_t ConfRecord<T>::setConf(T key, uint8_t* val, uint32_t buf_len) {
  int8_t ret = -1;
  const EnumDef<T>* const C_DEF = _KEY_DEFS->enumDef(key);
  if (nullptr != C_DEF) {
    switch ((TCode) C_DEF->CONTEXT) {
      case TCode::BINARY:
        break;

      default:   // Unhandled type.
        break;
    }
  }
  return ret;
}


/**
* Gets byte array for the given ConfKey.
*
* @param key is the enum-controlled conf key to fetch.
* @param val is the pointer to the variable to receive the result.
* @param len is the pointer to the variable to receive the length of the data.
* @return 0 on success, -1 for invalid key, -2 for data deprecation if converted.
*/
template <class T> int8_t ConfRecord<T>::getConf(T key, uint8_t** val, uint32_t* len) {
  int8_t ret = 0;
  return ret;
}


/**
* Gets a value for the given ConfKey.
*
* @param key is the enum-controlled conf key to fetch.
* @param val is the pointer to the variable to receive the result.
* @return 0 on success, -1 for invalid key, -2 for data deprecation if converted.
*/
template <class T> int8_t ConfRecord<T>::getConf(T key, uint8_t* val) {
  int8_t ret = 0;
  return ret;
}


/**
* Gets a value for the given ConfKey.
*
* @param key is the enum-controlled conf key to fetch.
* @param val is the pointer to the variable to receive the result.
* @return 0 on success, -1 for invalid key, -2 for data deprecation if converted.
*/
template <class T> int8_t ConfRecord<T>::getConf(T key, int* val) {
  int8_t ret = 0;
  return ret;
}


/**
* Gets a value for the given ConfKey.
*
* @param key is the enum-controlled conf key to fetch.
* @param val is the pointer to the variable to receive the result.
* @return 0 on success, -1 for invalid key, -2 for data deprecation if converted.
*/
template <class T> int8_t ConfRecord<T>::getConf(T key, int64_t* val) {
  int8_t ret = 0;
  return ret;
}


/**
* Gets a value for the given ConfKey.
*
* @param key is the enum-controlled conf key to fetch.
* @param val is the pointer to the variable to receive the result.
* @return 0 on success, -1 for invalid key, -2 for data deprecation if converted.
*/
template <class T> int8_t ConfRecord<T>::getConf(T key, uint32_t* val) {
  int8_t ret = 0;
  return ret;
}


/**
* Gets a value for the given ConfKey.
*
* @param key is the enum-controlled conf key to fetch.
* @param val is the pointer to the variable to receive the result.
* @return 0 on success, -1 for invalid key, -2 for data deprecation if converted.
*/
template <class T> int8_t ConfRecord<T>::getConf(T key, uint64_t* val) {
  int8_t ret = 0;
  return ret;
}


/**
* Gets a value for the given ConfKey.
*
* @param key is the enum-controlled conf key to fetch.
* @param val is the pointer to the variable to receive the result.
* @return 0 on success, -1 for invalid key, -2 for data deprecation if converted.
*/
template <class T> int8_t ConfRecord<T>::getConf(T key, Vector3f* val) {
  int8_t ret = 0;
  return ret;
}


/**
* Gets a value for the given ConfKey.
*
* @param key is the enum-controlled conf key to fetch.
* @param val is the pointer to the variable to receive the result.
* @return 0 on success, -1 for invalid key, -2 for data deprecation if converted.
*/
template <class T> int8_t ConfRecord<T>::getConf(T key, float* val) {
  int8_t ret = 0;
  return ret;
}


/**
* Gets a value for the given ConfKey.
*
* @param key is the enum-controlled conf key to fetch.
* @param val is the pointer to the variable to receive the result.
* @return 0 on success, -1 for invalid key, -2 for data deprecation if converted.
*/
template <class T> int8_t ConfRecord<T>::getConf(T key, bool* val) {
  int8_t ret = 0;
  return ret;
}



/*
* Print the named conf key to the given buffer.
*/
template <class T> void ConfRecord<T>::printConf(StringBuilder* output, const char* spec_key) {
  if (nullptr != spec_key) {
    int8_t ret = 0;
    const EnumDef<T>* const C_DEF = _KEY_DEFS->getEnumDefByStr(spec_key);
    if (nullptr != C_DEF) {
      const T KY = C_DEF->VAL;
      output->concatf("%24s (0x%02x) = ", C_DEF->STR, C_DEF->CONTEXT);
      switch ((TCode) C_DEF->CONTEXT) {
        case TCode::INT64:
          {
            int64_t val = 0;
            ret = getConf(KY, &val);
            if (0 == ret) {
              //output->concatf("%24s = %" PRIi64 "\n", spec_key, val);
              output->concatf("0x%08x%08x\n", ((uint32_t) (val >> 32)), ((uint32_t) (val & 0xFFFFFFFFULL)));
            }
          }
          break;
        case TCode::UINT64:
          {
            uint64_t val = 0;
            ret = getConf(KY, &val);
            if (0 == ret) {
              //output->concatf("%24s = %" PRIu64 "\n", spec_key, val);
              output->concatf("0x%08x%08x\n", ((uint32_t) (val >> 32)), ((uint32_t) (val & 0xFFFFFFFFULL)));
            }
          }
          break;
        case TCode::INT8:
        case TCode::INT16:
        case TCode::INT32:
          {
            int val = 0;
            ret = getConf(KY, &val);
            if (0 == ret) {
              output->concatf("%d\n", val);
            }
          }
          break;
        case TCode::UINT8:
        case TCode::UINT16:
        case TCode::UINT32:
          {
            uint32_t val = 0;
            ret = getConf(KY, &val);
            if (0 == ret) {
              output->concatf("%u\n", val);
            }
          }
          break;
        case TCode::BOOLEAN:
          {
            bool val = false;
            ret = getConf(KY, &val);
            if (0 == ret) {
              output->concatf("%c\n", val?'Y':'N');
            }
          }
          break;
        case TCode::FLOAT:
          {
            float val = 0.0;
            ret = getConf(KY, &val);
            if (0 == ret) {
              output->concatf("%.6f\n", (double) val);
            }
          }
          break;
        case TCode::STR:
          {
            uint8_t* val = nullptr;
            uint32_t len = 0;
            ret = getConf(KY, &val, &len);
            if (0 == ret) {
              output->concatf("%s\n", (char*) val);
            }
          }
          break;
        case TCode::VECT_3_FLOAT:
          {
            Vector3f val;
            ret = getConf(KY, &val);
            if (0 == ret) {
              output->concatf("(%.3f, %.3f, %.3f)\n", (double) val.x, (double) val.y, (double) val.z);
            }
          }
          break;
        case TCode::BINARY:
          {
            uint8_t* ptr = nullptr;
            uint32_t len = 0;
            ret = getConf(KY, &ptr, &len);
            output->concatf("Binary string: %p (%u bytes)\n", ptr, len);
          }
          break;
        default:
          output->concat("<Unknown type>.\n");
          break;
      }
      if (0 != ret) {
        output->concat("<Failed to get value>\n");
      }
    }
  }
  else {   // Dump all records.
    output->concat("----------------------------------------------\n");
    for (uint32_t i = 0; i < _KEY_DEFS->COUNT; i++) {
      if (!(_KEY_DEFS->LIST_PTR[i].FLAGS & ENUM_WRAPPER_FLAG_CATCHALL)) {
        printConf(output, _KEY_DEFS->LIST_PTR[i].STR);  // Single-depth recursion.
      }
    }
  }
}



/*******************************************************************************
* Parsing and packing
*******************************************************************************/


template <class T> int8_t ConfRecord<T>::serialize(StringBuilder* out, TCode format) {
  int8_t ret = -1;
  switch (format) {
    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_map(2);
          serialize_cbor_kvp_for_record(&encoder);  // Accounts for the first KVP.
          serialize_cbor_kvp_for_conf(&encoder);    // Accounts for the second KVP.
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


template <class T> int8_t ConfRecord<T>::deserialize(StringBuilder* raw, TCode format) {
  int8_t ret = -1;
  switch (format) {
    case TCode::CBOR:
      {
        CBORArgListener cl(&_kvp);
        cbor::input input(raw->string(), raw->length());
        cbor::decoder decoder(input, cl);
        decoder.run();
        ret = (cl.failed() ? -2 : 0);
      }
      break;

    case TCode::JSON:     // TODO
    case TCode::BINARY:   // TODO: Unsupported. Perhaps forever.
    default:
      break;
  }
  return ret;  // TODO: Return contract spec'd where?
}


/*
* If the given configuration key does not exist, returns TCode::NONE.
*/
template <class T> int ConfRecord<T>::serialize_cbor_kvp_for_conf(cbor::encoder* encoder) {
  uint32_t count = 0;
  int8_t ret = 0;
  for (uint32_t i = 0; i < _KEY_DEFS->COUNT; i++) {
    // Ignore any catch-alls. They are commonly INVALID bumpers.
    if (!(_KEY_DEFS->LIST_PTR[i].FLAGS & ENUM_WRAPPER_FLAG_CATCHALL)) {
      count++;
    }
  }
  encoder->write_string(_KEY_DEFS->LIST_NAME);
  encoder->write_map(count);

  for (uint32_t i = 0; i < _KEY_DEFS->COUNT; i++) {
    if (!(_KEY_DEFS->LIST_PTR[i].FLAGS & ENUM_WRAPPER_FLAG_CATCHALL)) {
      encoder->write_string(_KEY_DEFS->LIST_PTR[i].STR);
      switch ((TCode) _KEY_DEFS->LIST_PTR[i].CONTEXT) {
        case TCode::STR:
          {
            char* val = nullptr;
            uint32_t len = 0;
            ret = getConf(_KEY_DEFS->LIST_PTR[i].VAL, (uint8_t**) &val, &len);
            if (val) {
              encoder->write_string(val);
            }
          }
          break;

        case TCode::INT8:
        case TCode::INT16:
        case TCode::INT32:
          {
            int32_t val;
            ret = getConf(_KEY_DEFS->LIST_PTR[i].VAL, &val);
            encoder->write_int(val);
          }
          break;
        case TCode::INT64:
          {
            int64_t val = 0;
            ret = getConf(_KEY_DEFS->LIST_PTR[i].VAL, &val);
            encoder->write_int(val);
          }
          break;

        case TCode::UINT8:
        case TCode::UINT16:
        case TCode::UINT32:
          {
            uint32_t val = 0;
            ret = getConf(_KEY_DEFS->LIST_PTR[i].VAL, &val);
            encoder->write_int((unsigned int) val);
          }
          break;
        case TCode::UINT64:
          {
            uint64_t val = 0;
            ret = getConf(_KEY_DEFS->LIST_PTR[i].VAL, &val);
            encoder->write_int(val);
          }
          break;

        case TCode::BOOLEAN:
          {
            bool val = false;
            ret = getConf(_KEY_DEFS->LIST_PTR[i].VAL, &val);
            encoder->write_int(val ? 1:0);
          }
          break;
        case TCode::FLOAT:
          {
            float val = 0.0;
            ret = getConf(_KEY_DEFS->LIST_PTR[i].VAL, &val);
            encoder->write_float(val);
          }
          break;
        case TCode::VECT_3_FLOAT:
          {
            Vector3f val;
            ret = getConf(_KEY_DEFS->LIST_PTR[i].VAL, &val);
            encoder->write_array(3);
            encoder->write_float(val.x);
            encoder->write_float(val.y);
            encoder->write_float(val.z);
          }
          break;
        case TCode::BINARY:
          {
            uint8_t* ptr = nullptr;
            uint32_t len = 0;
            ret = getConf(_KEY_DEFS->LIST_PTR[i].VAL, &ptr, &len);
            encoder->write_bytes(ptr, len);
          }
          break;
        default:
          // TODO: We still want to emit valid CBOR even if there was a problem.
          encoder->write_int(0);
          break;
      }
    }
  }
  return ret;
}


#endif  // __C3P_DATAREC_CONFRECORD_H
