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
  data storage mechanism. Firmware that wants to use local data that
  is not compiled-in as constants will need at least one class that
  extends this.

All calls to this class are async. So buffer safety is to be enforced by the
  platform-specific implementation. This might mean more than doubling the
  memory burden for hard-copies with the raw buffer API.

TODO: Move to BusQueue for callback support? Might get really messy.
*/

#include "StringBuilder.h"
#include "LightLinkedList.h"
#include "CppPotpourri.h"
#include "cbor-cpp/cbor.h"

#ifndef __ABSTRACT_PERSIST_LAYER_H__
#define __ABSTRACT_PERSIST_LAYER_H__

class Storage;
class DataRecord;
class StorageBlock;

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

/* DataRecord flags */
#define DATA_RECORD_FLAG_PENDING_IO     0x01  // We are in the process of saving or loading.
#define DATA_RECORD_FLAG_PENDING_ALLOC  0x02  // We are waiting on an allocation to happen.

/* DataRecord types that are reserved for the driver's use. */
#define STORAGE_RECORD_TYPE_UNINIT           0  // Reserved. Uninitalized.
#define STORAGE_RECORD_TYPE_ROOT             1  // The root storage block.
#define STORAGE_RECORD_TYPE_KEY_LISTING      2  // A listing of DataRecord keys.
#define STORAGE_RECORD_TYPE_LOG              3  // A firmware-generated log.
#define STORAGE_RECORD_TYPE_USER_CONF        4  // User configuration.
#define STORAGE_RECORD_TYPE_LOCATION         5  // Location on Earth.
#define STORAGE_RECORD_TYPE_CAL_DATA         6
#define STORAGE_RECORD_TYPE_FIRMWARE_BLOB   10  // A versioned firmware blob.
#define STORAGE_RECORD_TYPE_AUDIO           14  // Audio stream.

/* DataRecord constants for serializer. */
#define DATARECORD_SERIALIZER_VERSION  1  // Makes record migration possible.
#define DATARECORD_BASE_SIZE          28  // How many bytes that are independent of block address size?



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
    inline bool     isDirty() {      return (_hash == _calculate_hash());             };
    inline bool     pendingIO() {    return _dr_flag(DATA_RECORD_FLAG_PENDING_IO);    };
    inline bool     pendingAlloc() { return _dr_flag(DATA_RECORD_FLAG_PENDING_ALLOC); };
    inline uint8_t  recordType() {   return _record_type;   };
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

    static const char* recordTypeStr(uint8_t);


  protected:
    Storage* _storage = nullptr;        // Pointer to the Storage driver that spawned us.
    LinkedList<StorageBlock*> _blocks;  // Blocks from the Storage driver.
    StringBuilder _outbound_buf;        // Data on its way to the Storage driver.

    DataRecord() {};
    DataRecord(uint8_t rtype) : _record_type(rtype) {};
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
    uint8_t  _record_type  = 0;      // What this means is mostly up to the application.
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
