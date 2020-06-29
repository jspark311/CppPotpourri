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

#ifndef __ABSTRACT_PERSIST_LAYER_H__
#define __ABSTRACT_PERSIST_LAYER_H__

/* Class flags */
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
*   An error code
*   The key that was requested
*   The data associated with the given key
*/
typedef int8_t (*StorageReadCallback)(StorageErr, const char* key, StringBuilder* data);



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
    virtual uint64_t   freeSpace() =0;  // How many bytes are availible for use?
    virtual StorageErr wipe()      =0;  // Call to wipe the data store.

    /* Raw buffer API. Might have more overhead on some platforms. */
    virtual StorageErr persistentWrite(const char*, uint8_t*, unsigned int, uint16_t) =0;
    virtual StorageErr persistentRead(const char*, uint8_t*, unsigned int*, uint16_t) =0;

    /* StringBuilder API to avoid pedantic copying. */
    virtual StorageErr persistentWrite(const char*, StringBuilder*, uint16_t) =0;
    virtual StorageErr persistentRead(const char*, StringBuilder*, uint16_t)  =0;

    /* Public flag accessors. */
    inline bool isFilesystem() {  return _pl_flag(PL_FLAG_USES_FILESYSTEM);  };
    inline bool isEncrypted() {   return _pl_flag(PL_FLAG_ENCRYPTED);        };
    inline bool isRemovable() {   return _pl_flag(PL_FLAG_REMOVABLE);        };
    inline bool isMounted() {     return _pl_flag(PL_FLAG_MEDIUM_MOUNTED);   };
    inline bool isReadable() {    return _pl_flag(PL_FLAG_MEDIUM_READABLE);  };
    inline bool isWritable() {    return _pl_flag(PL_FLAG_MEDIUM_WRITABLE);  };

    inline bool isBusy() {
      return (_pl_flags & (PL_FLAG_BUSY_WRITE | PL_FLAG_BUSY_READ));
    };

    inline void setReadCallback(StorageReadCallback cb) {
      _read_cb = cb;
    };

    static const char* const errStr(StorageErr);


  protected:
    StorageReadCallback  _read_cb = nullptr;

    Storage() {};  // Protected constructor.

    virtual void printStorage(StringBuilder*);

    inline void _report_free_space(uint64_t x) { _free_space = x; };

    inline bool _pl_flag(uint16_t f) {   return ((_pl_flags & f) == f);   };
    inline void _pl_clear_flag(uint16_t _flag) {  _pl_flags &= ~_flag;    };
    inline void _pl_set_flag(uint16_t _flag) {    _pl_flags |= _flag;     };
    inline void _pl_set_flag(bool nu, uint16_t _flag) {
      _pl_flags = (nu) ? (_pl_flags | _flag) : (_pl_flags & ~_flag);
    };


  private:
    uint64_t  _free_space  = 0L;
    uint16_t  _pl_flags    = 0;
};

#endif // __ABSTRACT_PERSIST_LAYER_H__
