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
* This class handles a conflux of three bodies of code
*   Validation (assuring that content is within-bounds of knowledge)
*   Storage (parsing and packing)
*   API (presentation to other code)
*
* This record type is pure virtual, and the type-control elements implemented as
*   a template against whatever set of enumerated values are stored in the
*   EnumDefList. This allows other classes to use ConfRecords generally without
*   concern for the specifics of their internal type-control. ConfRecord does
*   not force the propagation of templates.
*
* More-specific controls over value ranges (IE, constraining a parameter to the
*   range [0, 100]) should be exercised by a class that extends the template
*   ConfRecordValidation<your_validation_list>, with _that_ instance being used
*   by your program's code. See the examples and unit_tests directories for
*   guidance.
*
* Inside the validation layer, data is stored live with the KeyValuePair type,
*   and serialized into (and out of) the SimpleDataRecord class.
*/

#include "../../StringBuilder.h"
#include "../../C3PValue/KeyValuePair.h"
#include "../../Vector3.h"
#include "../../EnumWrapper.h"
#include "../Storage.h"

#ifndef __C3P_DATAREC_CONFRECORD_H
#define __C3P_DATAREC_CONFRECORD_H


/*
* ConfRecord is the base class for all records that pertain to runtime
*   configuration of the program.
*/
class ConfRecord : public SimpleDataRecord {
  public:
    ~ConfRecord();

    /**
    * Gets a value for the given ConfKey.
    *
    * @param key is the name of the data to fetch.
    * @param val is the pointer to the variable to receive the result.
    * @return 0 on success, -1 for invalid key, -2 for data deprecation if converted, -3 for no values.
    */
    //int8_t setConf(ConfKey, uint8_t*,  uint32_t  len) =0;  // Used for opaque tables.
    //int8_t getConf(ConfKey, uint8_t**, uint32_t* len) =0;  // Used for opaque tables.

    inline int8_t setConf(const char* KEY, uint8_t val) {       return _set_conf(KEY, TCode::UINT8,   (void*) &val);  };
    inline int8_t setConf(const char* KEY, uint16_t val) {      return _set_conf(KEY, TCode::UINT16,  (void*) &val);  };
    inline int8_t setConf(const char* KEY, uint32_t val) {      return _set_conf(KEY, TCode::UINT32,  (void*) &val);  };
    inline int8_t setConf(const char* KEY, uint64_t val) {      return _set_conf(KEY, TCode::UINT64,  (void*) &val);  };
    inline int8_t setConf(const char* KEY, int8_t val) {        return _set_conf(KEY, TCode::INT8,    (void*) &val);  };
    inline int8_t setConf(const char* KEY, int16_t val) {       return _set_conf(KEY, TCode::INT16,   (void*) &val);  };
    inline int8_t setConf(const char* KEY, int32_t val) {       return _set_conf(KEY, TCode::INT32,   (void*) &val);  };
    inline int8_t setConf(const char* KEY, int64_t val) {       return _set_conf(KEY, TCode::INT64,   (void*) &val);  };
    inline int8_t setConf(const char* KEY, bool val) {          return _set_conf(KEY, TCode::BOOLEAN, (void*) &val);  };
    inline int8_t setConf(const char* KEY, float val) {         return _set_conf(KEY, TCode::FLOAT,   (void*) &val);  };
    inline int8_t setConf(const char* KEY, double val) {        return _set_conf(KEY, TCode::DOUBLE,  (void*) &val);  };
    inline int8_t setConf(const char* KEY, char* val) {         return _set_conf(KEY, TCode::STR,     (void*) val);  };
    inline int8_t setConf(const char* KEY, const char* val) {   return _set_conf(KEY, TCode::STR,     (void*) val);  };
    inline int8_t setConf(const char* KEY, Vector3f* val) {     return _set_conf(KEY, TCode::VECT_3_FLOAT, (void*) val);  };

