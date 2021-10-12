/*
File:   ManuvrLink.cpp
Author: J. Ian Lindsay
Date:   2021.10.08

Copyright 2021 Manuvr, Inc

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

#include "ManuvrLink.h"
#include "BusQueue.h"

#if defined(CONFIG_MANUVR_M2M_SUPPORT)


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

ManuvrMsgHdr::ManuvrMsgHdr() : msg_code(ManuvrMsgCode::UNDEFINED), flags(0), chk_byte(0), msg_len(0), msg_id(0) {}

ManuvrMsgHdr::ManuvrMsgHdr(ManuvrMsgCode m) : msg_code(m), flags(0), chk_byte(0), msg_len(0), msg_id(0) {}

ManuvrMsgHdr::ManuvrMsgHdr(ManuvrMsgCode m, uint8_t pl_len, uint8_t f, uint32_t i) :
    msg_code(m), flags(f & ~(MANUVRMSGHDR_SETTABLE_FLAG_BITS)),
    chk_byte(0),
    msg_len(0),
    msg_id(i & 0x00FFFFFF)
{
  uint8_t calcd_id_sz = 0;
  if (msg_id > 0x00000000) calcd_id_sz++;
  if (msg_id > 0x000000FF) calcd_id_sz++;
  if (msg_id > 0x0000FFFF) calcd_id_sz++;
  flags = ((flags & ~MANUVRMSGHDR_FLAG_ENCODES_ID_BYTES) | (calcd_id_sz << 6));

  uint8_t calcd_len_sz = 1;
  uint32_t needed_total_sz = calcd_id_sz + pl_len + MANUVRMSGHDR_MINIMUM_HEADER_SIZE;
  if (needed_total_sz > 0x000000FF) calcd_len_sz++;
  if (needed_total_sz > 0x0000FFFE) calcd_len_sz++;
  if (needed_total_sz <= 0x00FFFFFD) {  // Anything larger than this is invalid.
    flags = ((flags & ~MANUVRMSGHDR_FLAG_ENCODES_LENGTH_BYTES) | (calcd_len_sz << 4));
    msg_len = needed_total_sz;
    chk_byte = (uint8_t) (flags + msg_len + (uint8_t)msg_code + MANUVRLINK_SERIALIZATION_VERSION);
  }
}


void ManuvrMsgHdr::wipe() {
    msg_code = ManuvrMsgCode::UNDEFINED;
    flags    = 0;
    chk_byte = 0;
    msg_len  = 0;
    msg_id   = 0;
}


/*******************************************************************************
* Accessors for lengths                                                        *
*******************************************************************************/

int ManuvrMsgHdr::header_length() {
  int ret = 0;
  uint8_t len_bytes = (flags & MANUVRMSGHDR_FLAG_ENCODES_LENGTH_BYTES) >> 4;
  uint8_t id_bytes  = (flags & MANUVRMSGHDR_FLAG_ENCODES_ID_BYTES) >> 6;
  if (len_bytes) {
    // Byte cost for header:
    // ManuvrMsgCode  1
    // Flags          1
    // Length field   (1, 3)   Length is a required field.
    // ID field       (0, 3)
    // Checksum byte  1
    ret = id_bytes + len_bytes + 3;
  }
  return ret;
}


bool ManuvrMsgHdr::set_payload_length(uint32_t pl_len) {
  bool ret = false;
  uint8_t calcd_len_sz = 1;
  uint32_t needed_total_sz = id_length() + pl_len + MANUVRMSGHDR_MINIMUM_HEADER_SIZE;
  if (needed_total_sz > 0x000000FF) calcd_len_sz++;
  if (needed_total_sz > 0x0000FFFE) calcd_len_sz++;
  if (needed_total_sz <= 0x00FFFFFD) {  // Anything larger than this is invalid.
    flags = ((flags & ~MANUVRMSGHDR_FLAG_ENCODES_LENGTH_BYTES) | (calcd_len_sz << 4));
    msg_len = needed_total_sz;
    chk_byte = calc_hdr_chcksm();
    ret = true;
  }
  return ret;
}


/*******************************************************************************
* Parser, Packer, and validity checking                                        *
*******************************************************************************/

bool ManuvrMsgHdr::serialize(StringBuilder* buf) {
  bool ret = isValid();
  if (ret) {
    const uint8_t len_l = len_length();
    const uint8_t id_l  = id_length();
    uint8_t header_bytes[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};  // Maximum size.
    uint8_t* tmp_buf = &header_bytes[0];
    *(tmp_buf++) = (uint8_t) msg_code;
    *(tmp_buf++) = (uint8_t) flags;

    // Write the multibyte value as big-endian.
    for (uint8_t i = 0; i < len_l; i++) {
      *(tmp_buf++) = (uint8_t) (msg_len >> (((len_l-1) - i) << 3));
    }

    // Write the multibyte value as big-endian.
    for (uint8_t i = 0; i < id_l; i++) {
      *(tmp_buf++) = (uint8_t) (msg_id >> (((id_l-1) - i) << 3));
    }

    *(tmp_buf++) = (uint8_t) chk_byte;
    buf->concat(header_bytes, header_length());
  }
  return ret;
}


bool ManuvrMsgHdr::isValid() {
  bool ret = false;
  if (flags == (flags & ~MANUVRMSGHDR_FLAG_RESERVED_MASK)) {   // Reserved flag bits are 0?
    if (MANUVRMSGHDR_MINIMUM_HEADER_SIZE >= header_length()) { // 4 bytes is the minimum header length.
      if (ManuvrLink::msgCodeValid(msg_code)) {           // Valid message code?
        uint8_t calcd_id_sz  = 0;
        uint8_t calcd_len_sz = 0;
        if (msg_id > 0x00000000) calcd_id_sz++;
        if (msg_id > 0x000000FF) calcd_id_sz++;
        if (msg_id > 0x0000FFFF) calcd_id_sz++;
        if (msg_id > 0x00FFFFFF) calcd_id_sz++;
        if (msg_len > 0x00000000) calcd_len_sz++;
        if (msg_len > 0x000000FF) calcd_len_sz++;
        if (msg_len > 0x0000FFFF) calcd_len_sz++;
        if (msg_len > 0x00FFFFFF) calcd_len_sz++;

        if (calcd_id_sz == id_length()) {                 // Is the ID field properly sized?
          if (calcd_id_sz == id_length()) {               // Is the len field properly sized?
            if (msg_len >= MANUVRMSGHDR_MINIMUM_HEADER_SIZE) {   // If the total length is legal...
              if (calcd_len_sz == len_length()) {           // Is the length field properly sized?
                if (chk_byte == calc_hdr_chcksm()) {       // Does the checksum match?
                }
                // Reply logic needs an ID if the message isn't a sync frame.
                if (ManuvrMsgCode::SYNC_KEEPALIVE != msg_code) {
                  ret = ((isReply() | expectsReply()) == (0 < id_length()));
                }
                else {   ret = true;   }
              }
            }
          }
        }
      }
    }
  }
  return ret;
}


bool ManuvrMsgHdr::isSync() {
  bool ret = false;
  if (ManuvrMsgCode::SYNC_KEEPALIVE == msg_code) {
    if ((flags & MANUVRMSGHDR_FLAG_SYNC_MASK) == 0x10) {
      if (msg_len == MANUVRMSGHDR_MINIMUM_HEADER_SIZE) {
        ret = (chk_byte == calc_hdr_chcksm());
      }
    }
  }
  return false;
}

#endif   // CONFIG_MANUVR_M2M_SUPPORT
