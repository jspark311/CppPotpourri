/*
File:   SimpleDataRecord.cpp
Author: J. Ian Lindsay
Date:   2023.02.14

TODO: First-principles first. Mechanics will follow.
*/

#include "Storage.h"


uint32_t static_hash_wrapper_fxn(const uint8_t* BUF, const uint32_t LEN) {
    #if defined(__HAS_CRYPT_WRAPPER)
      // We'd prefer a cryptographic hash...
    #else
      // But we'll fall back to something simple if we don't have one.
      // https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
    #endif  // __HAS_CRYPT_WRAPPER
  return (0xAAAAAAAA ^ LEN);  // TODO
}


/*******************************************************************************
* SimpleDataRecord base-class functions
*******************************************************************************/
void SimpleDataRecord::printDebug(StringBuilder* output) {
  output->concatf("\t Tag:\t 0x%08x\n", _storage_tag);
  output->concatf("\t Key:\t %s\n", _key);
  output->concatf("\t Dirty:\t %c\n", isDirty() ? 'y':'n');
  output->concatf("\t Type:\t 0x%02x\n", recordType());
  output->concatf("\t Hash:\t 0x%08x\n", _hash);
  output->concatf("\t Len:\t %u\n", _data_length);
}


/**
* Craft a descriptor for this record.
*
* @return  0 on success
*/
int8_t SimpleDataRecord::_descriptor_serialize(StringBuilder* outbound_buf) {
  int8_t ret = 0;
  uint8_t rec_desc[DATARECORD_BASE_SIZE] = {0, };
  uint32_t idx = 0;
  // Storsge tag field.
  *(rec_desc + idx++) = (uint8_t) (_storage_tag >> 0);
  *(rec_desc + idx++) = (uint8_t) (_storage_tag >> 8);
  *(rec_desc + idx++) = (uint8_t) (_storage_tag >> 16);
  *(rec_desc + idx++) = (uint8_t) (_storage_tag >> 24);
  *(rec_desc + idx++) = DATARECORD_SERIALIZER_VERSION;
  *(rec_desc + idx++) = 0;
  *(rec_desc + idx++) = (uint8_t) _record_type;
  *(rec_desc + idx++) = (uint8_t) _format;
  for (uint8_t i = 0; i < sizeof(_key); i++) {  *(rec_desc + idx++) = _key[i]; }
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
  // Hash field.
  *(rec_desc + idx++) = (uint8_t) (_hash >> 0);
  *(rec_desc + idx++) = (uint8_t) (_hash >> 8);
  *(rec_desc + idx++) = (uint8_t) (_hash >> 16);
  *(rec_desc + idx++) = (uint8_t) (_hash >> 24);

  // Prepend the record descriptor and dispatch the I/O.
  outbound_buf->prepend(rec_desc, idx);
  return ret;
}


/**
* Fills this object with the descriptor values returned by the storage driver.
* This function does some simple error checking.
*
* @param buf is assumed to contain a descriptor. It must be at least so long.
* @return  0 on success
*         -1 on impossible data length
*         -2 on unsupported record version
*/
int8_t SimpleDataRecord::_descriptor_deserialize(StringBuilder* sb_buf) {
  int8_t ret = -1;
  uint8_t* buf = sb_buf->string();
  uint32_t len = sb_buf->length();
  uint32_t buf_idx = 0;

  if (DATARECORD_BASE_SIZE <= len) {
    ret--;
    _storage_tag = *(buf + buf_idx++);
    _storage_tag |= (((uint32_t) *(buf + buf_idx++)) << 8);
    _storage_tag |= (((uint32_t) *(buf + buf_idx++)) << 16);
    _storage_tag |= (((uint32_t) *(buf + buf_idx++)) << 24);
    _version = *(buf + buf_idx++);
    if (DATARECORD_SERIALIZER_VERSION == _version) {
      // NOTE: This is where the migration chain ought to go, if/when it becomes
      //   necessary.
      ret--;
      _flags       = *(buf + buf_idx++);
      if ((uint8_t) _record_type == *(buf + buf_idx++)) {
        ret--;
        _format      = (TCode) *(buf + buf_idx++);
        for (uint8_t i = 0; i < sizeof(_key); i++) {  _key[i] = *(buf + buf_idx++);  }
        _data_length  = ((uint32_t) *(buf + buf_idx++));
        _data_length |= ((uint32_t) *(buf + buf_idx++) << 8);
        _data_length |= ((uint32_t) *(buf + buf_idx++) << 16);
        _data_length |= ((uint32_t) *(buf + buf_idx++) << 24);
        _timestamp  = ((uint64_t) *(buf + buf_idx++));
        _timestamp |= ((uint64_t) *(buf + buf_idx++) << 8);
        _timestamp |= ((uint64_t) *(buf + buf_idx++) << 16);
        _timestamp |= ((uint64_t) *(buf + buf_idx++) << 24);
        _timestamp |= ((uint64_t) *(buf + buf_idx++) << 32);
        _timestamp |= ((uint64_t) *(buf + buf_idx++) << 40);
        _timestamp |= ((uint64_t) *(buf + buf_idx++) << 48);
        _timestamp |= ((uint64_t) *(buf + buf_idx++) << 56);
        _hash  = ((uint32_t) *(buf + buf_idx++));
        _hash |= ((uint32_t) *(buf + buf_idx++) << 8);
        _hash |= ((uint32_t) *(buf + buf_idx++) << 16);
        _hash |= ((uint32_t) *(buf + buf_idx++) << 24);
        ret = 0;
      }
    }
  }
  return ret;
}


