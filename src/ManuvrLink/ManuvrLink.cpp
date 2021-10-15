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

// Definitions only needed inside this translation unit.
#define MANUVRLINK_PRIORITY_WAITING_FOR_ACK  5   // These will not resend until and unless they timeout.
#define MANUVRLINK_PRIORITY_APP              10  // Application messages.
#define MANUVRLINK_PRIORITY_INTERNAL         20  // Classes own messages have highest priority.


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

const char* ManuvrLink::sessionStateStr(const ManuvrLinkState CODE) {
  switch (CODE) {
    case ManuvrLinkState::UNINIT:           return "UNINIT";
    case ManuvrLinkState::PENDING_SETUP:    return "PENDING_SETUP";
    case ManuvrLinkState::SYNC_RESYNC:      return "SYNC_RESYNC";
    case ManuvrLinkState::SYNC_TENTATIVE:   return "SYNC_TENTATIVE";
    case ManuvrLinkState::PENDING_AUTH:     return "PENDING_AUTH";
    case ManuvrLinkState::IDLE:             return "IDLE";
    case ManuvrLinkState::PENDING_HANGUP:   return "PENDING_HANGUP";
    case ManuvrLinkState::HUNGUP:           return "HUNGUP";
    default:                                return "<UNKNOWN>";
  }
}

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
    case ManuvrLinkState::SYNC_RESYNC:
    case ManuvrLinkState::SYNC_TENTATIVE:
    case ManuvrLinkState::PENDING_AUTH:
    case ManuvrLinkState::IDLE:
    case ManuvrLinkState::PENDING_HANGUP:
    case ManuvrLinkState::HUNGUP:
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
    const uint8_t b0 = *(buf + i + 0);
    const uint8_t b1 = *(buf + i + 1);
    const uint8_t b2 = *(buf + i + 2);
    const uint8_t b3 = *(buf + i + 3);
    const uint8_t EXPECTED_4TH_SYNC_BYTE = (b0 + b1 + b2 + MANUVRLINK_SERIALIZATION_VERSION);
    if (b0 == (uint8_t) ManuvrMsgCode::SYNC_KEEPALIVE) {
      if ((b1 & MANUVRMSGHDR_FLAG_SYNC_MASK) == 0x10) {
        if (b2 == MANUVRMSGHDR_MINIMUM_HEADER_SIZE) {
          if (b3 == EXPECTED_4TH_SYNC_BYTE) {
            return i;
          }
        }
      }
    }
    i++;
  }
  return -1;
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
* Implementation of BufferAccepter                                             *
*******************************************************************************/

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
    case ManuvrLinkState::SYNC_RESYNC:
    case ManuvrLinkState::SYNC_TENTATIVE:  // We have exchanged sync packets with the counterparty.
    case ManuvrLinkState::PENDING_AUTH:
    case ManuvrLinkState::IDLE:            // The nominal case. Session is in-sync. Do nothing.
    case ManuvrLinkState::PENDING_HANGUP:
      _inbound_buf.concatHandoff(buf);
      break;
    default:
      buf->clear();  // In any other case, drop the data.
      break;
  }

  return ret;
}


/*******************************************************************************
* Exposed member functions.                                                    *
*******************************************************************************/

