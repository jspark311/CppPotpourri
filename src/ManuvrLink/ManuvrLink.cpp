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

// We define the 4th sync byte.
#define XENOMSG_4TH_SYNC_BYTE (XENOMSG_MINIMUM_HEADER_SIZE + 0x10 + (uint8_t) ManuvrMsgCode::SYNC_KEEPALIVE + MANUVRLINK_SERIALIZATION_VERSION)

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

const char* ManuvrLink::manuvMsgCodeStr(const ManuvrMsgCode CODE) {
  switch (CODE) {
    case ManuvrMsgCode::UNDEFINED:          return "UNDEFINED";
    case ManuvrMsgCode::SYNC_KEEPALIVE:     return "SYNC_KEEPALIVE";
    case ManuvrMsgCode::CONNECT:            return "CONNECT";
    case ManuvrMsgCode::PROTOCOL:           return "PROTOCOL";
    case ManuvrMsgCode::AUTH_CHALLENGE:     return "AUTH_CHALLENGE";
    case ManuvrMsgCode::HANGUP:             return "HANGUP";
    case ManuvrMsgCode::DESCRIBE:           return "DESCRIBE";
    case ManuvrMsgCode::MSG_FORWARD:        return "MSG_FORWARD";
    case ManuvrMsgCode::LOG:                return "LOG";
    case ManuvrMsgCode::APPLICATION:        return "APPLICATION";
    default:                                return "<UNKNOWN>";
  }
}

const char* ManuvrLink::sessionStateStr(const ManuvrLinkState CODE) {
  switch (CODE) {
    case ManuvrLinkState::UNINIT:           return "UNINIT";
    case ManuvrLinkState::PENDING_SETUP:    return "PENDING_SETUP";
    case ManuvrLinkState::SYNC_BEGIN:       return "SYNC_BEGIN";
    case ManuvrLinkState::SYNC_CASTING:     return "SYNC_CASTING";
    case ManuvrLinkState::SYNC_TENTATIVE:   return "SYNC_TENTATIVE";
    case ManuvrLinkState::PENDING_AUTH:     return "PENDING_AUTH";
    case ManuvrLinkState::ESTABLISHED:      return "ESTABLISHED";
    case ManuvrLinkState::PENDING_HANGUP:   return "PENDING_HANGUP";
    case ManuvrLinkState::HUNGUP:           return "HUNGUP";
    case ManuvrLinkState::DISCONNECTED:     return "DISCONNECTED";
    default:                                return "<UNKNOWN>";
  }
}

/**
* Is the given message code valid? Used to do safe enum conversion.
*
* @param The ManuvrMsgCode code to test.
* @return true if so. False otherwise.
*/
const bool ManuvrLink::msgCodeValid(const ManuvrMsgCode CODE) {
  switch (CODE) {
    case ManuvrMsgCode::SYNC_KEEPALIVE:
    case ManuvrMsgCode::CONNECT:
    case ManuvrMsgCode::PROTOCOL:
    case ManuvrMsgCode::AUTH_CHALLENGE:
    case ManuvrMsgCode::HANGUP:
    case ManuvrMsgCode::DESCRIBE:
    case ManuvrMsgCode::MSG_FORWARD:
    case ManuvrMsgCode::LOG:
    case ManuvrMsgCode::APPLICATION:
      return true;
  }
  return false;
}

/**
* Is the given FSM code valid? Used to do safe enum conversion.
*
* @param The FSM code to test.
* @return true if so. False otherwise.
*/
static const bool _link_fsm_code_valid(const ManuvrLinkState CODE) {
  switch (CODE) {
    case ManuvrLinkState::UNINIT:
    case ManuvrLinkState::PENDING_SETUP:
    case ManuvrLinkState::SYNC_BEGIN:
    case ManuvrLinkState::SYNC_CASTING:
    case ManuvrLinkState::SYNC_TENTATIVE:
    case ManuvrLinkState::PENDING_AUTH:
    case ManuvrLinkState::ESTABLISHED:
    case ManuvrLinkState::PENDING_HANGUP:
    case ManuvrLinkState::HUNGUP:
    case ManuvrLinkState::DISCONNECTED:
      return true;
  }
  return false;
}


/**
* Scan a buffer for the protocol's sync pattern.
* Only call this function if sync is required, since it will disregard any
*   message boundaries in the data.
*
* @param dat_in  The buffer to search through.
* @return The offset of the first sync pattern, or -1 if the buffer contained no such pattern.
*/
static int _contains_sync_pattern(StringBuilder* dat_in) {
  int i = 0;
  uint8_t* buf = dat_in->string();
  int      len = dat_in->length();
  while (i < (len-3)) {
    if (*(buf + i + 0) == (uint8_t) ManuvrMsgCode::SYNC_KEEPALIVE) {
      if ((*(buf + i + 1) & XENOMSG_FLAG_SYNC_MASK) == 0x10) {
        if (*(buf + i + 2) == XENOMSG_MINIMUM_HEADER_SIZE) {
          if (*(buf + i + 3) == XENOMSG_4TH_SYNC_BYTE) {
            return i;
          }
        }
      }
    }
    i++;
  }
  return -1;
}