/**
* This function must be called from a child class to prepend the actual payload
*   data with a descriptor so that this record can be re-loaded with confidence.
*
* @return  0 on success
*         -1 on illegal name
*         -2 on serializer failure of calling class
*         -3 on failure to serialize a descriptor
*/
int8_t SimpleDataRecord::save(uint32_t storage_tag, const char* name, StringBuilder* outbound_buf) {
  int8_t ret = -1;
  if (0 == _sanitize_name(name)) {
    ret--;
    _storage_tag = storage_tag;
    if (0 == serialize(outbound_buf, _format)) {
      ret--;
      _hash = static_hash_wrapper_fxn(outbound_buf->string(), outbound_buf->length());
      _data_length = outbound_buf->length();
      if (0 == _descriptor_serialize(outbound_buf)) {
        ret = 0;
      }
    }
  }
  return ret;
}


/**
* @return  0 on success
*         -1 on bad descriptor
*         -2 on wrong storage tag
*         -3 on hash mismatch
*         -4 on failure to deserialize
*/
int8_t SimpleDataRecord::load(uint32_t storage_tag, StringBuilder* inbound_buf) {
  int8_t ret = -1;
  if (0 == _descriptor_deserialize(inbound_buf)) {
    ret--;
    if (_storage_tag == storage_tag) {
      ret--;
      if (_hash == static_hash_wrapper_fxn((inbound_buf->string()+DATARECORD_BASE_SIZE), _data_length)) {
        // Hash matches. Cull the useless edges of the buffer...
        inbound_buf->cull(DATARECORD_BASE_SIZE);
        ret--;
        if (0 == deserialize(inbound_buf, _format)) {
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
int8_t SimpleDataRecord::_sanitize_name(const char* name) {
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


/*
* Calculate and return the hash of the payload data.
*/
uint32_t SimpleDataRecord::_calculate_hash() {
  uint32_t ret = 0;
  StringBuilder _record_bin;
  if (0 == serialize(&_record_bin, _format)) {
    ret = static_hash_wrapper_fxn(_record_bin.string(), _record_bin.length());
  }
  return ret;
}



/*
* TODO: This might get cut. Presently, this concern is being mostly handled by
*   the binary header code (which is simpler and more bounded). We will retain
*   this for now, since it is harmless, and makes results easy to decipher.
*/
int SimpleDataRecord::serialize_cbor_kvp_for_record(cbor::encoder* encoder) {
  int8_t ret = 0;
  encoder->write_string("meta");
  encoder->write_map(2);
    encoder->write_string("type");
    switch (recordType()) {
      case StorageRecordType::ROOT:           encoder->write_string("ROOT");           break;
      case StorageRecordType::KEY_LISTING:    encoder->write_string("KEY_LISTING");    break;
      case StorageRecordType::C3POBJ_ON_ICE:  encoder->write_string("C3POBJ_ON_ICE");  break;
      case StorageRecordType::LOG:            encoder->write_string("LOG");            break;
      case StorageRecordType::CONFIG_OBJ:     encoder->write_string("CONFIG_OBJ");     break;
      case StorageRecordType::FIRMWARE_BLOB:  encoder->write_string("FIRMWARE_BLOB");  break;
      default:                                encoder->write_string("unknwn");         break;
    }
    encoder->write_string("ts");
    encoder->write_int((uint64_t) timestamp());
  return ret;
}
