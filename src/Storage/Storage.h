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


This file represents the platform-agnostic interface to a persistent
  data storage mechanism. CppPotpourri provides a few broad ways to do this.

  1) SimpleDataRecord: This class is a glorified parser/packer class that
     provides some basic naming and consistency assurances. It transacts in
     terms of buffers, and is the easiest to use.
  2) The Storage class: This class represents an asynchronous general storage
     mechanism.
*/

#include "../StringBuilder.h"
#include "../LightLinkedList.h"
#include "../CppPotpourri.h"
#include "../cbor-cpp/cbor.h"

#ifndef __ABSTRACT_PERSIST_LAYER_H__
#define __ABSTRACT_PERSIST_LAYER_H__


/*******************************************************************************
* Common to both options
*******************************************************************************/
/* DataRecord constants for serializer. */
#define DATARECORD_SERIALIZER_VERSION  1  // Makes record migration possible.
#define DATARECORD_BASE_SIZE          40  // How many bytes that are independent of block address size?

/* DataRecord flags */
#define DATA_RECORD_FLAG_PENDING_IO     0x01  // We are in the process of saving or loading.
#define DATA_RECORD_FLAG_PENDING_ALLOC  0x02  // We are waiting on an allocation to happen.
#define DATA_RECORD_READ_ONLY           0x04  // Set for records which will not be written as a matter of policy.
#define DATA_RECORD_FLAG_WAS_LOADED     0x08  // This record was loaded from storage.

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



/*******************************************************************************
* Option 1
* A general data record that doesn't carry any implications of asynchronous
*   block I/O. This is useful in cases where you want data abstraction,
*   versioning, and assurance, but would prefer to handle the mechanisms of
*   storage your own way.
*******************************************************************************/
class SimpleDataRecord {
  public:
    inline uint32_t          hash() {         return _calculate_hash();             };
    inline StorageRecordType recordType() {   return _record_type;                  };
    inline uint64_t          timestamp() {    return _timestamp;                    };
    inline bool              isDirty() {      return (_hash != _calculate_hash());  };
    inline bool              wasLoaded() {    return _dr_flag(DATA_RECORD_FLAG_WAS_LOADED);  };

    void printDebug(StringBuilder*);
    int8_t save(uint32_t storage_tag, const char* name, StringBuilder* outbound_buf);
    int8_t load(uint32_t storage_tag, StringBuilder* inbound_buf);
    int8_t load(uint32_t storage_tag, uint8_t* inbound_buf, uint32_t len);

    virtual int8_t serialize(StringBuilder*, TCode) =0;
    virtual int8_t deserialize(StringBuilder*, TCode) =0;


  protected:
    SimpleDataRecord(uint32_t storage_tag, StorageRecordType rtype);
    SimpleDataRecord() : SimpleDataRecord(0, StorageRecordType::UNINIT) {};
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
    uint32_t          _storage_tag;  // Used by the storage apparatus.
    uint8_t           _version;      // Serializer version for this record.
    uint8_t           _flags;        // Flags to describe this record.
    StorageRecordType _record_type;  // Designated record type.
    TCode             _format;       // Default serialization format for payload is CBOR.
    uint8_t           _key[16];      // Application-provided name for this record.
    uint32_t          _data_length;  // How many bytes does the record occupy?
    uint64_t          _timestamp;    // Epoch timestamp of the record creation.
    uint32_t          _hash;         // Autogenerated payload hash.

    int8_t        _descriptor_serialize(StringBuilder*);
    int8_t        _descriptor_deserialize(StringBuilder*);
    int8_t        _sanitize_name(const char*);
    uint32_t      _calculate_hash();
    inline void   _mark_clean() {  _hash = _calculate_hash();  };
};



