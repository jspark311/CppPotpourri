/*
File:   Storage.h
Author: J. Ian Lindsay
Date:   2016.08.15

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

#include "StringBuilder.h"
#include "LightLinkedList.h"
#include "CppPotpourri.h"
#include "cbor-cpp/cbor.h"

#ifndef __ABSTRACT_PERSIST_LAYER_H__
#define __ABSTRACT_PERSIST_LAYER_H__

/* DataRecord constants for serializer. */
#define DATARECORD_SERIALIZER_VERSION  1  // Makes record migration possible.
#define DATARECORD_BASE_SIZE          40  // How many bytes that are independent of block address size?

/* DataRecord flags */
#define DATA_RECORD_FLAG_PENDING_IO     0x01  // We are in the process of saving or loading.
#define DATA_RECORD_FLAG_PENDING_ALLOC  0x02  // We are waiting on an allocation to happen.
#define DATA_RECORD_READ_ONLY           0x04  // Set for records which will not be written as a matter of policy.


/*
* DataRecord types. Although laregly arbitrary, this 8-bit enum space must be
*   construed as write-only. Once assigned to a record type, that assignment
*   should not be changed, lest back-compat be broken.
* NOTE: This enum need not be exapnded for most kinds of storage support, and is
*   mostly used to aid rapid sorting of records by a storage driver without
*   implying knowledge of anything and everything that might be held in a record
*   payload. Most storage needs can be tunneled within the C3POBJ_ON_ICE record.
*/
enum class StorageRecordType : uint8_t {
  UNINIT        = 0,   // RESERVED: Uninitalized.
  ROOT          = 1,   // RESERVED (Storage driver): The root storage block.
  KEY_LISTING   = 2,   // RESERVED (Storage driver): A listing of DataRecord keys.
  C3POBJ_ON_ICE = 3,   // A direct-to-disk C3P object, which should cover any serializable type in C3P.
  LOG           = 4,   // A firmware-generated log.
  CONFIG_OBJ    = 5,   // User configuration.
  FIRMWARE_BLOB = 6,   // A versioned firmware blob.
  INVALID       = 255  // RESERVED: Catch-all for unsupported types.
};

/**
* A general data record that doesn't carry any implications of asynchronous
*   block I/O. This is useful in cases where you want data abstraction,
*   versioning, and assurance, but would prefer to handle the mechanisms of
*   storage your own way.
*/
class SimpleDataRecord {
  public:
    inline uint32_t          hash() {         return _calculate_hash();             };
    inline StorageRecordType recordType() {   return _record_type;                  };
    inline uint64_t          timestamp() {    return _timestamp;                    };
    inline bool              isDirty() {      return (_hash != _calculate_hash());  };

    void printDebug(StringBuilder*);
    int8_t save(uint32_t storage_tag, const char* name, StringBuilder* outbound_buf);
    int8_t load(uint32_t storage_tag, StringBuilder* inbound_buf);

    virtual int8_t serialize(StringBuilder*, TCode) =0;
    virtual int8_t deserialize(StringBuilder*, TCode) =0;


  protected:
    SimpleDataRecord() {};
    SimpleDataRecord(uint32_t storage_tag, StorageRecordType rtype) : _record_type(rtype) {};
    ~SimpleDataRecord() {};

    int serialize_cbor_kvp_for_record(cbor::encoder*);

    inline uint32_t _get_storage_tag() {        return _storage_tag;     };
    inline char*    _record_name() {            return (char*) _key;     };

    /* Flag manipulation inlines */
    inline uint8_t _dr_flags() {                return _flags;           };
    inline bool _dr_flag(uint8_t _flag) {       return (_flags & _flag); };
    inline void _dr_clear_flag(uint8_t _flag) { _flags &= ~_flag;        };
    inline void _dr_set_flag(uint8_t _flag) {   _flags |= _flag;         };
    inline void _dr_set_flag(uint8_t _flag, bool nu) {
      if (nu) _flags |= _flag;
      else    _flags &= ~_flag;
    };


  private:
    // This member data is stored in an aligned binary format as the first
    //   bytes of any record.
    // All block address fields are 32-bit for class simplicity, but will be
    //   truncated to the size appropriate for the storage driver.
    uint32_t          _storage_tag  = 0;      // Used by the storage apparatus.
    uint8_t           _version      = 0;      // Serializer version for this record.
    uint8_t           _flags        = 0;      // Flags to describe this record.
    StorageRecordType _record_type  = StorageRecordType::UNINIT;
    TCode             _format       = TCode::CBOR;  // Default serialization format for payload is CBOR.
    uint8_t           _key[16]      = {0, };  // Application-provided name for this record.
    uint32_t          _data_length  = 0;      // How many bytes does the record occupy?
    uint64_t          _timestamp    = 0;      // Epoch timestamp of the record creation.
    uint32_t          _hash         = 0;      // Autogenerated payload hash.

    int8_t        _descriptor_serialize(StringBuilder*);
    int8_t        _descriptor_deserialize(StringBuilder*);
    int8_t        _sanitize_name(const char*);
    uint32_t      _calculate_hash();
    inline void   _mark_clean() {  _hash = _calculate_hash();  };
};

#endif // __ABSTRACT_PERSIST_LAYER_H__
