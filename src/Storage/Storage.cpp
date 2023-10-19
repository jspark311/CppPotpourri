/*
File:   Storage.cpp
Author: J. Ian Lindsay
Date:   2016.08.28

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


This is the basal implementation of the Storage interface.
*/

#include "Storage.h"
#include "cbor-cpp/cbor.h"


const char* Storage::errStr(const StorageErr e) {
  switch (e) {
    case StorageErr::KEY_CLOBBERED:    return "KEY_CLOBBERED";
    case StorageErr::NONE:             return "NONE";
    case StorageErr::UNSPECIFIED:      return "UNSPECIFIED";
    case StorageErr::BAD_PARAM:        return "BAD_PARAM";
    case StorageErr::BUSY:             return "BUSY";
    case StorageErr::MEM_ALLOC:        return "MEM_ALLOC";
    case StorageErr::NOT_MOUNTED:      return "NOT_MOUNTED";
    case StorageErr::NOT_READABLE:     return "NOT_READABLE";
    case StorageErr::NOT_WRITABLE:     return "NOT_WRITABLE";
    case StorageErr::NO_FREE_SPACE:    return "NO_FREE_SPACE";
    case StorageErr::HW_FAULT:         return "HW_FAULT";
    case StorageErr::KEY_NOT_FOUND:    return "KEY_NOT_FOUND";
    case StorageErr::KEY_COLLISION:    return "KEY_COLLISION";
  }
  return "UNKNOWN";
}

/*******************************************************************************
* Storage base-class functions
*******************************************************************************/

/**
* Protected constructor
*/
Storage::Storage(const uint32_t ds_bytes, const uint32_t bs_bytes) :
  DEV_SIZE_BYTES(ds_bytes),
  DEV_BLOCK_SIZE(bs_bytes),
  DEV_TOTAL_BLOCKS(ds_bytes / bs_bytes),
  DEV_ADDR_SIZE_BYTES((ds_bytes < 65536) ? 2:4) {};


void Storage::_print_storage(StringBuilder* output) {
  output->concatf("-- Storage [%cr %cw, %sencrypted, %sremovable]\n",
    _pl_flag(PL_FLAG_MEDIUM_READABLE) ? '+' : '-',
    _pl_flag(PL_FLAG_MEDIUM_WRITABLE) ? '+' : '-',
    _pl_flag(PL_FLAG_ENCRYPTED) ? "" : "un",
    _pl_flag(PL_FLAG_REMOVABLE) ? "" : "non-"
  );
  output->concatf("-- %u total bytes across %u pages of %u bytes each.\n", DEV_SIZE_BYTES, DEV_TOTAL_BLOCKS, DEV_BLOCK_SIZE);
  output->concatf("\t Address size:\t %u\n", DEV_ADDR_SIZE_BYTES);
  if (isMounted()) {
    output->concatf("\t Medium mounted (%u bytes free)\t%s\n",
      freeSpace(),
      isBusy() ? "[BUSY]" : ""
    );
  }
  if (_pl_flag(PL_FLAG_USES_FILESYSTEM)) {
    output->concat("\t On top of FS\n");
  }
  if (_pl_flag(PL_FLAG_BLOCK_ACCESS)) {
    output->concat("\t Block access\n");
  }
  if (_pl_flag(PL_FLAG_BATTERY_DEPENDENT)) {
    output->concat("\t Battery-backed\n");
  }
}


/**
* A constrained invocation function for callbacks following I/O
*   completion on records.
*
* @return 0 on no callback, 1 otherwise.
*/
int8_t Storage::_invoke_record_callback(DataRecord* rec, StorageErr err) {
  int8_t ret = 0;
  if (nullptr != _cb) {
    _cb(rec, err);
    ret++;
  }
  return ret;
}