/*******************************************************************************
* Option 2
* Firmware that wants to use the asynchronous storage abstraction will need to
*   provide an implementation of Storage that handles the concrete matters of
*   the particular storage media.
*
* All calls to Storage are async. So buffer safety is to be enforced by the
*   platform-specific implementation. This might mean more than doubling the
*   memory burden for hard-copies with the raw buffer API.
*
* TODO: Move to BusQueue for callback support? Might get really messy.
* TODO: This API works, but is terrible to use. Re-evalutate.
* TODO: DataRecord and SimpleDataRecord should be recondensed.
*******************************************************************************/

class Storage;
class DataRecord;
class StorageBlock;

/* Error codes for the storage class. */
enum class StorageErr : int8_t {
  KEY_CLOBBERED =  1,  // Special-case WRITE, the given key already existed, and was clobbered.
  NONE          =  0,  // No abnormal condition.
  UNSPECIFIED   = -1,  // Generic error.
  BAD_PARAM     = -2,  // A parameter given to the function was invalid.
  BUSY          = -3,  // The media is too busy to take the request.
  MEM_ALLOC     = -4,  // Not enough memory to run the operation safely.
  NOT_MOUNTED   = -5,  // No media available.
  NOT_READABLE  = -6,  // Media isn't readable.
  NOT_WRITABLE  = -7,  // Media isn't writable.
  NO_FREE_SPACE = -8,  // Special-case WRITE, Not enough free space to fulfill request.
  HW_FAULT      = -9,  // Underlying hardware fault.
  KEY_NOT_FOUND = -10, // Special-case READ, when the given key isn't found.
  KEY_COLLISION = -11, // Special-case WRITE, when the given key already exists.
};


/*
* Callback definition for read completion.
* Parameters are...
*   A DataRecord that recently completed an I/O operation.
*   An error code
*/
typedef int8_t (*StorageReadCallback)(DataRecord*, StorageErr);


/**
* This is an optional class for the convenience of concrete implementations of
*   Storage drivers.
*/
class StorageOp {
  public:
    LinkedList<StorageBlock*> _block_queue;
    DataRecord* callback_record;   //
    StringBuilder* buffer;         //
    bool is_write_op;              //

    StorageOp(DataRecord* _r, StringBuilder* _b, bool _w) :
      callback_record(_r),
      buffer(_b),
      is_write_op(_w)
      {};
};


/*******************************************************************************
* This class wraps and maps data into a sequence of blocks that are
*   stored in NVM. It allows data to be non-contiguous in memory, and allows
*   the generalized storage abstraction to forget about the low-level addresses
*   of indexed data.
*******************************************************************************/

/* Class for tracking lists of storage blocks. */
// DataStructure in use is a simple linked list (for now) TODO: Hash-table might be better.
// We eat an entire block for Record metadata.
class StorageBlock {
  public:
    // Low-level address containing the next chunk (or 0 if this is the tail).
    uint32_t this_offset;
    uint32_t next_offset;

    StorageBlock() : this_offset(0), next_offset(0) {};
    StorageBlock(const uint32_t to) : this_offset(to), next_offset(0) {};
    StorageBlock(const uint32_t to, const uint32_t no) : this_offset(to), next_offset(no) {};
};


/**
* A general data record, with a string key assigned by the application.
* When serialized, and stored, this object represents the head of a linked list
*   of blocks that represents a record. The real addresses of NVM locations are
*   held in this class.
*/
class DataRecord {
  public:
    inline uint32_t hash() {         return _calculate_hash();                        };
    inline bool     isDirty() {      return (_hash != _calculate_hash());             };
    inline bool     pendingIO() {    return _dr_flag(DATA_RECORD_FLAG_PENDING_IO);    };
    inline bool     pendingAlloc() { return _dr_flag(DATA_RECORD_FLAG_PENDING_ALLOC); };
    inline StorageRecordType recordType() {   return _record_type;   };
    inline uint64_t timestamp() {    return _timestamp;     };
    inline void setStorage(Storage* x) {    _storage = x;   };

