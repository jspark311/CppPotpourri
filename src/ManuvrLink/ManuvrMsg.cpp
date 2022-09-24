/*
File:   ManuvrMsg.cpp
Author: J. Ian Lindsay
Date:   2021.10.09

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
#include "../BusQueue.h"

#if defined(CONFIG_C3P_M2M_SUPPORT)

/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

/**
* Take the giuven StringBuilder and try to put its bytes into their respective
*   slots in a header object.
* If we can do that, see if the header makes sense.
* If it does make sense, check for message completeness.
*
* This function will assume good sync, and a packet starting at offset zero.
*
* @param hdr A blank header object.
* @return -3 for no header found because the initial bytes are totally wrong. Sync error.
*         -2 for no header found because not enough bytes to complete it.
*         -1 for header found, but total size exceeds MTU.
*          0 for header found, but message incomplete.
*          1 for header found, and message complete with no payload.
*          2 for header found, and message complete with payload.
*/
int8_t ManuvrMsg::attempt_header_parse(ManuvrMsgHdr* hdr, StringBuilder* dat_in) {
  int8_t ret = -2;
  const int AVAILABLE_LEN = dat_in->length();
  if (AVAILABLE_LEN >= MANUVRMSGHDR_MINIMUM_HEADER_SIZE) {
    uint8_t* tmp_buf = dat_in->string();
    hdr->msg_code = (ManuvrMsgCode) *(tmp_buf++);
    hdr->flags    = *(tmp_buf++);

    const uint8_t len_l = hdr->len_length();
    const uint8_t id_l  = hdr->id_length();
    if (hdr->header_length() <= AVAILABLE_LEN) {
      // Write the multibyte value as big-endian.
      for (uint8_t i = 0; i < len_l; i++) hdr->msg_len = ((hdr->msg_len << 8) | *(tmp_buf++));
      for (uint8_t i = 0; i < id_l; i++)  hdr->msg_id  = ((hdr->msg_id << 8)  | *(tmp_buf++));
      hdr->chk_byte = *(tmp_buf++);
      ret = -3;
      if (hdr->chk_byte == hdr->calc_hdr_chcksm()) {       // Does the checksum match?
        ret = 1;
        if (0 < hdr->payload_length()) {
          ret = (hdr->total_length() > AVAILABLE_LEN) ? 0 : 2;
        }
      }
    }
  }
  return ret;
}


ManuvrMsg* ManuvrMsg::unserialize(StringBuilder* dat_in) {
  ManuvrMsg* ret = nullptr;
  ManuvrMsgHdr _header;
  int8_t ret_header = attempt_header_parse(&_header, dat_in);
  switch (ret_header) {
    case -3:  // no header found because the initial bytes are totally wrong. Sync error.
    case -2:  // no header found because not enough bytes to complete it. Wait for more accumulation.
      break;
    case -1:  // header found, but total size exceeds MTU.
    case 0:   // header found, but message incomplete.
    case 1:   // header found, and message complete with no payload.
    case 2:   // header found, and message complete with payload.
      dat_in->cull(_header.header_length());
      ret = new ManuvrMsg(&_header);
      if (nullptr != ret) {
        ret->accumulate(dat_in);
        //if (ret->rxComplete()) {
        //  ret->_unpack_payload();
        //}
      }
      break;
  }
  return ret;
}


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* Constructor for an outbound message.
*/
//ManuvrMsg::ManuvrMsg(KeyValuePair* kvp) : _header(ManuvrMsgCode::APPLICATION), _op(BusOpcode::TX) {
//  _kvp = kvp;
//}

/**
* Constructor for an inbound message.
*/
ManuvrMsg::ManuvrMsg(ManuvrMsgHdr* hdr, BusOpcode d) : _header(hdr), _op(d) {
}

/**
* Destructor.
*/
ManuvrMsg::~ManuvrMsg() {
}



/*******************************************************************************
* Exposed member functions.                                                    *
*******************************************************************************/