/**
* Scan a StringBuilder for the protocol's sync pattern, and remove any data
*   fitting the pattern, for as long as the pattern holds.
* Only call this function if sync is required, since it will disregard any
*   message boundaries in the data.
*
* @param dat_in  The buffer to search through and modify.
* @return The offset of the first byte that is NOT sync-stream.
*/
static void _cull_sync_data(StringBuilder* dat_in) {
  int i = 0;
  uint8_t* buf = dat_in->string();
  int      len = dat_in->length();
  while (i < (len-3)) {
    bool bail = (*(buf + i + 0) != (uint8_t) ManuvrMsgCode::SYNC_KEEPALIVE);
    bail |= ((*(buf + i + 1) & XENOMSG_FLAG_SYNC_MASK) != 0x10);
    bail |= (*(buf + i + 2) != XENOMSG_MINIMUM_HEADER_SIZE);
    bail |= (*(buf + i + 3) != XENOMSG_4TH_SYNC_BYTE);
    if (bail) {
      dat_in->cull(i);
      return;
    }
    i += 4;
  }
  if (0 < i) {
    dat_in->cull(i);
  }
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
* Constructor
*/
ManuvrLink::ManuvrLink(const ManuvrLinkOpts* opts) : _opts(opts) {
  _set_fsm_route(4, ManuvrLinkState::PENDING_SETUP, ManuvrLinkState::SYNC_BEGIN, ManuvrLinkState::SYNC_CASTING, ManuvrLinkState::SYNC_TENTATIVE);
  if (_flags.value(MANUVRLINK_FLAG_AUTH_REQUIRED)) {
    _append_fsm_route(2, ManuvrLinkState::PENDING_AUTH, ManuvrLinkState::ESTABLISHED);
  }
  else {
    _append_fsm_route(1, ManuvrLinkState::ESTABLISHED);
  }
}


/**
* Destructor
*/
ManuvrLink::~ManuvrLink() {
  _purge_inbound();
  _purge_outbound();

  if (nullptr != _working) {
    delete _working;
    _working = nullptr;
  }
}



/*******************************************************************************
* Exposed member functions.                                                    *
*******************************************************************************/

/*
* This should be called periodically to service events in the link.
*/
int8_t ManuvrLink::poll(StringBuilder* text_return) {
  uint32_t now = millis();
  _churn_inbound();
  _churn_outbound();
  _poll_fsm();
  // If we need to send an obligatory sync packet, do so.
  if (_flags.value(MANUVRLINK_FLAG_SYNC_CASTING)) {
    if (wrap_accounted_delta(_ms_last_send, now) > _opts.ms_keepalive) {
      if (0 == _send_sync_packet(true)) {
        _ms_last_send = now;
      }
    }
  }
  // Aggregate or trash any logs...
  if (_local_log.length() > 0) {
    // If the link has generated logs...
    if (nullptr != text_return) {
      // ...and the caller wants them relay them to the caller.
      text_return->concatHandoff(&_local_log);
    }
    else {
      _local_log.clear();
    }
  }
  return 0;
}



/**
* Take the accumulated bytes from the transport and try to put them into their
*   respective slots in a header object.
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
int8_t ManuvrLink::_attempt_header_parse(XenoMsgHeader* hdr) {
  int8_t ret = -2;
  const int AVAILABLE_LEN = _inbound_buf.length();
  if (AVAILABLE_LEN >= XENOMSG_MINIMUM_HEADER_SIZE) {
    uint8_t* tmp_buf = _inbound_buf.string();
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
        ret = -1;
        if (hdr->total_length() > _opts.mtu) {
          ret = 1;
          if (0 < hdr->payload_length()) {
            ret = (hdr->total_length() > AVAILABLE_LEN) ? 0 : 2;
          }
        }
      }
    }
  }

  if (_verbosity > 6) {
    _local_log.concatf("ManuvrLink (tag: 0x%x) _attempt_header_parse returned %d.\n", _session_tag, ret);
  }
  return ret;
}


/**
* When we take bytes from the transport, and can't use them all right away,
*   we store them to prepend to the next group of bytes that come through.
*
* @param buf Incoming data from the transport.
* @return -1 to reject buffer, 0 to accept without claiming, 1 to accept with claim.
*/
int8_t ManuvrLink::provideBuffer(StringBuilder* buf) {
  int8_t ret = 1;
  _ms_last_rec = millis();

  switch (_fsm_pos) {   // Consider the session state.
    case ManuvrLinkState::SYNC_BEGIN:
    case ManuvrLinkState::SYNC_CASTING:
      _inbound_buf.concatHandoff(buf);
      _inbound_buf.printDebug(&_local_log);
      if (_inbound_buf.length() >= 4) {
        switch (_process_for_sync(&_inbound_buf)) {
          case 0:   // No change in the inbound data.
            _flags.clear(MANUVRLINK_FLAG_SYNC_CASTING);
            break;
          case 1:   // Sync processed and input buffer altered.
            _local_log.concatf("Link 0x%x processed sync.\n", _session_tag);
            break;
          default:
            _local_log.concatf("Link 0x%x returned failure when sync processing.\n", _session_tag);
            break;
        }
      }
      break;

    case ManuvrLinkState::SYNC_TENTATIVE:  // We have exchanged sync packets with the counterparty.
    case ManuvrLinkState::PENDING_AUTH:
    case ManuvrLinkState::ESTABLISHED:    // The nominal case. Session is in-sync. Do nothing.
    case ManuvrLinkState::PENDING_HANGUP:
      _inbound_buf.concatHandoff(buf);
      _inbound_buf.printDebug(&_local_log);
      if (nullptr == _working) {
        XenoMsgHeader _header;
        switch (_attempt_header_parse(&_header)) {
          case -3:  // no header found because the initial bytes are totally wrong. Sync error.
            _fsm_insert_sync_states();
            _sync_losses++;
            break;
          case -2:  // no header found because not enough bytes to complete it. Wait for more accumulation.
            break;
          case -1:  // header found, but total size exceeds MTU.
            break;
          case 0:   // header found, but message incomplete.
          case 1:   // header found, and message complete with no payload.
          case 2:   // header found, and message complete with payload.
            _inbound_buf.cull(_header.header_length());
            _working = new XenoMessage(&_header);
            break;
        }
      }
      if (nullptr != _working) {
        _working->accumulate(&_inbound_buf);
        if (_working->rxComplete()) {
          if (_working->isValidMsg()) {
            _inbound_messages.insert(_working);
            _seq_parse_errs = 0;
          }
          else {
            _seq_parse_errs++;
            if (_seq_parse_errs >= MANUVRLINK_MAX_PARSE_FAILURES) {
              // If we failed to parse too many times in-a-row, we assume the
              //   session is desyncd. Delete the bad message, and steer the
              //   session toward re-sync.
              if (_verbosity > 5) {
                _local_log.concatf("ManuvrLink (tag: 0x%x) experienced a parse failure:\n", _session_tag);
                _working->printDebug(&_local_log);
              }
              _fsm_insert_sync_states();
              _sync_losses++;
            }
          }
          _working = nullptr;
        }
      }
      break;

    case ManuvrLinkState::UNINIT:
    case ManuvrLinkState::PENDING_SETUP: // We're getting data before we are setup to proc it.
    case ManuvrLinkState::HUNGUP:
    case ManuvrLinkState::DISCONNECTED:
      buf->clear();  // Drop the data.
      break;

    default:
      break;
  }

  return ret;
}


/**
* Debug support method.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrLink::printDebug(StringBuilder* output) {
  uint32_t now = millis();
  StringBuilder temp("ManuvrLink ");
  temp.concatf("(tag: 0x%x)", _session_tag);
  StringBuilder::styleHeader2(output, (const char*) temp.string());
  output->concatf("\tMTU:           %u\n", _opts.mtu);
  output->concatf("\tTimeout:       %ums\n", _opts.ms_timeout);
  output->concatf("\tLast outbound: %ums ago\n", (now - _ms_last_send));
  output->concatf("\tLast inbound:  %ums ago\n", (now - _ms_last_rec));
  output->concatf("\tEncoding:      %s\n", typecodeToStr(_opts.encoding));
  output->concatf("\tSync losses:   %u\n", _sync_losses);
  output->concatf("\tACK timeouts:  %u\n", _seq_ack_fails);
  output->concatf("\tBuffer size:   %u\n", _inbound_buf.length());
  printFSM(output);

  int x = _outbound_messages.size();
  if (x > 0) {
    output->concatf("\n-- Outbound Queue %d total, showing top %d ------------\n", x, MANUVRLINK_MAX_QUEUE_PRINT);
    int max_print = (x < MANUVRLINK_MAX_QUEUE_PRINT) ? x : MANUVRLINK_MAX_QUEUE_PRINT;
    for (int i = 0; i < max_print; i++) {
        _outbound_messages.get(i)->printDebug(output);
    }
  }

  x = _inbound_messages.size();
  if (x > 0) {
    output->concatf("\n-- Inbound Queue %d total, showing top %d -------------\n", x, MANUVRLINK_MAX_QUEUE_PRINT);
    int max_print = (x < MANUVRLINK_MAX_QUEUE_PRINT) ? x : MANUVRLINK_MAX_QUEUE_PRINT;
    for (int i = 0; i < max_print; i++) {
      _inbound_messages.get(i)->printDebug(output);
    }
  }

  if (_working) {
    output->concat("\n-- XenoMessage in process  ----------------------------\n");
    _working->printDebug(output);
  }
  output->concat('\n');
}


/**
* Debug support method.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrLink::printFSM(StringBuilder* output) {
  bool keep_looping = true;
  int i = 0;
  output->concatf("\tPrior state:   %s\n", sessionStateStr(_fsm_pos_prior));
  output->concatf("\tCurrent state: %s%s\n\tNext states:   ", sessionStateStr(_fsm_pos), _fsm_is_waiting() ? " (LOCKED)":" ");
  while (keep_looping & (i < MANUVRLINK_FSM_WAYPOINT_DEPTH)) {
    if (ManuvrLinkState::UNINIT == _fsm_waypoints[i]) {
      output->concat("<STABLE>");
      keep_looping = false;
    }
    else {
      output->concatf("%s, ", sessionStateStr(_fsm_waypoints[i]));
    }
    i++;
  }
  if (_fsm_is_waiting()) {
    output->concatf("\tFSM locked for another %ums\n", _fsm_lockout_ms - millis());
  }
  output->concat('\n');
}



/*******************************************************************************
* Functions for managing dialogs and message queues.                           *
*******************************************************************************/

/**
* Empties the outbound message queue (those bytes designated for the transport).
*
* @return  int The number of outbound messages that were purged.
*/
int ManuvrLink::_purge_outbound() {
  int return_value = _outbound_messages.size();
  while (_outbound_messages.hasNext()) {
    XenoMessage* temp = _outbound_messages.dequeue();
    delete temp;
  }
  return return_value;
}


/**
* Empties the inbound message queue (those bytes from the transport that we need to proc).
*
* @return  int The number of inbound messages that were purged.
*/
int ManuvrLink::_purge_inbound() {
  int return_value = _inbound_messages.size();
  while (_inbound_messages.hasNext()) {
    XenoMessage* temp = _inbound_messages.dequeue();
    delete temp;
  }
  return return_value;
}


int8_t ManuvrLink::_churn_inbound() {
  int8_t ret = 0;
  while (_inbound_messages.hasNext()) {
    XenoMessage* temp = _inbound_messages.dequeue();
    switch (temp->msgCode()) {
      case ManuvrMsgCode::SYNC_KEEPALIVE:
        // We got a sync message. Enter a sync state.
        _fsm_insert_sync_states();
        break;

      case ManuvrMsgCode::CONNECT:
      case ManuvrMsgCode::PROTOCOL:
      case ManuvrMsgCode::AUTH_CHALLENGE:
        break;
      case ManuvrMsgCode::HANGUP:
        // The other side wants to hang up. ACK if needed.
        break;
      case ManuvrMsgCode::DESCRIBE:
      case ManuvrMsgCode::MSG_FORWARD:
        break;
      case ManuvrMsgCode::LOG:
        // Allow the counterparty to write to our session log.
        break;

      case ManuvrMsgCode::APPLICATION:
        _invoke_msg_callback(temp);
        _clear_waiting_send_by_id(temp->uniqueId());
        break;

      default:   // This should never happen.
        break;
    }
  }
  return ret;
}


/*
* Go through the outbound queue, looking for timeout violations.
*/
int8_t ManuvrLink::_churn_outbound() {
  int8_t ret = 0;
  for (int i = 0; i < _outbound_messages.size(); i++) {
    XenoMessage* temp = _outbound_messages.get(i);
    if (nullptr != temp) {
      if (temp->wasSent()) {
        if (_opts.ms_timeout < temp->msSinceSend()) {
          // There is something in the outbound queue that has been waiting for
          //   a reply longer than the session timeout. Resend, or fail it.
          _seq_ack_fails++;
          //_outbound_messages.remove(temp);
          //delete temp;
        }
      }
      else {
        // Send it, and mark it as having been sent.
        StringBuilder temp_out;
        if (0 == temp->serialize(&temp_out)) {
          _relay_to_output_target(&temp_out);
          _clear_waiting_reply_by_id(temp->uniqueId());
          if (!temp->expectsReply()) {
            _outbound_messages.remove(temp);
            delete temp;
          }
          else {
            temp->markSent();
          }
        }
      }
    }
  }
  return ret;
}


/**
* Calling this function with the ID of a message we previously received will
*   cause that message to be released from the inbound queue.
*/
int8_t ManuvrLink::_clear_waiting_reply_by_id(uint32_t id) {
  int8_t ret = 0;
  for (int i = 0; i < _inbound_messages.size(); i++) {
    XenoMessage* temp = _inbound_messages.get(i);
    if (nullptr != temp) {
      if (id == temp->uniqueId()) {
        _inbound_messages.remove(temp);
        delete temp;
        ret = 1;
      }
    }
  }
  return ret;
}


/**
* Calling this function with the ID of a message we previously sent will cause
*   that message to be released from the outbound queue, and is tantamount to
*   satisfying the reply.
*/
int8_t ManuvrLink::_clear_waiting_send_by_id(uint32_t id) {
  int8_t ret = 0;
  for (int i = 0; i < _outbound_messages.size(); i++) {
    XenoMessage* temp = _outbound_messages.get(i);
    if (nullptr != temp) {
      if (id == temp->uniqueId()) {
        _outbound_messages.remove(temp);
        delete temp;
        ret = 1;
      }
    }
  }
  return ret;
}


int8_t ManuvrLink::_fsm_insert_sync_states() {
  int8_t ret = -1;
  if (0 == _prepend_fsm_state(ManuvrLinkState::SYNC_TENTATIVE)) {
    if (0 == _prepend_fsm_state(ManuvrLinkState::SYNC_CASTING)) {
      if (0 == _prepend_fsm_state(ManuvrLinkState::SYNC_BEGIN)) {
        ret = 0;
      }
    }
  }
  return ret;
}



int8_t ManuvrLink::_relay_to_output_target(StringBuilder* buf) {
  int8_t ret = -1;
  if (nullptr != _output_target) {
    ret = 0;
    switch (_output_target->provideBuffer(buf)) {
      case 0:
        buf->clear();
        // NOTE: No break;
      case 1:
        _ms_last_send = millis();
        ret = 1;
        break;
      default:
        ret = -2;
        break;
    }
  }
  if ((0 > ret) & (3 < _verbosity)) _local_log.concatf("Link 0x%x failed in _relay_to_output_target(): %d\n", _session_tag, ret);
  return ret;
}


int8_t ManuvrLink::_invoke_msg_callback(XenoMessage* msg) {
  int8_t ret = 0;
  if (nullptr != _msg_callback) {   // Call the callback, if it is set.
    _msg_callback(_session_tag, msg);
    ret++;
  }
  return ret;
}



/*******************************************************************************
* Functions for managing and reacting to sync states.                          *
*******************************************************************************/

/**
* Given a StringBuilder full of incoming data, search for and remove sync data.
* Culls the sync-related leading bytes of the buffer, if found.
* Initiates a sync reply, if needed.
*
* @param dat_in  The buffer to search through.
* @return 0 if no change. -1 on failure. 1 on sync processed and input buffer altered.
*/
int ManuvrLink::_process_for_sync(StringBuilder* dat_in) {
  int ret = 0;
  int i = _contains_sync_pattern(dat_in);
  if (0 <= i) {
    ret = 1;  // Found sync data, and we are about to change the buffer.
    _flags.set(MANUVRLINK_FLAG_SYNC_INCOMING);
    _cull_sync_data(dat_in);
  }
  else {
    // Without finding a sync packet, we drop the data.
    dat_in->clear();
  }
  return ret;
}


int8_t ManuvrLink::_send_sync_packet(bool need_reply) {
  int8_t ret = -1;
  StringBuilder sync_packet;
  XenoMsgHeader sync_header(ManuvrMsgCode::SYNC_KEEPALIVE, 0, (need_reply ? XENOMSG_FLAG_EXPECTING_REPLY : XENOMSG_FLAG_IS_REPLY));
  if (sync_header.serialize(&sync_packet)) {
    ret = (0 < _relay_to_output_target(&sync_packet)) ? 0 : -2;
  }
  else if (3 < _verbosity) _local_log.concatf("Link 0x%x failed to serialize a sync header.\n", _session_tag);
  return ret;
}



/*******************************************************************************
* FSM functions
*******************************************************************************/

/**
* Considers the current link state, and decides whether or not to advance the
*   state machine.
* NOTE: This function does not plan state machine routes, and should thus not
*   call _set_fsm_position() directly. Only _advance_state_machine().
*
* @return  1 on state shift
*          0 on no action
*         -1 on error
*/
int8_t ManuvrLink::_poll_fsm() {
  int8_t ret = 0;
  bool fsm_advance = false;
  switch (_fsm_pos) {
    // Exit conditions: Class config is valid, and we have all the pointers we
    //   need.
    case ManuvrLinkState::UNINIT:
      fsm_advance = ((nullptr != _output_target) & (nullptr != _msg_callback));
      break;

    // Exit conditions: The class has seen the first data for this session.
    case ManuvrLinkState::PENDING_SETUP:
      fsm_advance = true;
      //fsm_advance = (0 < _inbound_buf.length());
      break;

    // Exit conditions:
    case ManuvrLinkState::SYNC_BEGIN:
      fsm_advance = true;
      break;

    // Exit conditions:
    case ManuvrLinkState::SYNC_CASTING:
      fsm_advance = !_flags.value(MANUVRLINK_FLAG_SYNC_CASTING);
      break;

    // Exit conditions: Incoming data is no longer preceeded by sync packets.
    case ManuvrLinkState::SYNC_TENTATIVE:
      fsm_advance = true;
      break;

    // Exit conditions: An acceptable authentication has happened.
    case ManuvrLinkState::PENDING_AUTH:
      fsm_advance = true;
      break;

    // Exit conditions: These states are canonically stable. So we advance when
    //   the state is not stable (the driver has somewhere else it wants to be).
    case ManuvrLinkState::ESTABLISHED:
      fsm_advance = !_fsm_is_stable();
      break;

    // Exit conditions:
    case ManuvrLinkState::PENDING_HANGUP:
      break;

    // Exit conditions:
    case ManuvrLinkState::HUNGUP:
      break;

    // Exit conditions:
    case ManuvrLinkState::DISCONNECTED:
      break;

    default:   // Can't exit from an unknown state.
      ret = -1;
      break;
  }

  // If the current state's exit criteria is met, we advance the FSM.
  if (fsm_advance & (-1 != ret)) {
    ret = (0 == _advance_state_machine()) ? 1 : 0;
  }
  return ret;
}


/**
* Takes actions appropriate for entry into the given state, and sets the current
*   FSM position if successful. Records the existing state as having been the
*   prior state.
* NOTE: Except in edge-cases, this function should ONLY be
*   called by _advance_state_machine().
*
* @param The FSM code to test.
* @return 0 on success, -1 otherwise.
*/
int8_t ManuvrLink::_set_fsm_position(ManuvrLinkState new_state) {
  int8_t fxn_ret = -1;
  int8_t ret = -1;
  uint32_t now = millis();
  bool state_entry_success = false;   // Fail by default.
  if (!_fsm_is_waiting()) {
    switch (new_state) {

      // Entry into PENDING_SETUP means that the class has been wiped, and the
      //   values we depend upon later have been validated.
      case ManuvrLinkState::PENDING_SETUP:
        _reset_class();
        _session_tag = randomUInt32();
        state_entry_success = (_session_tag != 0);
        break;

      // Entry into SYNC_BEGIN means we trash any unprocessed inbound data, and
      //   begin expecting sync packets. Entry always succeeds.
      case ManuvrLinkState::SYNC_BEGIN:
        _inbound_buf.clear();
        _flags.clear(MANUVRLINK_FLAG_SYNC_CASTING | MANUVRLINK_FLAG_SYNC_INCOMING);
        state_entry_success = true;
        break;

      // Entry into SYNC_CASTING means we begin blindly emitting sync on the
      //   default timeout interval.
      case ManuvrLinkState::SYNC_CASTING:
        state_entry_success = (0 == _send_sync_packet(true));
        _flags.set(MANUVRLINK_FLAG_SYNC_CASTING, state_entry_success);
        break;

      // Entry into SYNC_CASTING requires that sync packets have been exchanged,
      //   and non-sync data has not yet been located.
      case ManuvrLinkState::SYNC_TENTATIVE:
        _flags.clear(MANUVRLINK_FLAG_SYNC_INCOMING);
        state_entry_success = true;
        break;

      case ManuvrLinkState::PENDING_AUTH:
        state_entry_success = true;
        break;

      case ManuvrLinkState::ESTABLISHED:
        state_entry_success = true;
        break;

      case ManuvrLinkState::PENDING_HANGUP:
        state_entry_success = true;
        break;

      // Entry into HUNGUP involves clearing/releasing any buffers and states
      //   from the prior session.
      case ManuvrLinkState::HUNGUP:
        _reset_class();
        break;

      case ManuvrLinkState::DISCONNECTED:
        state_entry_success = true;
        break;

      // Entry into any other state is disallowed.
      default:
        break;
    }

    if (state_entry_success) {
      if (3 < _verbosity) _local_log.concatf("Link 0x%x moved %s ---> %s\n", _session_tag, sessionStateStr(_fsm_pos), sessionStateStr(new_state));
      _fsm_pos_prior = _fsm_pos;
      _fsm_pos       = new_state;
      fxn_ret = 0;
    }
  }
  return fxn_ret;
}


/**
* Internal function responsible for advancing the state machine.
* NOTE: This function does no checks for IF the FSM should move forward. It only
*   performs the actions required to do it.
* Although this function is sometimes called directly by function other than
*   _poll_fsm(), the comprehensibility of the code requires that we keep this
*   to a minimum.
*
* @return 0 on state change, -1 otherwise.
*/
int8_t ManuvrLink::_advance_state_machine() {
  int8_t ret = -1;
  if (ManuvrLinkState::UNINIT != _fsm_waypoints[0]) {
    if (0 == _set_fsm_position(_fsm_waypoints[0])) {
      ret = 0;
      for (int i = 0; i < (MANUVRLINK_FSM_WAYPOINT_DEPTH-1); i++) {
        _fsm_waypoints[i] = _fsm_waypoints[i+1];
      }
      _fsm_waypoints[MANUVRLINK_FSM_WAYPOINT_DEPTH-1] = ManuvrLinkState::UNINIT;
    }
  }
  return ret;
}


/*
* This function checks each state code for validity, but does not error-check
*   the validity of the FSM traversal route specified in the arguments. It just
*   adds them to the list if they all correspond to valid state codes.
* This function will accept a maximum of sizeof(_fsm_waypoints) arguments, and
*   will clobber the contents of that member if the call succeeds. Arguments
*   provided in excess of the limit will be truncated with no error.
*
* @return 0 on success, -1 on no params, -2 on invalid FSM code.
*/
int8_t ManuvrLink::_set_fsm_route(int arg_count, ...) {
  int8_t ret = -1;
  const int PARAM_COUNT = strict_min((int8_t) arg_count, (int8_t) MANUVRLINK_FSM_WAYPOINT_DEPTH);
  if (PARAM_COUNT > 0) {
    va_list args;
    va_start(args, arg_count);
    ManuvrLinkState test_values[PARAM_COUNT] = {ManuvrLinkState::UNINIT, };
    ret = 0;
    for (int i = 0; i < PARAM_COUNT; i++) {
      test_values[i] = (ManuvrLinkState) va_arg(args, int);
    }
    va_end(args);   // Close out the va_args, and error-check each value.
    for (int i = 0; i < PARAM_COUNT; i++) {
      if (!_link_fsm_code_valid(test_values[i])) {
        ret = -2;
      }
    }
    if (0 == ret) {
      // If everything looks good, add items to the state traversal list, and
      //   zero the remainder.
      for (int i = 0; i < MANUVRLINK_FSM_WAYPOINT_DEPTH; i++) {
        _fsm_waypoints[i] = (i < PARAM_COUNT) ? test_values[i] : ManuvrLinkState::UNINIT;
      }
    }
  }
  return ret;
}


/*
* This function checks each state code for validity, but does not error-check
*   the validity of the FSM traversal route specified in the arguments. It just
*   adds them to the list if they all correspond to valid state codes.
* This function will accept a maximum of sizeof(_fsm_waypoints) arguments, and
*   will append to the contents of that member if the call succeeds. Arguments
*   provided in excess of the limit will be truncated with no error.
*
* @return 0 on success, -1 on no params, -2 on invalid FSM code.
*/
int8_t ManuvrLink::_append_fsm_route(int arg_count, ...) {
  int8_t ret = -1;
  const int PARAM_COUNT = strict_min((int8_t) arg_count, (int8_t) MANUVRLINK_FSM_WAYPOINT_DEPTH);
  if (PARAM_COUNT > 0) {
    va_list args;
    va_start(args, arg_count);
    ManuvrLinkState test_values[PARAM_COUNT] = {ManuvrLinkState::UNINIT, };
    ret = 0;
    for (int i = 0; i < PARAM_COUNT; i++) {
      test_values[i] = (ManuvrLinkState) va_arg(args, int);
    }
    va_end(args);   // Close out the va_args, and error-check each value.
    for (int i = 0; i < PARAM_COUNT; i++) {
      if (!_link_fsm_code_valid(test_values[i])) {
        ret = -2;
      }
    }
    if (0 == ret) {
      // If everything looks good, seek to the end of the state traversal list,
      //   and append.
      uint8_t fidx = 0;
      while ((fidx < MANUVRLINK_FSM_WAYPOINT_DEPTH) && (ManuvrLinkState::UNINIT != _fsm_waypoints[fidx])) {
        fidx++;
      }
      const uint8_t PARAMS_TO_COPY = strict_min((uint8_t)(MANUVRLINK_FSM_WAYPOINT_DEPTH - fidx), (uint8_t) PARAM_COUNT);
      for (int i = 0; i < PARAMS_TO_COPY; i++) {
        _fsm_waypoints[i + fidx] = test_values[i];
      }
    }
  }
  return ret;
}


int8_t ManuvrLink::_prepend_fsm_state(ManuvrLinkState nxt) {
  int8_t ret = -1;
  if (_link_fsm_code_valid(nxt)) {
    ret--;
    // If everything looks good, seek to the end of the state traversal list,
    //   and append.
    uint8_t fidx = 0;
    ManuvrLinkState last = _fsm_waypoints[0];
    while ((fidx < MANUVRLINK_FSM_WAYPOINT_DEPTH) && (ManuvrLinkState::UNINIT != _fsm_waypoints[fidx])) {
      last = _fsm_waypoints[fidx];
      _fsm_waypoints[fidx] = nxt;
      nxt = last;
      fidx++;
    }
    if (fidx < MANUVRLINK_FSM_WAYPOINT_DEPTH) {
      _fsm_waypoints[fidx] = nxt;
      ret = 0;
    }
  }
  return ret;
}


/**
* Resets the object to a fresh state in preparation for a new session.
*/
void ManuvrLink::_reset_class() {
  _inbound_buf.clear();
  _purge_inbound();
  _purge_outbound();
  if (nullptr != _working) {
    delete _working;
    _working = nullptr;
  }
  _session_tag    = 0;
  _ms_last_send   = 0;
  _ms_last_rec    = 0;
  _seq_parse_errs = 0;
  _seq_ack_fails  = 0;
  _sync_losses    = 0;
}



/*******************************************************************************
* High-level FSM fxns.
*******************************************************************************/

/**
* Public function to hang up on the counterparty.
*
* @param graceful should be true if the application wants to be polite.
* @return 0 on success, nonzero otherwise.
*/
int8_t ManuvrLink::hangup(bool graceful) {
  int8_t ret = -1;

  switch (_fsm_pos) {
    case ManuvrLinkState::PENDING_SETUP:
    case ManuvrLinkState::SYNC_BEGIN:
    case ManuvrLinkState::SYNC_CASTING:
    case ManuvrLinkState::SYNC_TENTATIVE:
    case ManuvrLinkState::PENDING_AUTH:
    case ManuvrLinkState::ESTABLISHED:
      if (graceful) {
        ret = _append_fsm_route(3, ManuvrLinkState::PENDING_HANGUP, ManuvrLinkState::HUNGUP, ManuvrLinkState::PENDING_SETUP);
      }
      else {
        // If we just want to kill the connection with no delay, we won't bother
        //   with the PENDING_HANGUP state.
        ret = _append_fsm_route(2, ManuvrLinkState::HUNGUP, ManuvrLinkState::PENDING_SETUP);
      }
      break;

    // We might be seeing a repeat call from the application.
    case ManuvrLinkState::PENDING_HANGUP:
    case ManuvrLinkState::HUNGUP:
      break;

    default:
      break;
  }

  return ret;
}



/*******************************************************************************
* FSM state accessor functions.
*******************************************************************************/

/**
* Is this object out of the setup phase and exchanging data?
*
* @return true if so. False otherwise.
*/
bool ManuvrLink::isConnected() {
  switch (_fsm_pos) {
    case ManuvrLinkState::SYNC_BEGIN:
    case ManuvrLinkState::SYNC_CASTING:
    case ManuvrLinkState::SYNC_TENTATIVE:
    case ManuvrLinkState::PENDING_AUTH:
    case ManuvrLinkState::ESTABLISHED:
    case ManuvrLinkState::PENDING_HANGUP:
      return true;
  }
  return false;
}


/**
* Is the link idle? Not connected implies not idle.
* Empty buffers. Empty message queues. In sync.
*
* @return true if so. False otherwise.
*/
bool ManuvrLink::linkIdle() {
  if (ManuvrLinkState::ESTABLISHED == _fsm_pos) {
    if (0 < _outbound_messages.size()) {
      if (0 < _inbound_messages.size()) {
        if (nullptr == _working) {
          if (_inbound_buf.isEmpty()) {
            return true;
          }
        }
      }
    }
  }
  return false;
}


/**
* Is this object syncd with a remote version of itself?
*
* @return true if so. False otherwise.
*/
bool ManuvrLink::_link_syncd() {
  switch (_fsm_pos) {
    case ManuvrLinkState::PENDING_AUTH:
    case ManuvrLinkState::ESTABLISHED:
    case ManuvrLinkState::PENDING_HANGUP:
      return true;
  }
  return false;
}

bool ManuvrLink::_fsm_is_waiting() {
  bool ret = false;
  if (0 != _fsm_lockout_ms) {
    ret = !(millis() >= _fsm_lockout_ms);
    if (!ret) {
      _fsm_lockout_ms = 0;
    }
  }
  return ret;
}






/*****THE LINE OF FISSION******************************************************/

XenoMsgHeader::XenoMsgHeader() : msg_code(ManuvrMsgCode::UNDEFINED), flags(0), chk_byte(0), msg_len(0), msg_id(0) {}

XenoMsgHeader::XenoMsgHeader(ManuvrMsgCode m) : msg_code(m), flags(0), chk_byte(0), msg_len(0), msg_id(0) {}

XenoMsgHeader::XenoMsgHeader(ManuvrMsgCode m, uint8_t pl_len, uint8_t f, uint32_t i) :
    msg_code(m), flags(f & ~(XENOMSG_SETTABLE_FLAG_BITS)),
    chk_byte(0),
    msg_len(0),
    msg_id(i & 0x00FFFFFF)
{
  uint8_t calcd_id_sz = 0;
  if (msg_id > 0x00000000) calcd_id_sz++;
  if (msg_id > 0x000000FF) calcd_id_sz++;
  if (msg_id > 0x0000FFFF) calcd_id_sz++;
  flags = ((flags & ~XENOMSG_FLAG_ENCODES_ID_BYTES) | (calcd_id_sz << 6));

  uint8_t calcd_len_sz = 1;
  uint32_t needed_total_sz = calcd_id_sz + pl_len + XENOMSG_MINIMUM_HEADER_SIZE;
  if (needed_total_sz > 0x000000FF) calcd_len_sz++;
  if (needed_total_sz > 0x0000FFFE) calcd_len_sz++;
  if (needed_total_sz <= 0x00FFFFFD) {  // Anything larger than this is invalid.
    flags = ((flags & ~XENOMSG_FLAG_ENCODES_LENGTH_BYTES) | (calcd_len_sz << 4));
    msg_len = needed_total_sz;
    chk_byte = (uint8_t) (flags + msg_len + (uint8_t)msg_code + MANUVRLINK_SERIALIZATION_VERSION);
  }
}


void XenoMsgHeader::wipe() {
    msg_code = ManuvrMsgCode::UNDEFINED;
    flags    = 0;
    chk_byte = 0;
    msg_len  = 0;
    msg_id   = 0;
}


int XenoMsgHeader::header_length() {
  int ret = 0;
  uint8_t len_bytes = (flags & XENOMSG_FLAG_ENCODES_LENGTH_BYTES) >> 4;
  uint8_t id_bytes  = (flags & XENOMSG_FLAG_ENCODES_ID_BYTES) >> 6;
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



bool XenoMsgHeader::serialize(StringBuilder* buf) {
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


bool XenoMsgHeader::set_payload_length(uint32_t pl_len) {
  bool ret = false;
  uint8_t calcd_len_sz = 1;
  uint32_t needed_total_sz = id_length() + pl_len + XENOMSG_MINIMUM_HEADER_SIZE;
  if (needed_total_sz > 0x000000FF) calcd_len_sz++;
  if (needed_total_sz > 0x0000FFFE) calcd_len_sz++;
  if (needed_total_sz <= 0x00FFFFFD) {  // Anything larger than this is invalid.
    flags = ((flags & ~XENOMSG_FLAG_ENCODES_LENGTH_BYTES) | (calcd_len_sz << 4));
    msg_len = needed_total_sz;
    chk_byte = calc_hdr_chcksm();
    ret = true;
  }
  return ret;
}


bool XenoMsgHeader::isValid() {
  bool ret = false;
  if (flags == (flags & ~XENOMSG_FLAG_RESERVED_MASK)) {   // Reserved flag bits are 0?
    if (XENOMSG_MINIMUM_HEADER_SIZE >= header_length()) { // 4 bytes is the minimum header length.
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
            if (msg_len >= XENOMSG_MINIMUM_HEADER_SIZE) {   // If the total length is legal...
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


bool XenoMsgHeader::isSync() {
  bool ret = false;
  if (ManuvrMsgCode::SYNC_KEEPALIVE == msg_code) {
    if ((flags & XENOMSG_FLAG_SYNC_MASK) == 0x10) {
      if (msg_len == XENOMSG_MINIMUM_HEADER_SIZE) {
        ret = (chk_byte == calc_hdr_chcksm());
      }
    }
  }
  return false;
}

#endif   // CONFIG_MANUVR_M2M_SUPPORT