    void printDebug(StringBuilder*);
    int8_t save(char* name);
    int8_t load(char* name);
    virtual int8_t serialize(StringBuilder*, TCode) =0;
    virtual int8_t deserialize(StringBuilder*, TCode) =0;

    /* Interface functions for the Storage driver's use. */
    // TODO: These should be made private one this class is 'friendly' with Storage.
    //friend class Storage;
    int8_t buffer_request_from_storage(uint32_t* addr, uint8_t* buf, uint32_t* len);   // Storage providing data from a read request.
    int8_t buffer_offer_from_storage(uint32_t* addr, uint8_t* buf, uint32_t* len);     // Storage requesting data from a write request.
    inline LinkedList<StorageBlock*>* getBlockList() {  return &_blocks;  };

    static const char* const recordTypeStr(const StorageRecordType);


  protected:
    Storage* _storage = nullptr;        // Pointer to the Storage driver that spawned us.
    LinkedList<StorageBlock*> _blocks;  // Blocks from the Storage driver.
    StringBuilder _outbound_buf;        // Data on its way to the Storage driver.

    DataRecord() {};
    DataRecord(StorageRecordType rtype) : _record_type(rtype) {};
    ~DataRecord() {};

    uint32_t _calculate_hash();
    inline void   _mark_clean() {   _hash = _calculate_hash();     };

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
    uint8_t  _version      = 0;      // Serializer version for this record.
    uint8_t  _flags        = 0;      // Flags to describe this record.
    StorageRecordType  _record_type  = StorageRecordType::UNINIT;
    uint8_t  _key[9]       = {0, };  // Application-provided name for this record.
    uint32_t _hash         = 0;      // Autogenerated payload hash.
    uint32_t _data_length  = 0;      // How many bytes does the record occupy?
    uint64_t _timestamp    = 0;      // Epoch timestamp of the record creation.
    uint32_t _nxt_rec_addr = 0;      // Address of next record (0 if this is the last record).
    // _nxt_dat_addr is stored in the first StorageBlock member.

    /* Lookup functions for block relationships this record's data. */
    StorageBlock* _get_storage_block_by_addr(uint32_t addr);
    StorageBlock* _get_storage_block_by_nxt(uint32_t addr);
    uint32_t      _derive_allocated_size();
    int8_t        _post_alloc_save();
    int8_t        _sanitize_name(char*);

    int8_t  _fill_from_descriptor_block(uint8_t*);
};



/* Storage driver flags */
#define PL_FLAG_USES_FILESYSTEM      0x0001  // Medium is on top of a filesystem.
#define PL_FLAG_BLOCK_ACCESS         0x0002  // I/O operations must occur blockwise.
#define PL_FLAG_ENCRYPTED            0x0004  // Data stored in this medium is encrypted at rest.
#define PL_FLAG_REMOVABLE            0x0008  // Media are removable.
#define PL_FLAG_BATTERY_DEPENDENT    0x0010  // Data-retention is contingent on constant current, IE: RTC RAM.
#define PL_FLAG_MEDIUM_MOUNTED       0x0020  // Medium is ready for use.
#define PL_FLAG_MEDIUM_READABLE      0x0040  // Can be read.
#define PL_FLAG_MEDIUM_WRITABLE      0x0080  // Can be written.
#define PL_FLAG_BUSY_READ            0x4000  // There is a read operation pending completion.
#define PL_FLAG_BUSY_WRITE           0x8000  // There is a write operation pending completion.

/* Transfer option flags */
#define PL_FLAG_XFER_CLOBBER_KEY     0x0001  // A write operation should clobber the key if it already exists.


