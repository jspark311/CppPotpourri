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


This is the basal implementation of the storage interface.
*/

#include "Storage.h"
#include "cbor-cpp/cbor.h"

/*******************************************************************************
* Statics
*******************************************************************************/

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

void Storage::printStorage(StringBuilder* output) {
  output->concatf("-- Storage              [%sencrypted, %sremovable]\n",
    _pl_flag(PL_FLAG_ENCRYPTED) ? "" : "un",
    _pl_flag(PL_FLAG_REMOVABLE) ? "" : "non-"
  );
  if (isMounted()) {
    output->concatf("--\t Medium mounted %s %s  (%lu bytes free)\t%s\n",
      _pl_flag(PL_FLAG_MEDIUM_READABLE) ? "+r" : "",
      _pl_flag(PL_FLAG_MEDIUM_WRITABLE) ? "+w" : "",
      freeSpace(),
      isBusy() ? "[BUSY]" : ""
    );
  }
  if (_pl_flag(PL_FLAG_USES_FILESYSTEM)) {
    output->concat("--\t On top of FS\n");
  }
  if (_pl_flag(PL_FLAG_BLOCK_ACCESS)) {
    output->concat("--\t Block access\n");
  }
  if (_pl_flag(PL_FLAG_BATTERY_DEPENDENT)) {
    output->concat("--\t Battery-backed\n");
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



/*******************************************************************************
* DataRecord base-class functions
*******************************************************************************/

void DataRecord::printDebug(StringBuilder* output) {
  output->concatf("\t Key:\t %s\n", _key);
  output->concatf("\t Pending I/O:  %c\n", pendingIO() ? 'y':'n');
  output->concatf("\t Dirty:\t %c\n", isDirty() ? 'y':'n');
  output->concatf("\t Type:\t 0x%02x\n", recordType());
  output->concatf("\t Hash:\t 0x%08x\n", _hash);
  output->concatf("\t Len:\t %u\n", _data_length);

  for (int i = 0; i < _blocks.size(); i++) {
    StorageBlock* cur_block = _blocks.get(i);
    output->concatf("\t\t %5u  %5u\n", cur_block->this_offset, cur_block->next_offset);
  }
  StringBuilder::printBuffer(output, _outbound_buf.string(), _outbound_buf.length(), "");
}


/*
* Ensure that this record reflects the state of the storage, preferring the
*   state of the object.
* If the object was never stored, blocks will be allocated (or culled) and
*   metadata added to this object ahead of write initiation.
*/
int8_t DataRecord::save() {
  int8_t ret = -1;
  _outbound_buf.clear();
  if (0 == _serialize(&_outbound_buf, TCode::CBOR)) {
    ret--;
    // Craft a descriptor for this record. We will need our overhead region to
    //   be aligned and constant-length.
    const uint BLOCK_SIZE        = _storage->blockSize();
    const uint BLOCK_ADDR_SIZE   = _storage->blockAddrSize();
    const uint DESCRIPTOR_SIZE   = DATARECORD_BASE_SIZE + (BLOCK_ADDR_SIZE << 1);
    const uint PAYLOAD_SIZE      = _outbound_buf.length();
    const uint TOTAL_RECORD_SIZE = DESCRIPTOR_SIZE + PAYLOAD_SIZE;
    const uint ALLOCATED_SIZE    = _derive_allocated_size();
    _data_length = _outbound_buf.length();   // Update the payload length field.
    _mark_clean();  // Finalize the checksum over the data.

    uint8_t* rec_desc = (uint8_t*) alloca(DESCRIPTOR_SIZE);
    if (nullptr != rec_desc) {
      // Now to ensure we take up the right amount of space in storage.
      ret--;
      if (TOTAL_RECORD_SIZE <= ALLOCATED_SIZE) {
        // The record will fit. But if the record shrank, we might need to trim.
        const uint32_t BYTES_TO_TRIM = ALLOCATED_SIZE - TOTAL_RECORD_SIZE;
        if (BYTES_TO_TRIM >= BLOCK_SIZE) {
          // We can trim at least one block.
          // TODO: This should be done after saving.
        }
      }
      else if (0 != _storage->allocateBlocksForLength(TOTAL_RECORD_SIZE, &_blocks)) {
        // Storage driver couldn't find enough free blocks to store this record.
        // Clean up our mess and bail.
        _outbound_buf.clear();
        return ret;
      }

      // If our request for space was granted, we will now have a block list
      //   that we are expected to write to. Get the first block.
      ret--;
      StorageBlock* first_blk = _blocks.get(0);
      uint32_t _this_addr     = first_blk->this_offset;
      uint32_t _nxt_dat_addr  = first_blk->next_offset;

      // All the pieces are present. Start writing the descriptor.
      uint idx = 0;
      *(rec_desc + idx++) = DATARECORD_SERIALIZER_VERSION;
      *(rec_desc + idx++) = 0;   // No flag fields yet.
      *(rec_desc + idx++) = _record_type;
      for (uint8_t i = 0; i < sizeof(_key); i++) {
        *(rec_desc + idx++) = _key[i];
      }
      // Hash field.
      *(rec_desc + idx++) = (uint8_t) (_hash >> 0);
      *(rec_desc + idx++) = (uint8_t) (_hash >> 8);
      *(rec_desc + idx++) = (uint8_t) (_hash >> 16);
      *(rec_desc + idx++) = (uint8_t) (_hash >> 24);
      // Record length field (excluding this descriptor).
      *(rec_desc + idx++) = (uint8_t) (_data_length >> 0);
      *(rec_desc + idx++) = (uint8_t) (_data_length >> 8);
      *(rec_desc + idx++) = (uint8_t) (_data_length >> 16);
      *(rec_desc + idx++) = (uint8_t) (_data_length >> 24);
      // Timestamp field.
      *(rec_desc + idx++) = (uint8_t) (_timestamp >> 0);
      *(rec_desc + idx++) = (uint8_t) (_timestamp >> 8);
      *(rec_desc + idx++) = (uint8_t) (_timestamp >> 16);
      *(rec_desc + idx++) = (uint8_t) (_timestamp >> 24);
      *(rec_desc + idx++) = (uint8_t) (_timestamp >> 32);
      *(rec_desc + idx++) = (uint8_t) (_timestamp >> 40);
      *(rec_desc + idx++) = (uint8_t) (_timestamp >> 48);
      *(rec_desc + idx++) = (uint8_t) (_timestamp >> 56);

      // Finally, fill in our space-contingent address fields...
      for (uint8_t i = 0; i < BLOCK_ADDR_SIZE; i++) {
        const uint IDX_CONST   = idx++;
        const uint SHIFT_CONST = i << 3;
        *(rec_desc + IDX_CONST + 0)               = (uint8_t) (_nxt_rec_addr >> SHIFT_CONST);
        *(rec_desc + IDX_CONST + BLOCK_ADDR_SIZE) = (uint8_t) (_nxt_dat_addr >> SHIFT_CONST);
      }

      // Prepend the record descriptor and dispatch the I/O.
      _outbound_buf.prepend(rec_desc, idx);
      if (StorageErr::NONE == _storage->persistentWrite(_outbound_buf.string(), BLOCK_SIZE, _this_addr)) {
        _outbound_buf.cull(BLOCK_SIZE);   // TODO: Scary. Wasteful.
        _dr_set_flag(DATA_RECORD_FLAG_PENDING_IO);
        ret = 0;
      }
    }
  }
  return ret;
}


/*
* Ensure that this record reflects the state of the storage, preferring the
*   state of storage.
* If the object never existed in memory (but exists in NVM), the state of this
*   object will act as a sort of filter for the Storage driver. The first record
*   to be found with matching parameters is what will be returned.
*/
int8_t DataRecord::load(char* name) {
  int8_t ret = -1;
  // Clear the class state and copy the given key as the search term.
  _outbound_buf.clear();
  _blocks.clear();
  _data_length = 0;
  const uint16_t NAME_LENGTH = strict_min((uint16_t) sizeof(_key), (uint16_t) strlen(name));
  for (uint8_t i = 0; i < sizeof(_key); i++) {
    _key[i] = (i < NAME_LENGTH) ? *(name + i) : 0;
  }
  // To load a record we need a key and a record type.
  if ((_record_type != 0) & (_key[0] != 0) & (nullptr != _storage)) {
    ret--;
    if (StorageErr::NONE == _storage->persistentRead(nullptr, _storage->blockSize(), 0)) {
      _dr_set_flag(DATA_RECORD_FLAG_PENDING_IO);
      ret = 0;
    }
  }
  return ret;
}


/**
* This function is called by the Storage driver when there is a need for more
*   serialized data to feed into the NVM. We should make note of the block
*   address, and feed the payload into the deserializer.
*
* @param addr is a pointer to the address to which this data should be written.
* @param buf is the buffer to which this function should write.
* @param len is a pointer to the length of valid data in the buffer.
* @return  0 if the rewritten parameters are valid for next operation.
*         -1 to signal transfer termination (I/O complete)
*/
int8_t DataRecord::buffer_request_from_storage(uint32_t* addr, uint8_t* buf, uint32_t* len) {
  int8_t ret = -1;
  if (0 < _outbound_buf.length()) {
    // There is more to send. Reload the provided buffer with the requested amount of data.
    const uint BYTES_REQUESTED = *len;
    const uint BYTES_NEXT_SEND = strict_min((uint32_t) _outbound_buf.length(), (uint32_t) *len);
    const uint8_t* REM_BUF = _outbound_buf.string();
    const uint8_t BLOCK_ADDR_SIZE_BYTES = _storage->blockAddrSize();

    // Find the two values from StorageBlock to fill the addr field.
    StorageBlock* assoc_block     = nullptr;
    uint32_t      assoc_nxt_block = 0;
    for (int i = 0; i < _blocks.size(); i++) {
      StorageBlock* cur_block = _blocks.get(i);
      if (cur_block->this_offset == *addr) {
        assoc_block = cur_block;
        // We will definitely need the next block address. Might as well get it now.
        cur_block = _blocks.get(i+1);
        if (nullptr != cur_block) {
          assoc_nxt_block = cur_block->next_offset;
        }
        break;
      }
    }
    if (nullptr != assoc_block) {   // This should never fail.
      *addr = assoc_block->next_offset;  // The next address to write.
      for (uint i = 0; i < BLOCK_ADDR_SIZE_BYTES; i++) {
        // Add the page annotations for StorageBlock data.
        *(buf + i) = (uint8_t) ((assoc_nxt_block >> (i << 3)) & 0xFF);
      }

      // Copy over as much payload data as we can to fill the buffer, and zero the rest.
      for (uint i = 0; i < (BYTES_REQUESTED - BLOCK_ADDR_SIZE_BYTES); i++) {
        *(buf + i + BLOCK_ADDR_SIZE_BYTES) = (i < BYTES_NEXT_SEND) ? *(REM_BUF + i) : 0;
      }
      _outbound_buf.cull(BYTES_NEXT_SEND);
      // TODO: If we want to support partial page writes...
      //*len = BYTES_NEXT_SEND;
      ret = 0;
    }
  }
  if (0 != ret) {
    _dr_clear_flag(DATA_RECORD_FLAG_PENDING_IO);
  }
  return ret;
}


/**
* This function is called by the Storage driver when there is new data freshly
*   read from the NVM. We should make note of the block address, and feed the
*   payload into the deserializer.
*
* @param addr is a pointer to the address from which this buffer was read.
* @param buf is the buffer containing the data.
* @param len is a pointer to the length of valid data in the buffer.
* @return  0 if the rewritten parameters are valid for next operation.
*         -1 to signal transfer termination (I/O complete)
*/
int8_t DataRecord::buffer_offer_from_storage(uint32_t* addr, uint8_t* buf, uint32_t* len) {
  const uint BYTES_OFFERED = *len;
  const uint8_t BLOCK_ADDR_SIZE_BYTES = _storage->blockAddrSize();
  int8_t ret = -1;

  uint32_t bytes_remaining = BYTES_OFFERED;
  uint8_t* buffer_ptr      = buf;
  uint32_t nxt_dat_addr    = 0;
  uint32_t idx = 0;
  // If this is the first buffer to arrive for an uninitialized record...
  const uint BLOCK_ADDR_SIZE   = _storage->blockAddrSize();
  const uint DESCRIPTOR_SIZE   = DATARECORD_BASE_SIZE + (BLOCK_ADDR_SIZE << 1);
  if ((0 == _data_length) & (bytes_remaining >= DESCRIPTOR_SIZE)) {
    // If we haven't been loaded from NVM yet, check for a valid descriptor and
    //   compare our key and type against the ones we are looking for (ours).
    const uint8_t INCOMING_SER_VER = *(buf + 0);
    switch (INCOMING_SER_VER) {   // Version field, always.
      case DATARECORD_SERIALIZER_VERSION:   // Current version of serializer.
        {
          bool key_differs = false;
          for (uint i = 0; i < sizeof(_key); i++) {
            key_differs |= (_key[i] == *(buf + DATARECORD_BASE_SIZE + i));
          }
          if (key_differs | (_record_type != *(buf + 2))) {
            // If the key didn't match, instruct the Storage driver to get the
            //   next record block, rather than the next data block.
            for (uint i = 0; i < BLOCK_ADDR_SIZE_BYTES; i++) {
              nxt_dat_addr = (nxt_dat_addr << 8) | *(buf + DATARECORD_BASE_SIZE + i);
            }
            *addr = nxt_dat_addr;
            return (0 == nxt_dat_addr) ? -1 : 0;   // Stop I/O if this is the last record.
          }
          else {
            // This is the correct record. Fill the class variables, and ask for
            //   the next block of data (instead of the next record).
            _fill_from_descriptor_block(buf);
            for (uint i = 0; i < BLOCK_ADDR_SIZE_BYTES; i++) {
              nxt_dat_addr = (nxt_dat_addr << 8) | *(buf + DATARECORD_BASE_SIZE + BLOCK_ADDR_SIZE_BYTES + i);
            }
            buffer_ptr = buf + DESCRIPTOR_SIZE;
            bytes_remaining -= DESCRIPTOR_SIZE;
          }
        }
        break;
      default:
        // TODO: Fault condition.
        return -1;
    }
  }
  else {
    for (uint i = 0; i < BLOCK_ADDR_SIZE_BYTES; i++) {
      // Add the page annotations for StorageBlock data.
      nxt_dat_addr = (nxt_dat_addr << 8) + *(buf + i);
    }
    buffer_ptr = buf + BLOCK_ADDR_SIZE_BYTES;
    bytes_remaining -= BLOCK_ADDR_SIZE_BYTES;
  }

  if (0 < bytes_remaining) {
    // Any bytes remaining in this buffer are payload. Add to the accumulator.
    _outbound_buf.concat(buffer_ptr, bytes_remaining);
  }

  // If we have length information (we should), we can potentially know how many
  //   more reads need to be done.
  _blocks.insert(new StorageBlock(*addr, nxt_dat_addr));
  if (0 != nxt_dat_addr) {
    *addr = nxt_dat_addr;
    // TODO: If we want to support partial page reads...
    //*len = BYTES_NEXT_LEN;
    ret = 0;
  }

  if (0 != ret) {
    // This is the end of I/O for this record. Mark it as such,
    //   after inflating the object.
    _deserialize(&_outbound_buf, TCode::CBOR);
    _dr_clear_flag(DATA_RECORD_FLAG_PENDING_IO);
  }
  return ret;
}


/*
* Calculate and return the hash of the payload data.
*/
uint32_t DataRecord::_calculate_hash() {
  // TODO
  //https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
  return 0;
}



StorageBlock* DataRecord::_get_storage_block_by_addr(uint32_t addr) {
  for (int i = 0; i < _blocks.size(); i++) {
    StorageBlock* cur_block = _blocks.get(i);
    if (cur_block->this_offset == addr) {
      return cur_block;
    }
  }
  return nullptr;
}


StorageBlock* DataRecord::_get_storage_block_by_nxt(uint32_t addr) {
  for (int i = 0; i < _blocks.size(); i++) {
    StorageBlock* cur_block = _blocks.get(i);
    if (cur_block->next_offset == addr) {
      return cur_block;
    }
  }
  return nullptr;
}


/*
* Returns the number of bytes that the storage driver is already using to hold
*   this record.
*/
uint32_t DataRecord::_derive_allocated_size() {
  return (_blocks.size() * _storage->blockSize());
}


/*
* Fills this object with the descriptor values returned by the storage driver.
*/
int8_t DataRecord::_fill_from_descriptor_block(uint8_t* buf) {
  int8_t ret = 0;
  const uint8_t BLOCK_ADDR_SIZE_BYTES = _storage->blockAddrSize();
  _version = DATARECORD_SERIALIZER_VERSION;  // Always migrate old records if possible.
  _flags        = *(buf + 1);
  _record_type  = *(buf + 2);
  for (uint8_t i = 0; i < sizeof(_key); i++) {   *(buf + 3 + i) = _key[i];  }
  _hash = ((uint32_t) *(buf + 12))
    | ((uint32_t) *(buf + 13) << 8)
    | ((uint32_t) *(buf + 14) << 16)
    | ((uint32_t) *(buf + 15) << 24);
  _data_length = ((uint32_t) *(buf + 16))
    | ((uint32_t) *(buf + 17) << 8)
    | ((uint32_t) *(buf + 18) << 16)
    | ((uint32_t) *(buf + 19) << 24);
  _timestamp = ((uint32_t) *(buf + 20))
    | ((uint32_t) *(buf + 21) << 8)
    | ((uint32_t) *(buf + 22) << 16)
    | ((uint32_t) *(buf + 23) << 24)
    | ((uint32_t) *(buf + 24) << 32)
    | ((uint32_t) *(buf + 25) << 40)
    | ((uint32_t) *(buf + 26) << 48)
    | ((uint32_t) *(buf + 27) << 56);
  _nxt_rec_addr = 0;
  for (uint i = 0; i < BLOCK_ADDR_SIZE_BYTES; i++) {
    _nxt_rec_addr = (_nxt_rec_addr << 8) | *(buf + DATARECORD_BASE_SIZE + i);
  }
  return ret;
}