    inline int8_t getConf(const char* KEY, uint8_t* val) {      return _get_conf(KEY, TCode::UINT8,   (void*) val);  };
    inline int8_t getConf(const char* KEY, uint16_t* val) {     return _get_conf(KEY, TCode::UINT16,  (void*) val);  };
    inline int8_t getConf(const char* KEY, uint32_t* val) {     return _get_conf(KEY, TCode::UINT32,  (void*) val);  };
    inline int8_t getConf(const char* KEY, uint64_t* val) {     return _get_conf(KEY, TCode::UINT64,  (void*) val);  };
    inline int8_t getConf(const char* KEY, int8_t* val) {       return _get_conf(KEY, TCode::INT8,    (void*) val);  };
    inline int8_t getConf(const char* KEY, int16_t* val) {      return _get_conf(KEY, TCode::INT16,   (void*) val);  };
    inline int8_t getConf(const char* KEY, int32_t* val) {      return _get_conf(KEY, TCode::INT32,   (void*) val);  };
    inline int8_t getConf(const char* KEY, int64_t* val) {      return _get_conf(KEY, TCode::INT64,   (void*) val);  };
    inline int8_t getConf(const char* KEY, bool* val) {         return _get_conf(KEY, TCode::BOOLEAN, (void*) val);  };
    inline int8_t getConf(const char* KEY, float* val) {        return _get_conf(KEY, TCode::FLOAT,   (void*) val);  };
    inline int8_t getConf(const char* KEY, double* val) {       return _get_conf(KEY, TCode::DOUBLE,  (void*) val);  };
    inline int8_t getConf(const char* KEY, char** val) {        return _get_conf(KEY, TCode::STR,     (void*) val);  };
    inline int8_t getConf(const char* KEY, const char** val) {  return _get_conf(KEY, TCode::STR,     (void*) val);  };
    inline int8_t getConf(const char* KEY, Vector3f* val) {     return _get_conf(KEY, TCode::VECT_3_FLOAT, (void*) val);  };

    KeyValuePair* getKVP();

    /* Obligate overrides from DataRecord. */
    int8_t serialize(StringBuilder*, TCode);
    int8_t deserialize(StringBuilder*, TCode);

    inline const uint32_t keyCount() {   return _key_count();   };
    inline void           keyList(StringBuilder* out) {   _key_list(out);   };
    bool allocated(bool force_allocate = false);
    void printConfRecord(StringBuilder*, const char* KEY = nullptr);

    /* If a console is in use, provide a breakout. */
    int console_handler(StringBuilder* log, StringBuilder* args);


  protected:
    // This member should never be exposed publicly, or the validation layer
    //   will be compromised. The content of this KVP should always be exactly
    //   conformant to the content of the enum. See contract doc.
    KeyValuePair* _kvp;
    bool _config_is_complete;

    ConfRecord(uint32_t storage_tag) :
      SimpleDataRecord(storage_tag, StorageRecordType::CONFIG_OBJ),
      _kvp(nullptr), _config_is_complete(false) {};

    int8_t  _discard_allocations();
    int32_t _allocate_kvp();

    int8_t _set_conf(const char* KEY, const TCode, void* val);
    int8_t _get_conf(const char* KEY, const TCode, void* val);
    int8_t _get_conf(const char* KEY, const TCode, uint8_t**, uint32_t* len);

    /* Slots to be occupied by the validation template. */
    virtual const char* const _list_name() =0;
    virtual const uint32_t    _key_count() =0;
    virtual void              _key_list(StringBuilder*) =0;
    virtual TCode             _key_tcode(const char*) =0;
    virtual uint32_t          _data_size(uint32_t* keys, uint32_t* data) =0;
};


