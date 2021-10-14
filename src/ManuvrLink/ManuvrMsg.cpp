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

/**
* Constructor for an outbound message.
*/
ManuvrMsg::ManuvrMsg(KeyValuePair* kvp) : _header(ManuvrMsgCode::APPLICATION), _op(BusOpcode::TX) {
  _kvp = kvp;
}

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
  bool ret = false;
  switch (_op) {
    case BusOpcode::RX:
      // For RX, we are expected to have a full-and-complete header.
      if (_header.isValid()) {
        ret = true;
      }
      break;
    case BusOpcode::TX:
      // For TX,
      if (0 == _accumulator.length()) {
      }
      if (0 == _accumulator.length()) {
        ret = true;
      }
      break;
    default:
      break;
  }
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
* We need to reply to certain messages. This converts this message to a reply of
*   the message that it used to be. Then it can be simply fed back into the
*   outbound queue.
*
* @return  nonzero if there was a problem.
*/
int ManuvrMsg::reply(KeyValuePair* kvp) {
  int ret = -1;
  if (BusOpcode::RX == _op) {
    ret--;
    if (_header.expectsReply()) {
      // NOTE: No id check on purpose so that it also applies to SYNC_KA.
      _op = BusOpcode::TX;
      _header.expectsReply(false);
      _header.isReply(true);
      _header.rebuild_checksum();
      ret = 0;
    }
  }
  return ret;
}


// Application calls this to gain access to the message payload.
int ManuvrMsg::getPayload(KeyValuePair**) {
  int ret = -1;
  return ret;
}


// Application calls this to set the message payload.
int ManuvrMsg::setPayload(KeyValuePair*) {
  int ret = -1;
  return ret;
}



/*******************************************************************************
* Exposed member functions for ManuvrLink's use.                               *
*******************************************************************************/

/**
* This function should be called by the link object to serialize the KVP into
*   the provided StringBuilder.
*
* @return 0 on success, nonzero on failure.
*/
int ManuvrMsg::serialize(StringBuilder* buf) {
  int ret = -1;
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
      ret = 0;
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
  output->concatf("\t ManuvrMsg [%s: %s], id: %u\n", BusOp::getOpcodeString(_op), ManuvrLink::manuvMsgCodeStr(_header.msg_code), _header.msg_id);
  output->concatf("\t   %u bytes of %u expected payload with %s encoding.\n", _accumulator.length(), _header.payload_length(), typecodeToStr(_encoding));
}

#endif   // CONFIG_MANUVR_M2M_SUPPORT