/**
* Marks this outbound message as having been sent to the output buffer.
*/
void ManuvrMsg::markSent() {
  _ms_io_mark = millis();
  _class_set_flag(MANUVRMSG_FLAG_TX_COMPLETE);
}


/**
* Sometimes we might want to re-use this allocated object rather than free it.
*/
void ManuvrMsg::wipe() {
  _op          = BusOpcode::UNDEF;
  _encoding    = TCode::BINARY;
  _flags       = 0;
  _header.wipe();
  _accumulator.clear();
}


bool ManuvrMsg::isValidMsg() {
  bool ret = _header.isValid();
  //switch (_op) {
  //  case BusOpcode::RX:
  //    // For RX, we are expected to have a full-and-complete header.
  //    if (_header.isValid()) {
  //      ret = true;
  //    }
  //    break;
  //  case BusOpcode::TX:
  //    // For TX,
  //    if (0 == _accumulator.length()) {
  //      ret = true;
  //    }
  //    break;
  //  default:
  //    break;
  //}
  return ret;
}


void ManuvrMsg::expectsReply(bool x) {
  if (x) {
    if (!_header.expectsReply() | (0 == _header.msg_id)) {
      // Assign IDs idempotently.
      _header.msg_id = randomUInt32();
    }
  }
  else {
    _header.msg_id = 0;
  }
  _header.expectsReply(x);
}


/*******************************************************************************
* Exposed member functions for Applications's use.                             *
*******************************************************************************/

/**
* The link calls this function on the assumption that it will resend if it
*   returns true, since it will decrement the retry count in that case.
*
* @return true if a send retry should be attempted.
*/
bool ManuvrMsg::attemptRetry() {
  if (_retries > 0) {
    _retries--;
    return true;
  }
  return false;
};


/**
* We need to reply to certain messages. This converts this message to a reply of
*   the message that it used to be. Then it can be simply fed back into the
*   outbound queue.
* Clears the accumulator, and reserializes the provided KVP into it.
*
* @param kvp is the new payload, if any. Nullptr is a valid value.
* @return 0 on success
*        -1 if the message isn't inbound.
*        -2 if there was a problem serializing the message.
*/
int ManuvrMsg::reply(KeyValuePair* kvp, bool reply_expected) {
  int ret = -1;
  if (BusOpcode::RX == _op) {
    // NOTE: No id check on purpose so that it also applies to SYNC_KA.
    ret--;
    _op = BusOpcode::TX;
    _header.expectsReply(reply_expected);
    _header.isReply(true);
    _accumulator.clear();
    _kvp = kvp;
    _class_clear_flag(MANUVRMSG_FLAG_ACCUMULATOR_COMPLETE);
    if (0 == serialize(&_accumulator)) {
      ret = 0;
    }
  }
  return ret;
}


// Application calls this to gain access to the message payload.
int ManuvrMsg::getPayload(KeyValuePair** payload) {
  int ret = -1;
  if (rxComplete()) {
    ret--;
    //if (nullptr == _kvp) {
      *payload = KeyValuePair::unserialize(_accumulator.string(), _accumulator.length(), _encoding);
      ret = 0;
    //}
  }
  return ret;
}


/**
* Link or application calls this to set the message payload.
* This will only work if the message is marked as being TX. If it is, it will
*   obliterate any data that might be in the accumulator, and alter the header
*   to fit the new situation.
*
* @param payload is the desired payload (if any). NULL is a valid input.
* @return 0 on success.
*        -1 on wrong type of message.
*/
int ManuvrMsg::setPayload(KeyValuePair* payload) {
  int ret = -1;
  switch (_op) {
    case BusOpcode::UNDEF:   // Might happen on a fresh message object.
      _op = BusOpcode::TX;   // If it happens, we make the assignment.
      // NOTE: No break;
    case BusOpcode::TX:
      _accumulator.clear();
      _class_clear_flag(MANUVRMSG_FLAG_ACCUMULATOR_COMPLETE);
      _kvp = payload;
      ret = (0 == serialize(&_accumulator)) ? 0 : -2;
      _kvp = nullptr;   // TODO: Clearly enforce memory contract with client classes.
      break;
    default:
      break;
  }
  return ret;
}