/**
* This class is a gateway to block-oriented I/O. It will almost certainly need
*   to have some of its operations run asynchronously or threaded. We leave
*   those concerns for any implementing class.
*/
class Storage {
  public:
    /*
    * Data-persistence functions. This is the API used by anything that wants to write
    *   formless data to a place on the device to be recalled on a different runtime.
    */
    inline uint32_t    deviceSize() {    return DEV_SIZE_BYTES;     };  // How many bytes can the device hold?
    inline uint32_t    blockSize() {     return DEV_BLOCK_SIZE;     };  // How granular is our use of space?
    inline uint32_t    totalBlocks() {   return DEV_TOTAL_BLOCKS;   };  // How granular is our use of space?
    inline StorageErr  wipe() {     return wipe(0, deviceSize());   };  // Call to wipe the data store.
    inline StorageErr  wipe(uint32_t offset) {   return wipe(offset, blockSize());   };   // Wipe a block.

    virtual StorageErr wipe(uint32_t offset, uint32_t len) =0;  // Wipe a range.
    virtual uint8_t    blockAddrSize() =0;  // How many bytes is a block reference?
    virtual int8_t     allocateBlocksForLength(uint32_t, DataRecord*) =0;

    /* High-level DataRecord API. */
    virtual StorageErr persistentWrite(DataRecord*, StringBuilder* buf) =0;
    //virtual StorageErr persistentRead(DataRecord*, StringBuilder* buf) =0;

    /* Raw buffer API. Might have more overhead on some platforms. */
    virtual StorageErr persistentWrite(uint8_t* buf, unsigned int len, uint32_t offset) =0;
    virtual StorageErr persistentRead(uint8_t* buf,  unsigned int len, uint32_t offset) =0;

    /* Public flag accessors. */
    inline uint32_t freeSpace() {     return _free_space;   };  // How many bytes are availible for use?
    inline bool     isFilesystem() {  return _pl_flag(PL_FLAG_USES_FILESYSTEM);  };
    inline bool     isEncrypted() {   return _pl_flag(PL_FLAG_ENCRYPTED);        };
    inline bool     isRemovable() {   return _pl_flag(PL_FLAG_REMOVABLE);        };
    inline bool     isMounted() {     return _pl_flag(PL_FLAG_MEDIUM_MOUNTED);   };
    inline bool     isReadable() {    return _pl_flag(PL_FLAG_MEDIUM_READABLE);  };
    inline bool     isWritable() {    return _pl_flag(PL_FLAG_MEDIUM_WRITABLE);  };
    inline bool     isBusy() {        return (_pl_flags & (PL_FLAG_BUSY_WRITE | PL_FLAG_BUSY_READ));  };

    inline void setReadCallback(StorageReadCallback cb) {    _cb = cb;    };

    static const char* errStr(const StorageErr);


  protected:
    // Constants to represent the underlying hardware.
    const uint32_t DEV_SIZE_BYTES;
    const uint32_t DEV_BLOCK_SIZE;
    const uint32_t DEV_TOTAL_BLOCKS;     // Usually "pages" in NVM contexts.
    const uint8_t  DEV_ADDR_SIZE_BYTES;  // Size of integer required to hold an address.
    uint16_t  _pl_flags     = 0;
    uint32_t  _free_space   = 0L;
    StorageReadCallback _cb = nullptr;
    LinkedList<StorageOp*> _op_queue;   // List of state-bearing transactions.

    Storage(const uint32_t ds_bytes, const uint32_t bs_bytes);  // Protected constructor.

    void _print_storage(StringBuilder*);

    int8_t _invoke_record_callback(DataRecord* rec, StorageErr err);

    inline bool _pl_flag(uint16_t f) {   return ((_pl_flags & f) == f);   };
    inline void _pl_clear_flag(uint16_t _flag) {  _pl_flags &= ~_flag;    };
    inline void _pl_set_flag(uint16_t _flag) {    _pl_flags |= _flag;     };
    inline void _pl_set_flag(uint16_t _flag, bool nu) {
      _pl_flags = (nu) ? (_pl_flags | _flag) : (_pl_flags & ~_flag);
    };
};

#endif // __ABSTRACT_PERSIST_LAYER_H__