/*
* The template below is a shim for the class above. This arrangement allows us
*   to forget about the type-control happening under the requests, and makes
*   the resulting code much cleaner to work with.
*/
template <class T> class ConfRecordValidation : public ConfRecord {
  public:
    ConfRecordValidation(uint32_t storage_tag, const EnumDefList<T>* const KDEFS) :
      ConfRecord(storage_tag), _KEY_DEFS(KDEFS) {};

    /* Optional public API overrides for extending classes. */
    //virtual int8_t resetToDefaults() {  return 0;  };

    // If the user of the ConfRecord knows exactly what enum is doing the
    //   validation, provide lookup on that basis to reduce string replication.
    inline int8_t setConf(const T KEY, uint8_t val) {       return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::UINT8,   (void*) &val);  };
    inline int8_t setConf(const T KEY, uint16_t val) {      return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::UINT16,  (void*) &val);  };
    inline int8_t setConf(const T KEY, uint32_t val) {      return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::UINT32,  (void*) &val);  };
    inline int8_t setConf(const T KEY, uint64_t val) {      return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::UINT64,  (void*) &val);  };
    inline int8_t setConf(const T KEY, int8_t val) {        return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::INT8,    (void*) &val);  };
    inline int8_t setConf(const T KEY, int16_t val) {       return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::INT16,   (void*) &val);  };
    inline int8_t setConf(const T KEY, int32_t val) {       return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::INT32,   (void*) &val);  };
    inline int8_t setConf(const T KEY, int64_t val) {       return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::INT64,   (void*) &val);  };
    inline int8_t setConf(const T KEY, bool val) {          return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::BOOLEAN, (void*) &val);  };
    inline int8_t setConf(const T KEY, float val) {         return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::FLOAT,   (void*) &val);  };
    inline int8_t setConf(const T KEY, double val) {        return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::DOUBLE,  (void*) &val);  };
    inline int8_t setConf(const T KEY, char* val) {         return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::STR,     (void*) val);  };
    inline int8_t setConf(const T KEY, const char* val) {   return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::STR,     (void*) val);  };
    inline int8_t setConf(const T KEY, Vector3f* val) {     return _set_conf(_KEY_DEFS->enumStr(KEY), TCode::VECT_3_FLOAT, (void*) val);  };

    inline int8_t getConf(const T KEY, uint8_t* val) {      return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::UINT8,   (void*) val);  };
    inline int8_t getConf(const T KEY, uint16_t* val) {     return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::UINT16,  (void*) val);  };
    inline int8_t getConf(const T KEY, uint32_t* val) {     return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::UINT32,  (void*) val);  };
    inline int8_t getConf(const T KEY, uint64_t* val) {     return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::UINT64,  (void*) val);  };
    inline int8_t getConf(const T KEY, int8_t* val) {       return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::INT8,    (void*) val);  };
    inline int8_t getConf(const T KEY, int16_t* val) {      return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::INT16,   (void*) val);  };
    inline int8_t getConf(const T KEY, int32_t* val) {      return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::INT32,   (void*) val);  };
    inline int8_t getConf(const T KEY, int64_t* val) {      return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::INT64,   (void*) val);  };
    inline int8_t getConf(const T KEY, bool* val) {         return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::BOOLEAN, (void*) val);  };
    inline int8_t getConf(const T KEY, float* val) {        return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::FLOAT,   (void*) val);  };
    inline int8_t getConf(const T KEY, double* val) {       return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::DOUBLE,  (void*) val);  };
    inline int8_t getConf(const T KEY, char** val) {        return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::STR,     (void*) val);  };
    inline int8_t getConf(const T KEY, const char** val) {  return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::STR,     (void*) val);  };
    inline int8_t getConf(const T KEY, Vector3f* val) {     return _get_conf(_KEY_DEFS->enumStr(KEY), TCode::VECT_3_FLOAT, (void*) val);  };


  protected:
    const EnumDefList<T>* const _KEY_DEFS;   // A list of possible value keys, and their types.

    inline const T _get_confkey_by_string(const char* KEY_STR) {
      return _KEY_DEFS->getEnumByStr(KEY_STR);
    };

    /* Obligate overrides from ConfRecord. */
    const char* const _list_name() {                  return _KEY_DEFS->LIST_NAME;        };
    const uint32_t    _key_count() {                  return _KEY_DEFS->exportCount();    };
    void              _key_list(StringBuilder* ex) {  _KEY_DEFS->exportKeys(ex);          };
    TCode             _key_tcode(const char*);
    uint32_t          _data_size(uint32_t* keysizes = nullptr, uint32_t* datsizes = nullptr);
};



/*******************************************************************************
* ConfRecordValidation implementation
*******************************************************************************/

template <class T> TCode ConfRecordValidation<T>::_key_tcode(const char* KEY_STR) {
  const EnumDef<T>* DEF = _KEY_DEFS->getEnumDefByStr(KEY_STR);
  if (nullptr != DEF) {
    if (0 != DEF->CONTEXT) {
      return (TCode) DEF->CONTEXT;
    }
  }
  return TCode::INVALID;
}


/*
* Return the minimum size required to store this configuration.
*/
template <class T> uint32_t ConfRecordValidation<T>::_data_size(uint32_t* keysizes, uint32_t* datsizes) {
  uint32_t ret_keys = 0;
  uint32_t ret_data = 0;
  for (uint32_t i = 0; i < _KEY_DEFS->COUNT; i++) {
    // Ignore any catch-alls. They are commonly INVALID bumpers.
    if (!(_KEY_DEFS->LIST_PTR[i].FLAGS & ENUM_WRAPPER_FLAG_CATCHALL)) {
      const int THIS_KEYS_TCODE_SIZE = sizeOfType((const TCode) _KEY_DEFS->LIST_PTR[i].CONTEXT);
      ret_keys += strlen(_KEY_DEFS->LIST_PTR[i].STR);
      // TODO:
      //ret_data += (0 < THIS_KEYS_TCODE_SIZE) ? THIS_KEYS_TCODE_SIZE : _kvp->getLengthOfDataByKey(_KEY_DEFS->LIST_PTR[i].STR);
      if (0 < THIS_KEYS_TCODE_SIZE) {
        ret_data += THIS_KEYS_TCODE_SIZE;
      }
      else {
        // TODO: See above.
      }
    }
  }
  if (nullptr != keysizes) {  *keysizes = ret_keys;  }
  if (nullptr != datsizes) {  *datsizes = ret_data;  }
  return (ret_keys + ret_data);
}


#endif  // __C3P_DATAREC_CONFRECORD_H
