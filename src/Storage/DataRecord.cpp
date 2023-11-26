/*
File:   DataRecord.cpp
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


This is the basal implementation of a DataRecord to be used with Storage.
*/

#include "Storage.h"
#include "cbor-cpp/cbor.h"


const char* DataRecord::recordTypeStr(const StorageRecordType e) {
  switch (e) {
    case StorageRecordType::UNINIT:           return "UNINIT";
    case StorageRecordType::ROOT:             return "ROOT";
    case StorageRecordType::KEY_LISTING:      return "KEY_LISTING";
    case StorageRecordType::C3POBJ_ON_ICE:    return "C3POBJ_ON_ICE";
    case StorageRecordType::LOG:              return "LOG";
    case StorageRecordType::CONFIG_OBJ:       return "CONFIG_OBJ";
    case StorageRecordType::FIRMWARE_BLOB:    return "FIRMWARE_BLOB";
    case StorageRecordType::INVALID:          return "INVALID";
  }
  return "UNKNOWN";
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


/**
* Ensure that the state of the storage reflects that of this record.
* If the object was never stored, blocks will be allocated (or culled) and
*   metadata added to this object ahead of write initiation.
*
* @return  0 on success
*         -1 on illegal name
*         -2 on serializer failure
*         -3 on failure to allocate storage space
*         -4 on stack allocation failure
*         -5 on failure to write to storage
*/
int8_t DataRecord::save(char* name) {
  int8_t ret = -1;
  if (0 == _sanitize_name(name)) {
    ret--;
    _outbound_buf.clear();
    if (0 == serialize(&_outbound_buf, TCode::CBOR)) {
      ret--;
      // Craft a descriptor for this record. We will need our overhead region to
      //   be aligned and constant-length.
      const uint32_t BLOCK_SIZE        = _storage->blockSize();
      const uint32_t BLOCK_ADDR_SIZE   = _storage->blockAddrSize();
      const uint32_t DESCRIPTOR_SIZE   = DATARECORD_BASE_SIZE + (BLOCK_ADDR_SIZE << 1);
      const uint32_t PAYLOAD_SIZE      = _outbound_buf.length();
      const uint32_t TOTAL_RECORD_SIZE = DESCRIPTOR_SIZE + PAYLOAD_SIZE;
      //const uint32_t ALLOCATED_SIZE    = _derive_allocated_size();
      //const uint32_t BYTES_TO_TRIM     = ALLOCATED_SIZE - TOTAL_RECORD_SIZE;
      _data_length = _outbound_buf.length();   // Update the payload length field.
      _mark_clean();  // Finalize the checksum over the data.

      // Now to ensure we take up the right amount of space in storage.
      if (0 == _storage->allocateBlocksForLength(TOTAL_RECORD_SIZE, this)) {
        // If our request for space was granted, we will now have a block list
        //   that we are expected to write to. Get the first block.
        ret--;
        uint8_t rec_desc[DESCRIPTOR_SIZE];
        memset(rec_desc, 0, DESCRIPTOR_SIZE);

        if (nullptr != rec_desc) {
          ret--;
          StorageBlock* first_blk = _blocks.get(0);
          uint32_t _this_addr     = first_blk->this_offset;
          uint32_t _nxt_dat_addr  = first_blk->next_offset;

          // All the pieces are present. Start writing the descriptor.
          uint32_t idx = 0;
          *(rec_desc + idx++) = DATARECORD_SERIALIZER_VERSION;
          *(rec_desc + idx++) = 0;   // No flag fields yet.
          *(rec_desc + idx++) = (uint8_t) _record_type;
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
            const uint32_t IDX_CONST   = idx++;
            const uint32_t SHIFT_CONST = i << 3;
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
      else {
        // Storage driver couldn't change the size of this record. It probably
        //   failed to find enough free blocks when asked for a resize.
        // Clean up our mess and bail.
        _outbound_buf.clear();
      }
    }
  }
  return ret;
}


/**
* Ensure that this record reflects the state of the storage, preferring the
*   state of storage. Calling this function wipes the record state from the
*   object.
* Lookup requires a match of the key as well as a matching record_type. The
*   record type is imparted at construction, and the given name is truncated to
*   the size of the key, and zero-padded.
* The first record to be found with matching parameters is what will be returned.
*
* @param name is the name of the record to locate.
* @return 0 on success. Nonzero otherwise.
*/
int8_t DataRecord::load(char* name) {
  int8_t ret = -1;
  // Clear the class state and copy the given key as the search term.
  if (0 == _sanitize_name(name)) {
    ret--;
    _outbound_buf.clear();
    _blocks.clear();
    _data_length = 0;
    // To load a record we need a key and a record type.
    if ((_key[0] != 0) & (nullptr != _storage)) {
      if ((StorageRecordType::UNINIT != _record_type) & (StorageRecordType::INVALID != _record_type)) {
        ret--;
        //if (StorageErr::NONE == _storage->persistentRead((DataRecord*) this, _storage->blockSize(), 0)) {
        //  _dr_set_flag(DATA_RECORD_FLAG_PENDING_IO);
        //  ret = 0;
        //}
      }
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
    const uint32_t BYTES_REQUESTED = *len;
    const uint32_t BYTES_NEXT_SEND = strict_min((uint32_t) _outbound_buf.length(), (uint32_t) *len);
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
      for (uint32_t i = 0; i < BLOCK_ADDR_SIZE_BYTES; i++) {
        // Add the page annotations for StorageBlock data.
        *(buf + i) = (uint8_t) ((assoc_nxt_block >> (i << 3)) & 0xFF);
      }

      // Copy over as much payload data as we can to fill the buffer, and zero the rest.
      for (uint32_t i = 0; i < (BYTES_REQUESTED - BLOCK_ADDR_SIZE_BYTES); i++) {
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
  const uint32_t BLOCK_ADDR_SIZE   = _storage->blockAddrSize();
  const uint32_t DESCRIPTOR_SIZE   = DATARECORD_BASE_SIZE + (BLOCK_ADDR_SIZE << 1);
  const uint32_t BYTES_OFFERED     = *len;
  const uint8_t BLOCK_ADDR_SIZE_BYTES = _storage->blockAddrSize();
  uint32_t bytes_remaining = BYTES_OFFERED;
  uint8_t* buffer_ptr      = buf;
  uint32_t nxt_dat_addr    = 0;
  //uint32_t idx = 0;
  int8_t   ret = -1;

  // If this is the first buffer to arrive for an uninitialized record...
  if ((0 == _data_length) & (bytes_remaining >= DESCRIPTOR_SIZE)) {
    // If we haven't been loaded from NVM yet, check for a valid descriptor and
    //   compare our key and type against the ones we are looking for (ours).
    const uint8_t INCOMING_SER_VER = *(buf + 0);
    switch (INCOMING_SER_VER) {   // Version field, always.
      case DATARECORD_SERIALIZER_VERSION:   // Current version of serializer.
        {
          bool key_differs = false;
          for (uint32_t i = 0; i < sizeof(_key); i++) {
            key_differs |= (_key[i] == *(buf + DATARECORD_BASE_SIZE + i));
          }
          if (key_differs | (_record_type != (StorageRecordType) *(buf + 2))) {
            // If the key didn't match, instruct the Storage driver to get the
            //   next record block, rather than the next data block.
            for (uint32_t i = 0; i < BLOCK_ADDR_SIZE_BYTES; i++) {
              nxt_dat_addr = (nxt_dat_addr << 8) | *(buf + DATARECORD_BASE_SIZE + i);
            }
            *addr = nxt_dat_addr;
            return (0 == nxt_dat_addr) ? -1 : 0;   // Stop I/O if this is the last record.
          }
          else {
            // This is the correct record. Fill the class variables, and ask for
            //   the next block of data (instead of the next record).
            _fill_from_descriptor_block(buf);
            for (uint32_t i = 0; i < BLOCK_ADDR_SIZE_BYTES; i++) {
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
    for (uint32_t i = 0; i < BLOCK_ADDR_SIZE_BYTES; i++) {
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
    deserialize(&_outbound_buf, TCode::CBOR);
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


/**
* Calculates the number of bytes that the storage driver is already using to
*   hold this record. This is the actual size occupied in Storage. NOT the size
*   of the payload (which is smaller).
*
* @return the number of bytes presently allocated to this record.
*/
uint32_t DataRecord::_derive_allocated_size() {
  return (_blocks.size() * _storage->blockSize());
}


/**
* Fills this object with the descriptor values returned by the storage driver.
* This function does some simple error checking.
*
* @param buf is assumed to contain a descriptor. It must be at least so long.
* @return  0 on success
*         -1 on invalid record_type
*         -2 on impossible data length
*         -3 on invalid next_record address
*/
int8_t DataRecord::_fill_from_descriptor_block(uint8_t* buf) {
  const uint32_t DEV_BYTES_TOTAL       = _storage->deviceSize();
  const uint32_t BLOCK_SIZE            = _storage->blockSize();
  const uint8_t  BLOCK_ADDR_SIZE_BYTES = _storage->blockAddrSize();
  int8_t ret = -1;

  _version = DATARECORD_SERIALIZER_VERSION;  // Always migrate old records if possible.
  _flags        = *(buf + 1);
  _record_type  = (StorageRecordType) *(buf + 2);
  for (uint8_t i = 0; i < sizeof(_key); i++) {   *(buf + 3 + i) = _key[i];  }
  _hash = ((uint32_t) *(buf + 12))
    | ((uint32_t) *(buf + 13) << 8)
    | ((uint32_t) *(buf + 14) << 16)
    | ((uint32_t) *(buf + 15) << 24);
  _data_length = ((uint32_t) *(buf + 16))
    | ((uint32_t) *(buf + 17) << 8)
    | ((uint32_t) *(buf + 18) << 16)
    | ((uint32_t) *(buf + 19) << 24);
  _timestamp = ((uint64_t) *(buf + 20))
    | ((uint64_t) *(buf + 21) << 8)
    | ((uint64_t) *(buf + 22) << 16)
    | ((uint64_t) *(buf + 23) << 24)
    | ((uint64_t) *(buf + 24) << 32)
    | ((uint64_t) *(buf + 25) << 40)
    | ((uint64_t) *(buf + 26) << 48)
    | ((uint64_t) *(buf + 27) << 56);
  _nxt_rec_addr = 0;
  for (uint32_t i = 0; i < BLOCK_ADDR_SIZE_BYTES; i++) {
    _nxt_rec_addr = (_nxt_rec_addr << 8) | *(buf + DATARECORD_BASE_SIZE + i);
  }
  // Simple error checking.

  if ((StorageRecordType::UNINIT != _record_type) & (StorageRecordType::INVALID != _record_type)) {
    ret--;
    if (_data_length < DEV_BYTES_TOTAL) {
      ret--;
      if ((_nxt_rec_addr + BLOCK_SIZE) < DEV_BYTES_TOTAL) {
        if (0 == (_nxt_rec_addr % BLOCK_SIZE)) {
          ret = 0;
        }
      }
    }
  }
  return ret;
}

/**
* Given a non-null string pointer, check that the string conforms to our rules,
*   and replace our _key with the sanitized value.
* Rules:
*   * Names must be at least one character long.
*   * Names must be null-terminated.
*   * Names longer than the maximum length will be truncated without failing.
*
* @param name is the new name for this record, null-terminated.
* @return 0 on success. -1 on failure of rule-check.
*/
int8_t DataRecord::_sanitize_name(char* name) {
  const uint8_t NAME_LENGTH = strict_min((uint8_t) sizeof(_key), (uint8_t) strlen(name));
  int8_t ret = -1;
  if (NAME_LENGTH > 0) {
    for (uint8_t i = 0; i < sizeof(_key); i++) {
      _key[i] = (i < NAME_LENGTH) ? *(name + i) : 0;
    }
    ret = 0;
  }
  return ret;
}