/*
* This should be called periodically to service events in the link.
*/
int8_t ManuvrLink::poll(StringBuilder* text_return) {
  uint32_t now = millis();
  _process_input_buffer();
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
* Public function to hang up on the counterparty.
*
* @param graceful should be true if the application wants to be polite.
* @return 0 on success, nonzero otherwise.
*/
int8_t ManuvrLink::hangup(bool graceful) {
  int8_t ret = -1;

  switch (_fsm_pos) {
    case ManuvrLinkState::PENDING_SETUP:
    case ManuvrLinkState::SYNC_RESYNC:
    case ManuvrLinkState::SYNC_TENTATIVE:
    case ManuvrLinkState::PENDING_AUTH:
    case ManuvrLinkState::IDLE:
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


/**
* Is the link idle? Not connected implies not idle.
* Empty buffers. Empty message queues. In sync.
*
* @return true if so. False otherwise.
*/
bool ManuvrLink::linkIdle() {
  if (ManuvrLinkState::IDLE == _fsm_pos) {
    if (0 == _outbound_messages.size()) {
      if (0 == _inbound_messages.size()) {
        if (nullptr == _working) {
          return (_inbound_buf.isEmpty());
        }
      }
    }
  }
  return false;
}



/*******************************************************************************
* Debugging                                                                    *
*******************************************************************************/

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
  output->concatf("\tConnected:     %c\n", _flags.value(MANUVRLINK_FLAG_ESTABLISHED) ? 'y':'n');
  output->concatf("\tSync incoming: %c\n", _flags.value(MANUVRLINK_FLAG_SYNC_INCOMING) ? 'y':'n');
  output->concatf("\tSync casting:  %c\n", _flags.value(MANUVRLINK_FLAG_SYNC_CASTING) ? 'y':'n');
  output->concatf("\tSync replies:  %c\n", _flags.value(MANUVRLINK_FLAG_SYNC_REPLY_RXD) ? 'y':'n');

  if (_flags.value(MANUVRLINK_FLAG_AUTH_REQUIRED)) {
    output->concatf("\tAuth'd:        %c\n", _flags.value(MANUVRLINK_FLAG_AUTHD) ? 'y':'n');
  }
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
    output->concat("\n-- ManuvrMsg in process  ----------------------------\n");
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
* Application-facing interface for sending messages.
*
* @return  0 on success with no ID
*         >0 on success with a message ID for application tracking.
*         -1 if we fail to allocate a ManuvrMsg.
*         -2 if payload serialization failed.
*         -3 on queue insertion failure
*/
int ManuvrLink::send(KeyValuePair* kvp, bool need_reply) {
  int ret = -1;
  ManuvrMsgHdr hdr(ManuvrMsgCode::APPLICATION, 0, need_reply);
  ManuvrMsg* msg = new ManuvrMsg(&hdr, BusOpcode::TX);
  if (nullptr != msg) {
    ret = 0;
    if (nullptr != kvp) {
      if (0 != msg->setPayload(kvp)) {
        ret = -2;
      }
    }

    if (0 == ret) {
      ret = -3;
      if (0 == _send_msg(msg)) {
        ret = msg->uniqueId();
      }
    }

    // Clean up after ourselves if we fail.
    if (0 > ret) {
      delete msg;
    }
  }
  return ret;
}


/**
* Internal choke-point for outbound message logic.
*
* @return  0 on success.
*         -1 on flooded output queue (retry after messages clear out)
*         -2 on invalid message
*         -3 on queue insertion failure
*/
int8_t ManuvrLink::_send_msg(ManuvrMsg* msg) {
  int8_t ret = -1;
  if (_outbound_messages.size() < _opts.max_outbound) {
    ret--;
    if ((nullptr != msg) && (msg->isValidMsg())) {
      ret--;
      // Our use of the priority queue is to demote them in the queue based on if
      //   they are waiting for replies or not. Messages wit ha priority of 0 are
      //   only being held in the queue to verify that a response arrives.
      // Being as this is, a new message, it gets the default priority.
      int priority = (msg->msgCode() == ManuvrMsgCode::APPLICATION) ? MANUVRLINK_PRIORITY_APP : MANUVRLINK_PRIORITY_INTERNAL;
      if (0 <= _outbound_messages.insert(msg, priority)) {
        ret = 0;
      }
    }
  }
  if ((0 > ret) & (3 < _verbosity)) {
    _local_log.concatf("Link 0x%x failed in _send_msg(): %d\n", _session_tag, ret);
    msg->printDebug(&_local_log);
  }
  return ret;
}


/**
* Empties the inbound message queue (those bytes from the transport that we need to proc).
*
* @return  int The number of inbound messages that were purged.
*/
int ManuvrLink::_purge_inbound() {
  int return_value = _inbound_messages.size();
  while (_inbound_messages.hasNext()) {
    ManuvrMsg* temp = _inbound_messages.dequeue();
    delete temp;
  }
  return return_value;
}


/**
* Empties the outbound message queue (those bytes designated for the transport).
*
* @return  int The number of outbound messages that were purged.
*/
int ManuvrLink::_purge_outbound() {
  int return_value = _outbound_messages.size();
  while (_outbound_messages.hasNext()) {
    ManuvrMsg* temp = _outbound_messages.dequeue();
    delete temp;
  }
  return return_value;
}


/**
* Cycle through the inbound message queue and handle anything internal.
* Callback on anything marked for the application.
*
* @return  int The number of inbound messages processed.
*/
int8_t ManuvrLink::_churn_inbound() {
  int8_t ret = 0;
  while (_inbound_messages.hasNext()) {
    bool gc_message = true;
    ManuvrMsg* temp = _inbound_messages.dequeue();
    //if (_verbosity > 5) {
    //  _local_log.concatf("ManuvrLink (tag: 0x%x) responding to...\n", _session_tag);
    //  temp->printDebug(&_local_log);
    //}

    switch (temp->msgCode()) {
      case ManuvrMsgCode::SYNC_KEEPALIVE:
        // We got a sync message. Is it a reply?
        if (temp->isReply()) {   // If so, we can stop casting now.
          _flags.set(MANUVRLINK_FLAG_SYNC_REPLY_RXD);
          _flags.clear(MANUVRLINK_FLAG_SYNC_CASTING);
        }
        else {
          // If not, we need to reply, since the lower-tier logic has
          //   stopped doing so.
          _send_sync_packet(false);
        }
        break;

      case ManuvrMsgCode::CONNECT:
        if (temp->isReply()) {
          if (!_flags.value(MANUVRLINK_FLAG_ESTABLISHED)) {
            _flags.set(MANUVRLINK_FLAG_ESTABLISHED);
            if (_fsm_is_stable()) {
              if (_flags.value(MANUVRLINK_FLAG_AUTH_REQUIRED)) {
                _append_fsm_route(2, ManuvrLinkState::PENDING_AUTH, ManuvrLinkState::IDLE);
              }
              else {
                _append_fsm_route(1, ManuvrLinkState::IDLE);
              }
            }
          }
        }
        else if (temp->expectsReply()) {
          if (0 != temp->ack()) {
            if (_verbosity > 2) _local_log.concatf("ManuvrLink (tag: 0x%x) Failed to reply to CONNECT\n", _session_tag);
          }
          //if (_verbosity > 5) {
          //  _local_log.concatf("ManuvrLink (tag: 0x%x) responding with...\n", _session_tag);
          //  temp->printDebug(&_local_log);
          //}
          StringBuilder temp_out;
          temp->serialize(&temp_out);
          _relay_to_output_target(&temp_out);
        }
        break;

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
        switch (_invoke_msg_callback(temp)) {
          case 2:   // Requeue the message as a reply. Don't GC it.
            gc_message = (0 != _send_msg(temp));
            if (gc_message & (2 < _verbosity)) _local_log.concatf("Link 0x%x failed to insert a reply message into our queue.\n", _session_tag);
            break;
          case 1:   // Drop the message.
          case 0:   // No callback. TODO: Might choose to retain in the queue?
            break;
        }
        break;

      default:   // This should never happen.
        break;
    }

    if (gc_message) {
      if (temp->isReply()) {
        _clear_waiting_send_by_id(temp->uniqueId());
      }
      delete temp;
    }
  }
  return ret;
}


/**
* Go through the outbound queue, sending as necessary, and looking for
*   timeout violations.
*
* @return the number of messages sent.
*/
int8_t ManuvrLink::_churn_outbound() {
  int8_t ret = 0;
  if (_outbound_messages.hasNext()) {
    int current_priority = _outbound_messages.getPriority(0);
    ManuvrMsg* temp = _outbound_messages.dequeue();
    if (nullptr != temp) {
      int new_priority = (temp->msgCode() == ManuvrMsgCode::APPLICATION) ? MANUVRLINK_PRIORITY_APP : MANUVRLINK_PRIORITY_INTERNAL;
      bool gc_msg    = false;
      bool will_send = !temp->wasSent();
      switch (current_priority) {
        case MANUVRLINK_PRIORITY_WAITING_FOR_ACK:
          new_priority = MANUVRLINK_PRIORITY_WAITING_FOR_ACK;
        case MANUVRLINK_PRIORITY_APP:
        case MANUVRLINK_PRIORITY_INTERNAL:
        default:
          if (!will_send) {
            if (_opts.ms_timeout < temp->msSinceSend()) {
              // There is something in the outbound queue that has been waiting for
              //   a reply longer than the session timeout. Resend, or fail it.
              _seq_ack_fails++;
              will_send = temp->attemptRetry();
              gc_msg = !will_send;
            }
          }
          break;
      }

      if (will_send) {
        // Send it, and mark it as having been sent.
        StringBuilder temp_out;
        if (0 == temp->serialize(&temp_out)) {
          if (0 <= _relay_to_output_target(&temp_out)) {
            // If the buffer was moved to the transport driver...
            temp->markSent();
            new_priority = MANUVRLINK_PRIORITY_WAITING_FOR_ACK;
            gc_msg = !temp->expectsReply();
            ret++;
          }
        }
      }

      if (gc_msg) {
        _outbound_messages.remove(temp);
        delete temp;
      }
      else {
        _outbound_messages.insert(temp, new_priority);
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
    ManuvrMsg* temp = _outbound_messages.get(i);
    if (nullptr != temp) {
      if (id == temp->uniqueId()) {
        _outbound_messages.remove(temp);
        ret = 1;
      }
    }
  }
  return ret;
}


/*******************************************************************************
* Buffers, parsing, and scattered low-level functions                          *
*******************************************************************************/

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
  _flags.clear(~MANUVRLINK_FLAG_RESET_PRESERVE_MASK);
  _session_tag    = 0;
  _ms_last_send   = 0;
  _ms_last_rec    = 0;
  _seq_parse_errs = 0;
  _seq_ack_fails  = 0;
  _sync_losses    = 0;
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
  if ((0 > ret) & (1 < _verbosity)) _local_log.concatf("Link 0x%x failed in _relay_to_output_target(): %d\n", _session_tag, ret);
  return ret;
}


/**
* Internal function to invoke the application-provided callback for messages
*   received. During this stack frame, the application will be able to reply
*   to the message.
*
* @return 0 if no callback invoked.
*         1 if the message is to be dropped.
*         2 if the message was converted into a reply.
*/
int8_t ManuvrLink::_invoke_msg_callback(ManuvrMsg* msg) {
  int8_t ret = 0;
  if (nullptr != _msg_callback) {   // Call the callback, if it is set.
    ret++;
    _msg_callback(_session_tag, msg);
    if (BusOpcode::TX == msg->direction()) {
      // If the message is now marked as TX, it means the application wants to
      //   reply.
      ret++;
    }
  }
  return ret;
}


/**
* Consumes the class's input accumulation buffer, considering state, and driving
*   state reactions accordingly.
* For shorter stacks, and greater concurrency safety (including on the wire),
*   this function should only be called in the poll() function's stack frame.
*   This eliminates the risk of sending because we received because we
*   sent because....
*
* @return 0 on
*/
int8_t ManuvrLink::_process_input_buffer() {
  const int AVAILABLE_LENGTH = _inbound_buf.length();
  int8_t ret = 0;
  bool proc_fallthru = false;

  if (_verbosity > 6) _inbound_buf.printDebug(&_local_log);

  switch (_fsm_pos) {
    // If the link is actively trying to attain sync...
    case ManuvrLinkState::SYNC_RESYNC:
      switch (_process_for_sync(&_inbound_buf)) {
        case -1:   // insufficient length. No change to input data.
        case 0:    // no sync was found and so the input data was maximally culled.
        default:   // Should be impossible.
          break;
        case 1:    // sync found and search ended because we ran out of data to cull.
        case 2:    // sync found and search ended because sync ceased repeating.
          proc_fallthru = true;
          if (_flags.value(MANUVRLINK_FLAG_SYNC_CASTING)) {
            // Prevents us from having to wait on our own timeout to trigger
            //   our half of the sync exchange.
            _send_sync_packet(true);
          }
          break;
      }
      break;

    // The link believes that the input buffer is neatly-justified, but has yet
    //   to see something other than sync come across. We don't want to react
    //   to incoming sync. The general parse will catch it, if it exists.
    case ManuvrLinkState::SYNC_TENTATIVE:
    case ManuvrLinkState::PENDING_AUTH:
    case ManuvrLinkState::IDLE:
    case ManuvrLinkState::PENDING_HANGUP:
      proc_fallthru = true;
      break;

    // In any other state, do nothing, and leave the input buffer alone.
    default:
      break;
  }

  if (proc_fallthru) {
    if (_inbound_buf.length() >= 4) {
      if (nullptr == _working) {
        ManuvrMsgHdr _header;
        int8_t ret_header = ManuvrMsg::attempt_header_parse(&_header, &_inbound_buf);
        switch (ret_header) {
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
            if (_header.total_length() <= _opts.mtu) {
              _working = new ManuvrMsg(&_header);
            }
            break;
        }
        if ((_verbosity > 6) | ((ret_header < 0) & (_verbosity > 3))) {
          _local_log.concatf("ManuvrLink (tag: 0x%x) _attempt_header_parse returned %d.\n", _session_tag, ret_header);
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
            delete _working;
          }
          _working = nullptr;
        }
      }
    }
  }
  return ret;
}

/*******************************************************************************
* Functions for managing and reacting to sync states.                          *
*******************************************************************************/

/**
* Scan a StringBuilder for the protocol's sync pattern, and remove any data
*   fitting the pattern, for as long as the pattern holds.
* Only call this function if sync is required, since it will disregard any
*   non-sync message boundaries in the data.
* The only case where this function will NOT cull from the input data is if the
*   length of the input data was less than MANUVRMSGHDR_MINIMUM_HEADER_SIZE.
* Sets the SYNC_INCOMING flag if the received sync is a reply.
* Sends a reply sync if the received sync demands a reply.
*
* @param dat_in  The buffer to search through.
* @return -1 on insufficient length. No change to input data.
*          0 if no sync was found and so the input data was maximally culled.
*          1 if sync found and search ended because we ran out of data to cull.
*          2 if sync found and search ended because sync ceased repeating.
*/
int8_t ManuvrLink::_process_for_sync(StringBuilder* dat_in) {
  const int AVAILABLE_LEN = dat_in->length();
  int ret = -1;
  int i = _contains_sync_pattern(dat_in);
  if (0 <= i) {
    // Found sync data, and we are about to change the buffer. But before we
    //   cull the sync packet, note if it is a reply. We might-should
    //   take further action.
    ret = 1;
    uint8_t* buf          = dat_in->string();
    int      len          = dat_in->length();
    int      sync_0_idx   = i;  // Correct for cases where (0 != offset % 4).
    bool     keep_looping = ((AVAILABLE_LEN-i) >= MANUVRMSGHDR_MINIMUM_HEADER_SIZE);
    bool     set_sync     = false;
    bool     send_sync    = false;

    // This loop culls all of the sync data from the buffer, carefully noting
    //   the reply flags for each sync so discarded.
    while (keep_looping) {
      // Grab all the comparison bytes.
      const uint8_t b0 = *(buf + sync_0_idx + 0);  // msg_code
      const uint8_t b1 = *(buf + sync_0_idx + 1);  // flags
      const uint8_t b2 = *(buf + sync_0_idx + 2);  // length
      const uint8_t b3 = *(buf + sync_0_idx + 3);  // chksum
      const uint8_t MANUVRMSG_4TH_SYNC_BYTE = (b0 + b1 + b2 + MANUVRLINK_SERIALIZATION_VERSION);

      // In order to search for more data, we need at least the minimum header
      //   length, beyond what we just looked at.
      bool enough_4_nxt_loop = ((sync_0_idx + (MANUVRMSGHDR_MINIMUM_HEADER_SIZE << 1)) <= AVAILABLE_LEN);

      // Is the current index the start of a sync packet?
      bool bail = (b0 != (uint8_t) ManuvrMsgCode::SYNC_KEEPALIVE);
      bail |= ((b1 & MANUVRMSGHDR_FLAG_SYNC_MASK) != 0x10);
      bail |= (b2 != MANUVRMSGHDR_MINIMUM_HEADER_SIZE);
      bail |= (b3 != MANUVRMSG_4TH_SYNC_BYTE);

      if (!bail) {
        // If this packet was sync, accumulate flags.
        set_sync  |= (b1 & MANUVRMSGHDR_FLAG_IS_REPLY);
        send_sync |= (b1 & MANUVRMSGHDR_FLAG_EXPECTING_REPLY);
      }
      else {
        // Otherwise, the search may have ended because we found the last sync.
        if (enough_4_nxt_loop) {
          // But we can only know this if there is enough length remaining to
          //   prove us wrong.
          ret = 2;
        }
      }

      // Keep looping as long as we are still seeing sync, and there is enough
      //   buffer remaining to test for another one.
      keep_looping = (enough_4_nxt_loop & !bail);
      if (keep_looping) {
        sync_0_idx += MANUVRMSGHDR_MINIMUM_HEADER_SIZE;
      }
    }

    // Left-justify the buffer against the beginning of the packet that
    //   breaks the sequence of syncs, if such a pattern was found. If not,
    //   drop all the data up-to the terminal %4 bytes.
    // NOTE: It is safe to pass 0 as an argument to cull(). Nothing will happen.
    dat_in->cull(sync_0_idx);

    // Finally, consider the things we discovered about the syncs we just
    //   dropped, and act accordingly.
    if (set_sync) {
      // If we got a sync reply, mark the class as such and stop casting.
      _flags.set(MANUVRLINK_FLAG_SYNC_INCOMING);
      _flags.set(MANUVRLINK_FLAG_SYNC_REPLY_RXD);
    }
    if (send_sync) {
      // If a sync packet demanded a reply, issue it unconditionally.
      // Issues a single reply to possibly many syncs that demanded one. So
      //   class logic needs to respect the fact that sync packets will not
      //   necesarilly get sync replies in a 1:1 ratio.
      _send_sync_packet(false);
    }
  }
  else {
    // Without finding a sync packet, we drop the data.
    // Cull to a modulus of 4 bytes so as not to drop data we haven't tested.
    const uint32_t CULL_LEN = ((uint32_t) AVAILABLE_LEN) & 0xFFFFFFFC;
    if (0 < CULL_LEN) {   // clear() is cheaper than cull().
      ret = 0;
      if (AVAILABLE_LEN == CULL_LEN) {  dat_in->clear();         }
      else {                            dat_in->cull(CULL_LEN);  }
    }
  }
  if (_verbosity > 5) _local_log.concatf("Link 0x%x _process_for_sync() returned %d.\n", _session_tag, ret);
  return ret;
}


/**
* Is this object syncd with a remote version of itself?
*
* @return true if so. False otherwise.
*/
bool ManuvrLink::_link_syncd() {
  switch (_fsm_pos) {
    case ManuvrLinkState::PENDING_AUTH:
    case ManuvrLinkState::IDLE:
    case ManuvrLinkState::PENDING_HANGUP:
      return true;
  }
  return false;
}


int8_t ManuvrLink::_send_sync_packet(bool need_reply) {
  int8_t ret = -1;
  StringBuilder sync_packet;
  ManuvrMsgHdr sync_header(ManuvrMsgCode::SYNC_KEEPALIVE, 0, (need_reply ? MANUVRMSGHDR_FLAG_EXPECTING_REPLY : MANUVRMSGHDR_FLAG_IS_REPLY));
  if (sync_header.serialize(&sync_packet)) {
    ret = (0 < _relay_to_output_target(&sync_packet)) ? 0 : -2;
  }
  else if (2 < _verbosity) _local_log.concatf("Link 0x%x failed to serialize a sync header.\n", _session_tag);
  return ret;
}


int8_t ManuvrLink::_send_connect_message() {
  int8_t ret = -1;
  // TODO: For now, this will just send a connection packet.
  StringBuilder connect_packet;
  ManuvrMsgHdr connect_header(ManuvrMsgCode::CONNECT, 0, true);
  ManuvrMsg connect_msg(&connect_header, BusOpcode::TX);
  connect_msg.printDebug(&_local_log);

  if (connect_header.serialize(&connect_packet)) {
    ret = (0 < _relay_to_output_target(&connect_packet)) ? 0 : -2;
  }
  else if (2 < _verbosity) _local_log.concatf("Link 0x%x failed to serialize a connect header.\n", _session_tag);
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
      if (fsm_advance) {   // Make sure we have somewhere to advance INTO.
        _set_fsm_route(3, ManuvrLinkState::PENDING_SETUP, ManuvrLinkState::SYNC_RESYNC, ManuvrLinkState::SYNC_TENTATIVE);
      }
      break;

    // Exit conditions: The class has seen the first data for this session.
    case ManuvrLinkState::PENDING_SETUP:
      fsm_advance = true;
      //fsm_advance = (0 < _inbound_buf.length());
      break;

    // Exit conditions:
    case ManuvrLinkState::SYNC_RESYNC:
      if (_flags.value(MANUVRLINK_FLAG_SYNC_CASTING)) {
        fsm_advance = _flags.value(MANUVRLINK_FLAG_SYNC_CASTING);
      }
      break;

    // Exit conditions: Incoming data is no longer preceeded by sync packets.
    case ManuvrLinkState::SYNC_TENTATIVE:
      if (!_flags.value(MANUVRLINK_FLAG_SYNC_CASTING)) {
        fsm_advance = _flags.value(MANUVRLINK_FLAG_ESTABLISHED);
      }
      break;

    // Exit conditions: An acceptable authentication has happened.
    case ManuvrLinkState::PENDING_AUTH:
      fsm_advance = true;
      break;

    // Exit conditions: These states are canonically stable. So we advance when
    //   the state is not stable (the driver has somewhere else it wants to be).
    case ManuvrLinkState::IDLE:
      fsm_advance = !_fsm_is_stable();
      break;

    // Exit conditions:
    case ManuvrLinkState::PENDING_HANGUP:
      fsm_advance = true;
      break;

    // Exit conditions:
    case ManuvrLinkState::HUNGUP:
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

      // Entry into SYNC_CASTING means we trash any unprocessed inbound data, and
      //   begin emitting and expecting sync packets. Entry is contingent on a
      //   successful TX of a sync packet.
      case ManuvrLinkState::SYNC_RESYNC:
        _inbound_buf.clear();
        if (nullptr != _working) {
          delete _working;
          _working = nullptr;
        }
        _flags.clear(MANUVRLINK_FLAG_SYNC_INCOMING | MANUVRLINK_FLAG_SYNC_REPLY_RXD);
        state_entry_success = (0 == _send_sync_packet(true));
        _flags.set(MANUVRLINK_FLAG_SYNC_CASTING, state_entry_success);
        break;

      // Entry into SYNC_TENTATIVE requires that sync packets have been
      //   exchanged, and the start of non-sync data has yet to be located.
      //   Entry always succeeds.
      case ManuvrLinkState::SYNC_TENTATIVE:
        state_entry_success = (0 == _send_connect_message());
        if (!state_entry_success) {
          if (3 < _verbosity) _local_log.concatf("Link 0x%x failed to send initial connect.\n", _session_tag);
        }
        break;

      // Entry into PENDING_AUTH means we have successfully dispatched an
      //   authentication message.
      case ManuvrLinkState::PENDING_AUTH:
        state_entry_success = true;   // TODO: This entire feature.
        break;

      // Entry into IDLE means we reset any sync-related flags.
      //   Entry always succeeds.
      case ManuvrLinkState::IDLE:
        _flags.clear(MANUVRLINK_FLAG_SYNC_INCOMING | MANUVRLINK_FLAG_SYNC_REPLY_RXD);
        state_entry_success = true;
        break;

      // Entry into PENDING_HANGUP means we have successfully dispatched a
      //   message notifying our counterparty of our desire to hang-up, and are
      //   now waiting on the handshake to complete.
      case ManuvrLinkState::PENDING_HANGUP:
        state_entry_success = true;   // TODO: This
        break;

      // Entry into HUNGUP involves clearing/releasing any buffers and states
      //   from the prior session. Entry always succeeds.
      case ManuvrLinkState::HUNGUP:
        _reset_class();
        state_entry_success = true;
        break;

      // Entry into any other state is disallowed.
      default:
        break;
    }

    if (state_entry_success) {
      if (4 < _verbosity) _local_log.concatf("Link 0x%x moved %s ---> %s\n", _session_tag, sessionStateStr(_fsm_pos), sessionStateStr(new_state));
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


int8_t ManuvrLink::_fsm_insert_sync_states() {
  int8_t ret = -1;
  if (0 == _prepend_fsm_state(ManuvrLinkState::SYNC_TENTATIVE)) {
    if (0 == _prepend_fsm_state(ManuvrLinkState::SYNC_RESYNC)) {
      ret = 0;
    }
  }
  return ret;
}

#endif   // CONFIG_MANUVR_M2M_SUPPORT