/**
* Sets the payload encoding scheme.
*
* @param enc is the desired payload encoding.
* @return 0 on success.
*        -1 invalid encoding type.
*/
int ManuvrMsg::encoding(TCode enc) {
  switch (enc) {
    case TCode::BINARY:
    case TCode::CBOR:
      _encoding = enc;
      return 0;
    default:
      return -1;
  }
}

/*******************************************************************************
* Exposed member functions for ManuvrLink's use.                               *
*******************************************************************************/

/**
* This function should be called by the link object to serialize the KVP into
*   the provided StringBuilder.
* If the accumulator length matches the length that the header claims it ought
*   to be, assume that the accumulator already contains the desired data, and
*   make a buffer copy, instead of trying to reserialize.
*
* @return 0 on success, nonzero on failure.
*/
int ManuvrMsg::serialize(StringBuilder* buf) {
  int ret = -1;
  if (_class_flag(MANUVRMSG_FLAG_ACCUMULATOR_COMPLETE)) {
    buf->concat(_accumulator.string(), _accumulator.length());
    ret = 0;
  }
  else {
    StringBuilder payload;
    int payload_len = 0;
    if (nullptr != _kvp) {
      ret--;
      if (0 == _kvp->serialize(&payload, _encoding)) {
        ret--;
        payload_len = payload.length();
      }
    }
    if (_header.set_payload_length(payload_len)) {
      ret--;
      StringBuilder header;
      if (_header.serialize(&header)) {
        buf->concatHandoff(&header);
        if (!payload.isEmpty()) {
          buf->concatHandoff(&payload);
        }
        _class_set_flag(MANUVRMSG_FLAG_ACCUMULATOR_COMPLETE);
        ret = 0;
      }
    }
  }
  return ret;
}


/**
* This function should be called by the link object to feed bytes to a message.
* This function will consume data from the input buffer, but might not consume
*   it all.
*
* @param buf is the container for the input data from the transport.
* @return 1 on accumulation with completion.
*         0 on accumulation without completion.
*        -1 on unserializer error.
*/
int ManuvrMsg::accumulate(StringBuilder* buf) {
  int ret = 1;
  int bytes_remaining = _header.payload_length() - _accumulator.length();
  int bytes_incoming  = buf->length();
  if (0 < bytes_remaining) {
    if (bytes_incoming <= bytes_remaining) {
      _accumulator.concatHandoff(buf);
      if (bytes_incoming < bytes_remaining) {
        ret = 0;
      }
    }
    else {   //
      _accumulator.concat(buf->string(), bytes_remaining);
      buf->cull(bytes_remaining);
    }
  }
  if (1 == ret) {
    if (nullptr == _kvp) {
      _kvp = KeyValuePair::unserialize(_accumulator.string(), _accumulator.length(), _encoding);
    }
  }
  return ret;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrMsg::printDebug(StringBuilder* output) {
  output->concatf(
    "    --- ManuvrMsg [%s: %s], %sid: %u %s\n",
    BusOp::getOpcodeString(_op),
    ManuvrLink::manuvMsgCodeStr(_header.msg_code),
    (_header.isReply() ? "reply to " : ""),
    _header.msg_id,
    (_header.expectsReply() ? "(need reply)" : "")
  );
  output->concatf("\t  %u bytes of %u expected payload with %s encoding.\n", _accumulator.length(), _header.payload_length(), typecodeToStr(_encoding));
  if (nullptr != _kvp) {
    output->concat("\t--- Payload -----------------------\n");
    _kvp->printDebug(output);
  }
}

#endif   // CONFIG_C3P_M2M_SUPPORT
