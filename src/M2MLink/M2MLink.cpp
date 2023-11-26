/*
File:   M2MLink.cpp
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

#include "M2MLink.h"
#include "../BusQueue/BusQueue.h"

#if defined(CONFIG_C3P_M2M_SUPPORT)

/*******************************************************************************
* Definitions only needed inside this translation unit.
*******************************************************************************/
// Priority levels for various kinds of messages.
#define M2MLINK_PRIORITY_WAITING_FOR_ACK  5   // These will not resend until and unless they timeout.
#define M2MLINK_PRIORITY_APP              10  // Application messages.
#define M2MLINK_PRIORITY_INTERNAL         20  // Classes own messages have highest priority.


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

const char* M2MLink::sessionStateStr(const M2MLinkState CODE) {
  switch (CODE) {
    case M2MLinkState::UNINIT:           return "UNINIT";
    case M2MLinkState::PENDING_SETUP:    return "PENDING_SETUP";
    case M2MLinkState::SYNC_RESYNC:      return "SYNC_RESYNC";
    case M2MLinkState::SYNC_TENTATIVE:   return "SYNC_TENTATIVE";
    case M2MLinkState::PENDING_AUTH:     return "PENDING_AUTH";
    case M2MLinkState::LIVE:             return "LIVE";
    case M2MLinkState::PENDING_HANGUP:   return "PENDING_HANGUP";
    case M2MLinkState::HUNGUP:           return "HUNGUP";
    default:                                return "<UNKNOWN>";
  }
}

const char* M2MLink::manuvMsgCodeStr(const M2MMsgCode CODE) {
  switch (CODE) {
    case M2MMsgCode::UNDEFINED:          return "UNDEFINED";
    case M2MMsgCode::SYNC_KEEPALIVE:     return "SYNC_KEEPALIVE";
    case M2MMsgCode::CONNECT:            return "CONNECT";
    case M2MMsgCode::PROTOCOL:           return "PROTOCOL";
    case M2MMsgCode::AUTH_CHALLENGE:     return "AUTH_CHALLENGE";
    case M2MMsgCode::HANGUP:             return "HANGUP";
    case M2MMsgCode::DESCRIBE:           return "DESCRIBE";
    case M2MMsgCode::MSG_FORWARD:        return "MSG_FORWARD";
    case M2MMsgCode::LOG:                return "LOG";
    case M2MMsgCode::WHO:                return "WHO";
    case M2MMsgCode::DHT_FXN:            return "DHT_FXN";
    case M2MMsgCode::APPLICATION:        return "APPLICATION";
    default:                                return "<UNKNOWN>";
  }
}

/**
* Is the given message code valid? Used to do safe enum conversion.
*
* @param The M2MMsgCode code to test.
* @return true if so. False otherwise.
*/
const bool M2MLink::msgCodeValid(const M2MMsgCode CODE) {
  switch (CODE) {
    case M2MMsgCode::SYNC_KEEPALIVE:
    case M2MMsgCode::CONNECT:
    case M2MMsgCode::PROTOCOL:
    case M2MMsgCode::AUTH_CHALLENGE:
    case M2MMsgCode::HANGUP:
    case M2MMsgCode::DESCRIBE:
    case M2MMsgCode::MSG_FORWARD:
    case M2MMsgCode::LOG:
    case M2MMsgCode::WHO:
    case M2MMsgCode::DHT_FXN:
    case M2MMsgCode::APPLICATION:
      return true;
    default:
      return false;
  }
}

/**
* Is the given FSM code valid? Used to do safe enum conversion.
*
* @param The FSM code to test.
* @return true if so. False otherwise.
*/
static const bool _link_fsm_code_valid(const M2MLinkState CODE) {
  switch (CODE) {
    case M2MLinkState::UNINIT:
    case M2MLinkState::PENDING_SETUP:
    case M2MLinkState::SYNC_RESYNC:
    case M2MLinkState::SYNC_TENTATIVE:
    case M2MLinkState::PENDING_AUTH:
    case M2MLinkState::LIVE:
    case M2MLinkState::PENDING_HANGUP:
    case M2MLinkState::HUNGUP:
      return true;
    default:
      return false;
  }
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
    const uint8_t EXPECTED_4TH_SYNC_BYTE = (b0 + b1 + b2 + M2MLINK_SERIALIZATION_VERSION);
    if (b0 == (uint8_t) M2MMsgCode::SYNC_KEEPALIVE) {
      if ((b1 & M2MMSGHDR_FLAG_SYNC_MASK) == 0x10) {
        if (b2 == M2MMSGHDR_MINIMUM_HEADER_SIZE) {
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
M2MLink::M2MLink(const M2MLinkOpts* opts) : _opts(opts), _flags(opts->default_flags) {
}


/**
* Destructor
*
* Destroy any queued messages.
*/
M2MLink::~M2MLink() {
  _purge_inbound();
  _purge_outbound();
  if (nullptr != _working) {
    _reclaim_m2mmsg(_working);
    _working = nullptr;
  }
}



/*******************************************************************************
* Implementation of BufferAccepter                                             *
*******************************************************************************/

/**
* When we take bytes from the transport we store them in our local accumulator,
*   and process them on a polling cycle. This keeps stack usage lower, and saves
*   us concurrency concerns with regard to the transport driver's behavior.
* If the link isn't in such a state to accept the data, it claims it anyway, and
*   trashes it.
*
* @param buf Incoming data from the transport.
* @return -1 to reject buffer, 0 to accept without claiming, 1 to accept with claim.
*/
int8_t M2MLink::pushBuffer(StringBuilder* buf) {
  int8_t ret = 1;
  _ms_last_rec = millis();

  switch (_fsm_pos) {   // Consider the session state.
    case M2MLinkState::SYNC_RESYNC:
    case M2MLinkState::SYNC_TENTATIVE:  // We have exchanged sync packets with the counterparty.
    case M2MLinkState::PENDING_AUTH:
    case M2MLinkState::LIVE:            // The nominal case. Session is in-sync. Do nothing.
    case M2MLinkState::PENDING_HANGUP:
      if (_verbosity >= LOG_LEV_DEBUG) {
        StringBuilder tmp_log;
        tmp_log.concatf("\n\n__________Accepted (%u)\t", buf->length());
        buf->printDebug(&tmp_log);
        tmp_log.concatf("\n");
        c3p_log(LOG_LEV_INFO, __PRETTY_FUNCTION__, &tmp_log);
      }
      _inbound_buf.concatHandoff(buf);
      break;
    default:
      buf->clear();  // In any other case, drop the data.
      break;
  }

  return ret;
}


/**
* This function is called by a transport driver trying to feed incoming data to
*   the session. Thus we consider _inbound_buf.
*
* @return the number of bytes available in the TX ring.
*/
int32_t M2MLink::bufferAvailable() {
  return (_opts.mtu - _inbound_buf.length());
}


/*******************************************************************************
* Exposed member functions.                                                    *
*******************************************************************************/

/**
* This should be called periodically to service events in the link.
*
* @param log_ret is the optional buffer to receive class logs.
* @return 1 on link state shift
*         0 on no action
*        -1 on error
*/
int8_t M2MLink::poll(StringBuilder* log_ret) {
  switch (_fsm_pos) {
    case M2MLinkState::PENDING_SETUP:
    case M2MLinkState::HUNGUP:
      break;
    default:
      _process_input_buffer();
      _churn_inbound();
      _churn_outbound();
      // If we need to send an obligatory sync packet, do so.
      if (_flags.value(M2MLINK_FLAG_SYNC_CASTING | M2MLINK_FLAG_SEND_KA)) {
        if (millis_since(_ms_last_send) > _opts.ms_keepalive) {
          _send_sync_packet(true);
        }
      }
      break;
  }
  int8_t ret = _poll_fsm();                 // Poll the link's FSM.
  if (_remote_log.length() > 0) {           // If the link generated logs...
    if (nullptr != log_ret)                 // ...and the caller wants them...
      log_ret->concatHandoff(&_remote_log); // ...relay them to the caller.
    else _remote_log.clear();               // Otherwise, trash them.
  }
  return ret;
}


/**
* Public function to hang up on the counterparty.
*
* @param graceful should be true if the application wants to be polite.
* @return 0 on success. The state machine will carry things forward from here.
*        -1 if the link is not in such a state as to support a HANGUP.
*        -2 if a hangup is already in progress.
*/
int8_t M2MLink::hangup(bool graceful) {
  int8_t ret = -1;
  bool forced_hangup = false;
  switch (_fsm_pos) {
    case M2MLinkState::SYNC_RESYNC:
    case M2MLinkState::SYNC_TENTATIVE:
    case M2MLinkState::PENDING_AUTH:
    case M2MLinkState::LIVE:
      forced_hangup = !graceful;
      if (graceful) {
        ret = _append_fsm_route(2, M2MLinkState::PENDING_HANGUP, M2MLinkState::HUNGUP);
      }
      break;

    // We might be seeing a repeat call from the application.
    case M2MLinkState::PENDING_HANGUP:
    case M2MLinkState::HUNGUP:
      forced_hangup = !graceful;
      if (graceful) {
        ret = -2;
      }
      break;

    default:
      break;
  }

  if (forced_hangup) {
    // If we just want to kill the connection with no delay, we won't bother
    //   with the PENDING_HANGUP state. Obliterate the existing dialogs.
    _purge_inbound();
    _purge_outbound();
    ret = _set_fsm_route(1, M2MLinkState::HUNGUP);
  }
  return ret;
}



/**
* Reset the link object after a HANGUP.
* This is not to be used as a general "class re-init". The HANGUP state is
*   intended to give the application time to notice the Link's state, and do any
*   side-work that might be required prior to (optionally) re-using the Link for
*   another connection, or allowing GC. This function releases that hold.
*
* @return 0 on success, or -1 on improper state.
*/
int8_t M2MLink::reset() {
  int8_t ret = -1;
  if (M2MLinkState::HUNGUP == _fsm_pos) {
    // Clearing this flag will allow the polling loop to start re-using the
    //   class instance for another connection.
    _flags.clear(M2MLINK_FLAG_ON_HOOK);
    ret = 0;
  }
  return ret;
}


/**
* Write a message to the counterparty's system log.
* This is useful for investigating problems in firmware without
*   needing to have a console attached.
* This function always consumes the buffer it is fed as a parameter.
*
* @param outbound_log is the string we want to send.
* @param need_reply should be true if we want our log insertion ACK'd.
* @return 0 on successful send.
*        -1 if the log is empty.
*        -2 if the link is not established.
*        -3 on failure to allocate M2MMsg.
*        -4 on failure to append logs to message.
*        -5 on failure to send completed message.
*/
int8_t M2MLink::writeRemoteLog(StringBuilder* outbound_log, bool need_reply) {
  int8_t ret = -1;
  if (!outbound_log->isEmpty()) {
    if (_flags.value(M2MLINK_FLAG_ESTABLISHED)) {
      ret--;
      M2MMsgHdr hdr(M2MMsgCode::LOG, 0, (need_reply ? M2MMSGHDR_FLAG_EXPECTING_REPLY : 0));
      M2MMsg* msg = _allocate_m2mmsg(&hdr, BusOpcode::TX);
      ret--;
      if (nullptr != msg) {
        bool gc_message = true;  // Trash the message if sending doesn't work.
        ret--;
        KeyValuePair kvp("b", outbound_log);
        if (0 == msg->setPayload(&kvp)) {
          ret--;
          // At this point, we are discharged of the responsibility of keeping our
          //   original copy of kvp, since it has been already serialized into the
          //   message's accumulator. It will be fed to the transport later.
          if (0 == _send_msg(msg)) {
            gc_message = false;
            ret = 0;
          }
        }
        outbound_log->clear();  // Free the original buffer.
        if (gc_message) _reclaim_m2mmsg(msg);
      }
    }
  }
  return ret;
}


/**
* Is the link idle? Not connected implies not idle.
* Empty buffers. Empty message queues. In sync.
*
* @return true if so. False otherwise.
*/
bool M2MLink::linkIdle() {
  if (M2MLinkState::LIVE == _fsm_pos) {
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
void M2MLink::printDebug(StringBuilder* output) {
  uint32_t now = millis();
  StringBuilder temp("M2MLink ");
  temp.concatf("(tag: 0x%08x)", _session_tag);
  StringBuilder::styleHeader2(output, (const char*) temp.string());
  output->concatf("\tConnected:     %c\n", isConnected() ? 'y':'n');
  output->concatf("\tSync incoming: %c\n", _flags.value(M2MLINK_FLAG_SYNC_INCOMING) ? 'y':'n');
  output->concatf("\tSync casting:  %c\n", _flags.value(M2MLINK_FLAG_SYNC_CASTING) ? 'y':'n');
  output->concatf("\tSync replies:  %c\n", _flags.value(M2MLINK_FLAG_SYNC_REPLY_RXD) ? 'y':'n');
  output->concatf("\tAllow LOG:     %c\n", _flags.value(M2MLINK_FLAG_ALLOW_LOG_WRITE) ? 'y':'n');

  if (requireAuth()) {
    output->concatf("\tAuth'd:        %c\n", _flags.value(M2MLINK_FLAG_AUTHD) ? 'y':'n');
  }
  output->concatf("\tMTU:           %u\n", _opts.mtu);
  output->concatf("\tTimeout:       %ums\n", _opts.ms_timeout);
  output->concatf("\tEncoding:      %s\n", typecodeToStr(_opts.encoding));
  output->concatf("\tSync losses:   %u\n", _sync_losses);
  output->concatf("\tACK timeouts:  %u\n", _seq_ack_fails);
  output->concatf("\tBuffer size:   %u\n", _inbound_buf.length());
  output->concatf("\tLast outbound: %ums ago\n", (now - _ms_last_send));
  output->concatf("\tLast inbound:  %ums ago\n", (now - _ms_last_rec));

  if (isConnected()) {
    output->concat("\n-- Counterparty:\n");
    if (nullptr != remoteIdentity()) {

      output->concatf("\t[%s]:\t", remoteIdentity()->getHandle());
      remoteIdentity()->toString(output);
      output->concat('\n');
    }
    else {
      output->concat("\tUnidentified\n");
    }
  }
  output->concat('\n');
}


/**
* Debug support method.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void M2MLink::printQueues(StringBuilder* output) {
  int x = _outbound_messages.size();
  if (x > 0) {
    output->concatf("\n-- Outbound Queue %d total, showing top %d ------------\n", x, M2MLINK_MAX_QUEUE_PRINT);
    int max_print = (x < M2MLINK_MAX_QUEUE_PRINT) ? x : M2MLINK_MAX_QUEUE_PRINT;
    for (int i = 0; i < max_print; i++) {
      _outbound_messages.get(i)->printDebug(output);
    }
  }

  x = _inbound_messages.size();
  if (x > 0) {
    output->concatf("\n-- Inbound Queue %d total, showing top %d -------------\n", x, M2MLINK_MAX_QUEUE_PRINT);
    int max_print = (x < M2MLINK_MAX_QUEUE_PRINT) ? x : M2MLINK_MAX_QUEUE_PRINT;
    for (int i = 0; i < max_print; i++) {
      _inbound_messages.get(i)->printDebug(output);
    }
  }

  if (_working) {
    output->concat("\n-- M2MMsg in process  ----------------------------\n");
    _working->printDebug(output);
  }
  output->concat('\n');
}


/**
* Debug support method.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void M2MLink::printFSM(StringBuilder* output) {
  bool keep_looping = true;
  int i = 0;
  output->concatf("\tPrior state:   %s\n", sessionStateStr(_fsm_pos_prior));
  output->concatf("\tCurrent state: %s%s\n\tNext states:   ", sessionStateStr(_fsm_pos), _fsm_is_waiting() ? " (LOCKED)":" ");
  while (keep_looping & (i < M2MLINK_FSM_WAYPOINT_DEPTH)) {
    if (M2MLinkState::UNINIT == _fsm_waypoints[i]) {
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
* Application-facing interface for sending messages. This function should not be
*   called by the link class itself.
*
* @return  0 on success with no ID
*         >0 on success with a message ID for application tracking.
*         -1 if we fail to allocate a M2MMsg
*         -2 if payload serialization failed
*         -3 on queue insertion failure
*         -4 if the link is unwilling to take the message
*/
int M2MLink::send(KeyValuePair* kvp, bool need_reply) {
  int ret = -1;

  // Early abort tests.
  switch (_fsm_pos) {
    case M2MLinkState::PENDING_SETUP:
    case M2MLinkState::SYNC_RESYNC:
    case M2MLinkState::SYNC_TENTATIVE:
    case M2MLinkState::PENDING_AUTH:
    case M2MLinkState::LIVE:
      if (_outbound_messages.size() >= _opts.max_outbound) {
        return -3;
      }
      break;

    case M2MLinkState::UNINIT:
    case M2MLinkState::PENDING_HANGUP:  // If we are hanging/hung up, refuse.
    case M2MLinkState::HUNGUP:
    default:
      return -4;
  }

  M2MMsgHdr hdr(M2MMsgCode::APPLICATION, 0, need_reply);
  M2MMsg* msg = _allocate_m2mmsg(&hdr, BusOpcode::TX);
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
      _reclaim_m2mmsg(msg);
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
int8_t M2MLink::_send_msg(M2MMsg* msg) {
  int8_t ret = -1;
  if (_outbound_messages.size() < _opts.max_outbound) {
    ret--;
    if ((nullptr != msg) && (msg->isValidMsg())) {
      ret--;
      // Our use of the priority queue is to demote them in the queue based on if
      //   they are waiting for replies or not. Messages wit ha priority of 0 are
      //   only being held in the queue to verify that a response arrives.
      // Being as this is, a new message, it gets the default priority.
      int priority = M2MLINK_PRIORITY_INTERNAL;
      switch (msg->msgCode()) {
        case M2MMsgCode::APPLICATION:
          priority = M2MLINK_PRIORITY_APP;
          break;
        case M2MMsgCode::HANGUP:
          priority = 0;   // Hangup ought to be the last thing we deal with.
          // If we are processing a HANGUP message going outbound, we lock-out
          //   any additional sends that aren't already in the queue.
          break;
        default:
          break;
      }
      if (0 <= _outbound_messages.insert(msg, priority)) {
        ret = 0;
      }
    }
  }
  if ((0 > ret) & (LOG_LEV_ERROR >= _verbosity)) {
    StringBuilder tmp_log;
    tmp_log.concatf("Link 0x%08x failed in _send_msg(): %d\n", _session_tag, ret);
    msg->printDebug(&tmp_log);
    c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, &tmp_log);
  }
  return ret;
}


/**
* Empties the inbound message queue (those bytes from the transport that we need to proc).
*
* @return  int The number of inbound messages that were purged.
*/
int M2MLink::_purge_inbound() {
  int return_value = _inbound_messages.size();
  while (_inbound_messages.hasNext()) {
    M2MMsg* temp = _inbound_messages.dequeue();
    _reclaim_m2mmsg(temp);
  }
  return return_value;
}


/**
* Empties the outbound message queue (those bytes designated for the transport).
*
* @return  int The number of outbound messages that were purged.
*/
int M2MLink::_purge_outbound() {
  int return_value = _outbound_messages.size();
  while (_outbound_messages.hasNext()) {
    M2MMsg* temp = _outbound_messages.dequeue();
    _reclaim_m2mmsg(temp);
  }
  return return_value;
}


/**
* Cycle through the inbound message queue and handle anything internal.
* Callback on anything marked for the application.
*
* @return  int The number of inbound messages processed.
*/
int8_t M2MLink::_churn_inbound() {
  int8_t ret = 0;
  while (_inbound_messages.hasNext()) {
    bool gc_message = true;
    M2MMsg* temp = _inbound_messages.dequeue();
    KeyValuePair* kvps_rxd = nullptr;

    if (_verbosity >= LOG_LEV_INFO) {
      StringBuilder tmp_log;
      tmp_log.concatf("Link 0x%08x processing inbound...\n", _session_tag);
      temp->printDebug(&tmp_log);
      c3p_log(LOG_LEV_INFO, __PRETTY_FUNCTION__, &tmp_log);
    }

    switch (temp->msgCode()) {
      case M2MMsgCode::SYNC_KEEPALIVE:
        // We got a sync message. Is it a reply?
        if (temp->isReply()) {   // If so, we can stop casting now.
          _flags.set(M2MLINK_FLAG_SYNC_REPLY_RXD | M2MLINK_FLAG_SYNC_INCOMING);
          _flags.clear(M2MLINK_FLAG_SYNC_CASTING);
        }
        else {
          // If not, we need to reply, since the lower-tier logic has
          //   stopped doing so.
          _flags.set(M2MLINK_FLAG_SYNC_INCOMING);
          _send_sync_packet(false);
        }
        break;

      case M2MMsgCode::CONNECT:
        if (temp->isReply()) {
          if (!_flags.value(M2MLINK_FLAG_ESTABLISHED)) {
            if (_fsm_is_stable()) {   // Ensures we are in SYNC_TENTATIVE.
              if (_flags.value(M2MLINK_FLAG_AUTH_REQUIRED)) {
                _append_fsm_route(2, M2MLinkState::PENDING_AUTH, M2MLinkState::LIVE);
              }
              else {
                _append_fsm_route(1, M2MLinkState::LIVE);
              }
            }
          }
          else {
            _append_fsm_route(1, M2MLinkState::LIVE);
          }
          _flags.set(M2MLINK_FLAG_ESTABLISHED);
        }
        else if (temp->expectsReply()) {
          if (0 == temp->ack()) {
            gc_message = false;
          }
          else if (_verbosity >= LOG_LEV_ERROR) c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "Link 0x%08x Failed to reply to CONNECT\n", _session_tag);
          //if (_verbosity >= LOG_LEV_INFO) {
          //  StringBuilder tmp_log;
          //  tmp_log.concatf("Link 0x%08x responding with...\n", _session_tag);
          //  temp->printDebug(&tmp_log);
          //  c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, &tmp_log);
          //}
          StringBuilder temp_out;
          temp->serialize(&temp_out);
          _relay_to_output_target(&temp_out);
        }
        break;

      case M2MMsgCode::PROTOCOL:
      case M2MMsgCode::AUTH_CHALLENGE:
        break;
      case M2MMsgCode::HANGUP:
        // The other side wants to hang up. ACK if needed.
        _flags.set(M2MLINK_FLAG_HANGUP_RXD);
        if (temp->isReply()) {
          // We are seeing a reply to a HANGUP we previously sent. We take this
          //   to mean that our counterparty has finished sending, and has
          //   hung up. We should do the same.
        }
        else if (temp->expectsReply()) {
          if (0 == temp->ack()) {
            if (0 == _send_msg(temp)) {
              gc_message = false;
              _append_fsm_route(2, M2MLinkState::PENDING_HANGUP, M2MLinkState::HUNGUP);
            }
          }
          if (gc_message & (_verbosity >= LOG_LEV_ERROR)) {
            c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "Link 0x%08x Failed to reply to HANGUP\n", _session_tag);
          }
        }
        break;
      case M2MMsgCode::DESCRIBE:
      case M2MMsgCode::MSG_FORWARD:
        break;
      case M2MMsgCode::LOG:
        // Allow the counterparty to write to our session log?
        if (!temp->isReply()) {
          switch (_handle_msg_log(temp)) {
            case 2:   // Requeue the message as a reply. Don't GC it.
              gc_message = (0 != _send_msg(temp));
              if (gc_message & (_verbosity >= LOG_LEV_ERROR)) {
                c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "Link 0x%08x Failed to reply to LOG\n", _session_tag);
              }
              break;
            default:   // Drop the message.
              break;
          }
        }
        break;

      case M2MMsgCode::WHO:
        temp->getPayload(&kvps_rxd);
        if (kvps_rxd) {
          // Reply or not, this message might contain Identity information about
          //   our counterparty. If we don't already know who we're talking to,
          //   look for an Identity, and copy it.
          if (nullptr == _id_remote) {
            C3PValue* ident_container = kvps_rxd->valueWithKey("ident");
            if (ident_container) {
              if (_verbosity >= LOG_LEV_NOTICE) c3p_log(LOG_LEV_NOTICE, __PRETTY_FUNCTION__, "Link 0x%08x found a remote identity.\n", _session_tag);
              if (0 == ident_container->get_as(&_id_remote)) {
                ident_container->reapValue(false);   // We now own this object.
              }
            }
          }
        }

        if (temp->isReply()) {
          // TODO: Check if we require auth, and if so, do we have a remote identity?
          // We might choose to HANGUP if counterparty won't cooperate.
          if (temp->expectsReply()) {
            temp->ack();
            gc_message = (0 != _send_msg(temp));
          }
        }
        else if (temp->expectsReply()) {
          // We are being queried about our Identity.
          if (_id_loc) {
            // If we have one assigned, serialize and relay it as a reply.
            KeyValuePair* a = new KeyValuePair("ident", _id_loc);
            int8_t ret_local = temp->reply(a, true);
            const uint8_t VERB_LEV = (0 > ret_local) ? LOG_LEV_ERROR : LOG_LEV_INFO;
            if (_verbosity >= VERB_LEV) {
              c3p_log(VERB_LEV, __PRETTY_FUNCTION__, "Link 0x%08x reply to WHO returns %u.\n", _session_tag, ret_local);
            }
          }
          else {
            // If we don't have an identity to provide, try a simple ACK, and
            //   hope the other side doesn't HANGUP.
            temp->ack();
          }
          gc_message = (0 != _send_msg(temp));
          if (gc_message & (_verbosity >= LOG_LEV_ERROR)) {
            c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "Link 0x%08x Failed to reply to WHO\n", _session_tag);
          }
        }
        break;

      case M2MMsgCode::DHT_FXN:
        break;

      case M2MMsgCode::APPLICATION:
        switch (_invoke_msg_callback(temp)) {
          case 2:   // Requeue the message as a reply. Don't GC it.
            c3p_log(LOG_LEV_INFO, __PRETTY_FUNCTION__, "Requeue as a reply");
            gc_message = (0 != _send_msg(temp));
            if (gc_message & (_verbosity >= LOG_LEV_ERROR)) {
              c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "Link 0x%08x failed to insert an APPLICATION reply message into our queue.\n", _session_tag);
            }
            break;
          case 1:   // Drop the message.
            c3p_log(LOG_LEV_INFO, __PRETTY_FUNCTION__, "DROPPED");
            break;
          case 0:   // No callback. TODO: Might choose to retain in the queue?
            c3p_log(LOG_LEV_INFO, __PRETTY_FUNCTION__, "NO CALLBACK");
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
      _reclaim_m2mmsg(temp);
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
int8_t M2MLink::_churn_outbound() {
  int8_t ret = 0;
  if (_outbound_messages.hasNext()) {
    int current_priority = _outbound_messages.getPriority(0);
    M2MMsg* temp = _outbound_messages.dequeue();
    if (nullptr != temp) {
      int new_priority = (temp->msgCode() == M2MMsgCode::APPLICATION) ? M2MLINK_PRIORITY_APP : M2MLINK_PRIORITY_INTERNAL;
      bool gc_msg    = false;
      bool will_send = !temp->wasSent();
      switch (current_priority) {
        case M2MLINK_PRIORITY_WAITING_FOR_ACK:
          new_priority = M2MLINK_PRIORITY_WAITING_FOR_ACK;
        case M2MLINK_PRIORITY_APP:
        case M2MLINK_PRIORITY_INTERNAL:
        default:
          if (!will_send) {
            if (_opts.ms_timeout < temp->msSinceSend()) {
              // There is something in the outbound queue that has been waiting for
              //   a reply longer than the session timeout. Resend, or fail it.
              _seq_ack_fails++;
              will_send = temp->attemptRetry();
              gc_msg = !will_send;
              if (!will_send) _unackd_sends++;
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
            new_priority = M2MLINK_PRIORITY_WAITING_FOR_ACK;
            gc_msg = !temp->expectsReply();
            // Some internal message types have implications upon
            //   being successfully sent.
            if (M2MMsgCode::HANGUP == temp->msgCode()) {
              _flags.set(M2MLINK_FLAG_HANGUP_TXD);
            }
            ret++;
          }
        }
      }

      if (gc_msg) {
        _outbound_messages.remove(temp);
        _reclaim_m2mmsg(temp);
      }
      else {
        // NOTE: We skip the queue-size check, considering that we just pulled
        //   this object from this very queue.
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
*
* @param id is the message's uniqueID to search for.
* @return the number of messages cleared from the outbound queue.
*/
int8_t M2MLink::_clear_waiting_send_by_id(uint32_t id) {
  int8_t ret = 0;
  for (int i = 0; i < _outbound_messages.size(); i++) {
    M2MMsg* temp = _outbound_messages.get(i);
    if (nullptr != temp) {
      if (id == temp->uniqueId()) {
        temp->markACKd();
        _outbound_messages.remove(temp);
        ret = 1;
      }
    }
  }
  return ret;
}


/**
* This handler deals with the specifics of receiving a LOG message.
*
* @return 0 if no log was written.
*         1 if the message is to be dropped.
*         2 if the message was converted into a reply.
*/
int8_t M2MLink::_handle_msg_log(M2MMsg* msg) {
  int8_t ret = 0;
  if (_flags.value(M2MLINK_FLAG_ALLOW_LOG_WRITE)) {   // Will we allow a write to our log?
    ret++;
    KeyValuePair* inbound_kvp = nullptr;
    if (0 == msg->getPayload(&inbound_kvp)) {
      char* inbound_log = nullptr;
      if (0 == inbound_kvp->valueWithKey("b", &inbound_log)) {
        // TODO: Surround with randomly-generated tags to prevent confusion.
        _remote_log.concatf("Link 0x%08x counterparty says:\n%s\n", _session_tag, inbound_log);
      }
      else {
        if (LOG_LEV_NOTICE <= _verbosity) c3p_log(LOG_LEV_NOTICE, __PRETTY_FUNCTION__, "Link 0x%08x failed to decompose LOG message.\n", _session_tag);
      }
      //delete inbound_kvp;
    }
    else {
      if (LOG_LEV_NOTICE <= _verbosity) c3p_log(LOG_LEV_NOTICE, __PRETTY_FUNCTION__, "Link 0x%08x failed to find LOG payload.\n", _session_tag);
    }
  }
  if (msg->expectsReply()) {
    // Regardless of if we wrote log or not, ack the message so it won't be
    //   retransmitted.
    if (0 == msg->ack()) {
      ret = 2;  // The queue processor should re-insert this message.
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
void M2MLink::_reset_class() {
  _inbound_buf.clear();
  _purge_inbound();
  _purge_outbound();
  if (nullptr != _working) {
    _reclaim_m2mmsg(_working);
    _working = nullptr;
  }
  _flags.clear(~M2MLINK_FLAG_RESET_PRESERVE_MASK);
  _session_tag    = 0;
  _ms_last_send   = 0;
  _ms_last_rec    = 0;
  _seq_parse_errs = 0;
  _seq_ack_fails  = 0;
  _sync_losses    = 0;
  _sync_losses    = 0;
  _unackd_sends   = 0;
  if (nullptr != _id_remote) {
    // TODO: Follow logic tree for Identity storage. For now, just wipe it.
    Identity* tmp_ptr = _id_remote;
    _id_remote = nullptr;
    delete tmp_ptr;
  }
}


/**
* Calling this function with the ID of a message we previously sent will cause
*   that message to be released from the outbound queue, and is tantamount to
*   satisfying the reply.
*
* @param buf The StringBuilder containing the bytes to send to the transport.
* @return -2 on error (transport rejected data)
*         -1 on error (no transport set)
*          0 on success
*/
int8_t M2MLink::_relay_to_output_target(StringBuilder* buf) {
  int8_t ret = -1;
  if (nullptr != _output_target) {
    if (_verbosity >= LOG_LEV_DEBUG) {
      StringBuilder tmp_log;
      tmp_log.concatf("\n\n__________Emitting (%u)\t", buf->length());
      buf->printDebug(&tmp_log);
      tmp_log.concatf("\n");
      c3p_log(LOG_LEV_INFO, __PRETTY_FUNCTION__, &tmp_log);
    }

    switch (_output_target->pushBuffer(buf)) {
      case 0:
        buf->clear();  // If the BufferPipe didn't claim the buffer, clear it.
        // NOTE: No break;
      case 1:
        _ms_last_send = millis();
        ret = 0;
        break;
      default:
        ret = -2;
        break;
    }
  }

  if ((0 > ret) & (_verbosity >= LOG_LEV_ERROR)) {
    c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "Link 0x%08x failed in _relay_to_output_target(): %d\n", _session_tag, ret);
  }
  return ret;
}


/**
* Internal function to invoke the application-provided callback for noteworthy
*   state changes.
*/
void M2MLink::_invoke_state_callback() {
  if (nullptr != _lnk_callback) {   // Call the callback, if it is set.
    _lnk_callback(this);
  }
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
int8_t M2MLink::_invoke_msg_callback(M2MMsg* msg) {
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
int8_t M2MLink::_process_input_buffer() {
  int8_t ret = 0;
  bool proc_fallthru = false;

  switch (_fsm_pos) {
    // If the link is actively trying to attain sync...
    case M2MLinkState::SYNC_RESYNC:
      switch (_process_for_sync()) {
        case -1:   // insufficient length. No change to input data.
        case 0:    // no sync was found and so the input data was maximally culled.
        default:   // Should be impossible.
          break;
        case 1:    // sync found and search ended because we ran out of data to cull.
        case 2:    // sync found and search ended because sync ceased repeating.
          proc_fallthru = true;
          if (_flags.value(M2MLINK_FLAG_SYNC_CASTING)) {
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
    case M2MLinkState::SYNC_TENTATIVE:
    case M2MLinkState::PENDING_AUTH:
    case M2MLinkState::LIVE:
    case M2MLinkState::PENDING_HANGUP:
      proc_fallthru = true;
      break;

    // In any other state, do nothing, and leave the input buffer alone.
    default:
      break;
  }

  if (proc_fallthru) {
    if (_inbound_buf.length() >= 4) {
      if (nullptr == _working) {
        M2MMsgHdr _header;
        int8_t ret_header = M2MMsg::attempt_header_parse(&_header, &_inbound_buf);
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
            if (_header.total_length() <= (int) _opts.mtu) {
              _working = _allocate_m2mmsg(&_header, BusOpcode::RX);
            }
            break;
        }

        if ((_verbosity >= LOG_LEV_DEBUG) | ((ret_header < 0) & (_verbosity >= LOG_LEV_ERROR))) {
          c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "Link 0x%08x _attempt_header_parse returned %d.\n", _session_tag, ret_header);
        }
      }
      if (nullptr != _working) {
        _working->accumulate(&_inbound_buf);
        if (_working->rxComplete()) {
          if (_working->isValidMsg()) {
            _seq_parse_errs = 0;
            if (CONFIG_C3PLINK_MAX_QUEUE_DEPTH > _inbound_messages.size()) {
              _inbound_messages.insert(_working);
            }
            else {
              c3p_log(LOG_LEV_WARN, __PRETTY_FUNCTION__, "Link 0x%08x inbound message queue flood.\n", _session_tag);
            }
          }
          else {
            _seq_parse_errs++;
            if (_seq_parse_errs >= M2MLINK_MAX_PARSE_FAILURES) {
              // If we failed to parse too many times in-a-row, we assume the
              //   session is desyncd. Delete the bad message, and steer the
              //   session toward re-sync.
              if (_verbosity >= LOG_LEV_NOTICE) {
                StringBuilder tmp_log;
                tmp_log.concatf("Link 0x%08x experienced a parse failure:\n", _session_tag);
                _working->printDebug(&tmp_log);
                c3p_log(LOG_LEV_NOTICE, __PRETTY_FUNCTION__, &tmp_log);
              }
              _fsm_insert_sync_states();
              _sync_losses++;
            }
            _reclaim_m2mmsg(_working);
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
*   length of the input data was less than M2MMSGHDR_MINIMUM_HEADER_SIZE.
* Sets the SYNC_INCOMING flag and sends a reply sync if the received sync
*   demands a reply.
* Sets the SYNC_REPLY_RXD flag if the received sync is a reply.
*
* @param dat_in  The buffer to search through.
* @return -1 on insufficient length. No change to input data.
*          0 if no sync was found and so the input data was maximally culled.
*          1 if sync found and search ended because we ran out of data to cull.
*          2 if sync found and search ended because sync ceased repeating.
*/
int8_t M2MLink::_process_for_sync() {
  const int AVAILABLE_LEN = _inbound_buf.length();
  int ret = -1;
  int i = _contains_sync_pattern(&_inbound_buf);
  if (0 <= i) {
    // Found sync data, and we are about to change the buffer. But before we
    //   cull the sync packet, note if it is a reply. We might-should
    //   take further action.
    ret = 1;
    uint8_t* buf          = _inbound_buf.string();
    int      sync_0_idx   = i;  // Correct for cases where (0 != offset % 4).
    bool     keep_looping = ((AVAILABLE_LEN-i) >= M2MMSGHDR_MINIMUM_HEADER_SIZE);
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
      const uint8_t M2MMSG_4TH_SYNC_BYTE = (b0 + b1 + b2 + M2MLINK_SERIALIZATION_VERSION);

      // In order to search for more data, we need at least the minimum header
      //   length, beyond what we just looked at.
      bool enough_4_nxt_loop = ((sync_0_idx + (M2MMSGHDR_MINIMUM_HEADER_SIZE << 1)) <= AVAILABLE_LEN);

      // Is the current index the start of a sync packet?
      bool bail = (b0 != (uint8_t) M2MMsgCode::SYNC_KEEPALIVE);
      bail |= ((b1 & M2MMSGHDR_FLAG_SYNC_MASK) != 0x10);
      bail |= (b2 != M2MMSGHDR_MINIMUM_HEADER_SIZE);
      bail |= (b3 != M2MMSG_4TH_SYNC_BYTE);

      if (!bail) {
        // If this packet was sync, accumulate flags.
        set_sync  |= (b1 & M2MMSGHDR_FLAG_IS_REPLY);
        send_sync |= (b1 & M2MMSGHDR_FLAG_EXPECTING_REPLY);
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
        sync_0_idx += M2MMSGHDR_MINIMUM_HEADER_SIZE;
      }
    }

    // Left-justify the buffer against the beginning of the packet that
    //   breaks the sequence of syncs, if such a pattern was found. If not,
    //   drop all the data up-to the terminal %4 bytes.
    // NOTE: It is safe to pass 0 as an argument to cull(). Nothing will happen.
    _inbound_buf.cull(sync_0_idx);

    // Finally, consider the things we discovered about the syncs we just
    //   dropped, and act accordingly.
    if (set_sync) {
      // If we got a sync reply, mark the class as such. The state machine
      //   will handle any class transitions from here.
      _flags.set(M2MLINK_FLAG_SYNC_REPLY_RXD | M2MLINK_FLAG_SYNC_INCOMING);
    }
    if (send_sync) {
      // If a sync packet demanded a reply, issue it unconditionally.
      // Issues a single reply to possibly many syncs that demanded one. So
      //   class logic needs to respect the fact that sync packets will not
      //   necesarilly get sync replies in a 1:1 ratio.
      _flags.set(M2MLINK_FLAG_SYNC_INCOMING);
      _send_sync_packet(false);
    }
  }
  else {
    // Without finding a sync packet, we drop the data.
    // Cull to a modulus of 4 bytes so as not to drop data we haven't tested.
    const uint32_t CULL_LEN = ((uint32_t) AVAILABLE_LEN) & 0xFFFFFFFC;
    if (0 < CULL_LEN) {   // clear() is cheaper than cull().
      ret = 0;
      if ((uint32_t)AVAILABLE_LEN == CULL_LEN) {  _inbound_buf.clear();  }
      else {  _inbound_buf.cull(CULL_LEN);  }
    }
  }
  if (_verbosity >= LOG_LEV_DEBUG) {
    StringBuilder tmp_log;
    tmp_log.concatf("Link 0x%08x _process_for_sync() returned %d.\n", _session_tag, ret);
    //_working->printDebug(&tmp_log);
    c3p_log(LOG_LEV_DEBUG, __PRETTY_FUNCTION__, &tmp_log);
  }
  return ret;
}


/**
* SYNC packets are so important that they skip the normal flow of message
*   control, and are sent to the transport immediately when requested.
*
* @param need_reply should be set to true if we are still expecting SYNC reply.
* @return 0 on success. Nonzero on failure (typically rejection by the transport).
*/
int8_t M2MLink::_send_sync_packet(bool need_reply) {
  int8_t ret = -1;
  StringBuilder sync_packet;
  M2MMsgHdr sync_header(M2MMsgCode::SYNC_KEEPALIVE, 0, (need_reply ? M2MMSGHDR_FLAG_EXPECTING_REPLY : M2MMSGHDR_FLAG_IS_REPLY));
  if (sync_header.serialize(&sync_packet)) {
    ret = (0 <= _relay_to_output_target(&sync_packet)) ? 0 : -2;
  }
  //else if (2 < _verbosity) _local_log.concatf("Link 0x%08x failed to serialize a sync header.\n", _session_tag);
  return ret;
}


/**
* Sends a CONNECT message via the normal message pipeline.
*
* @return 0 on success. -1 on failure to allocate M2MMsg, -2 on send failure.
*/
int8_t M2MLink::_send_connect_message() {
  int8_t ret = -1;
  // TODO: For now, this will just send a connection message directly to the transport.
  StringBuilder connect_packet;
  M2MMsgHdr connect_header(M2MMsgCode::CONNECT, 0, true);
  M2MMsg connect_msg(&connect_header, BusOpcode::TX);
  if (connect_header.serialize(&connect_packet)) {
    ret = (0 <= _relay_to_output_target(&connect_packet)) ? 0 : -2;
  }
  else {
    if (_verbosity >= LOG_LEV_CRIT) {
      StringBuilder tmp_log;
      tmp_log.concatf("Link 0x%08x failed to serialize a connect header.\n", _session_tag);
      _working->printDebug(&tmp_log);
      c3p_log(LOG_LEV_CRIT, __PRETTY_FUNCTION__, &tmp_log);
    }
  }

  // TODO: Would prefer to use the code below. But SYNC is not well-enough
  //   under control.
  //M2MMsgHdr hdr(M2MMsgCode::CONNECT, 0, true);
  //M2MMsg* msg = _allocate_m2mmsg(&hdr, BusOpcode::TX);
  //if (nullptr != msg) {
  //  ret--;
  //  if (0 == _send_msg(msg)) {
  //    ret = 0;
  //  }
  //  else {
  //    _reclaim_m2mmsg(msg);
  //  }
  //}
  return ret;
}


/**
* Sends a HANGUP message via the normal message pipeline.
*
* @param graceful should be true if we want orderly termination on both sides.
* @return 0 on success. -1 on failure to allocate M2MMsg, -2 on send failure.
*/
int8_t M2MLink::_send_hangup_message(bool graceful) {
  int8_t ret = -1;
  M2MMsgHdr hdr(M2MMsgCode::HANGUP, 0, true);
  M2MMsg* msg = _allocate_m2mmsg(&hdr, BusOpcode::TX);
  if (nullptr != msg) {
    ret--;
    if (0 == _send_msg(msg)) {
      ret = 0;
    }
    else {
      _reclaim_m2mmsg(msg);
    }
  }
  return ret;
}


/**
* Sends a WHO message via the normal message pipeline.
*
* @return 0 on success. -1 on failure to allocate M2MMsg, -2 on send failure.
*/
int8_t M2MLink::_send_who_message() {
  int8_t ret = -1;
  M2MMsgHdr hdr(M2MMsgCode::WHO, 0, true);
  M2MMsg* msg = _allocate_m2mmsg(&hdr, BusOpcode::TX);
  if (nullptr != msg) {
    ret--;
    if (_id_loc) {
      KeyValuePair* a = new KeyValuePair("ident", _id_loc);
      msg->setPayload(a);
    }
    if (0 == _send_msg(msg)) {
      ret = 0;
    }
    else {
      _reclaim_m2mmsg(msg);
    }
  }
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
int8_t M2MLink::_poll_fsm() {
  int8_t ret = 0;
  bool fsm_advance = false;
  switch (_fsm_pos) {
    // Exit conditions: Class config is valid, and we have all the pointers we
    //   need.
    case M2MLinkState::UNINIT:
      fsm_advance = ((nullptr != _output_target) & (nullptr != _msg_callback));
      if (fsm_advance) {   // Make sure we have somewhere to advance INTO.
        _set_fsm_route(3, M2MLinkState::PENDING_SETUP, M2MLinkState::SYNC_RESYNC, M2MLinkState::SYNC_TENTATIVE);
      }
      break;

    // Exit conditions: The class has seen the first data for this session.
    case M2MLinkState::PENDING_SETUP:
      fsm_advance = true;
      //fsm_advance = (0 < _inbound_buf.length());
      break;

    // Exit conditions: We have begun sending sync packets into the transport.
    //   We have seen replies to our syncs, and incoming data is no longer
    //   preceeded by sync packets.
    case M2MLinkState::SYNC_RESYNC:
      fsm_advance = (_flags.value(M2MLINK_FLAG_SYNC_INCOMING) & _flags.value(M2MLINK_FLAG_SYNC_REPLY_RXD));
      break;

    // Exit conditions: We've exchanged CONNECT messages.
    case M2MLinkState::SYNC_TENTATIVE:
      fsm_advance = _flags.value(M2MLINK_FLAG_ESTABLISHED);
      break;

    // Exit conditions: An acceptable authentication has happened.
    case M2MLinkState::PENDING_AUTH:
      fsm_advance = _flags.value(M2MLINK_FLAG_AUTHD);
      break;

    // Exit conditions: These states are canonically stable. So we advance when
    //   the state is not stable (the link has somewhere else it wants to be).
    case M2MLinkState::LIVE:
      fsm_advance = !_fsm_is_stable();
      break;

    // Exit conditions: The outbound queue is empty, and at least one HANGUP
    //   message has been sent and ACK'd.
    case M2MLinkState::PENDING_HANGUP:
      if (!_outbound_messages.hasNext()) {
        fsm_advance = _flags.value(M2MLINK_FLAG_HANGUP_RXD) & _flags.value(M2MLINK_FLAG_HANGUP_TXD);
      }
      break;

    // Exit conditions: The application has called reset(), to take this link
    //   back "off-hook".
    case M2MLinkState::HUNGUP:
      fsm_advance = !_flags.value(M2MLINK_FLAG_ON_HOOK);
      if (fsm_advance) {   // Make sure we have somewhere to advance INTO.
        _set_fsm_route(3, M2MLinkState::PENDING_SETUP, M2MLinkState::SYNC_RESYNC, M2MLinkState::SYNC_TENTATIVE);
      }
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
int8_t M2MLink::_set_fsm_position(M2MLinkState new_state) {
  int8_t fxn_ret = -1;
  bool state_entry_success = false;   // Fail by default.
  if (!_fsm_is_waiting()) {
    switch (new_state) {
      // Entry into PENDING_SETUP means that the class has been wiped, and the
      //   values we depend upon later have been validated.
      case M2MLinkState::PENDING_SETUP:
        _reset_class();
        _session_tag = randomUInt32();
        state_entry_success = (_session_tag != 0);
        break;

      // Entry into SYNC_CASTING means we trash any unprocessed inbound data, and
      //   begin emitting and expecting sync packets. Entry is contingent on a
      //   successful TX of a sync packet.
      case M2MLinkState::SYNC_RESYNC:
        _inbound_buf.clear();
        if (nullptr != _working) {
          _reclaim_m2mmsg(_working);
          _working = nullptr;
        }
        _flags.clear(M2MLINK_FLAG_SYNC_INCOMING | M2MLINK_FLAG_SYNC_REPLY_RXD);
        state_entry_success = (0 == _send_sync_packet(true));
        _flags.set(M2MLINK_FLAG_SYNC_CASTING, state_entry_success);
        break;

      // Entry into SYNC_TENTATIVE requires that sync packets have been
      //   exchanged, and the start of non-sync data has yet to be located.
      //   Entry always succeeds.
      case M2MLinkState::SYNC_TENTATIVE:
        _flags.clear(M2MLINK_FLAG_SYNC_CASTING);
        state_entry_success = (0 == _send_connect_message());
        if (!state_entry_success) {
          if (_verbosity >= LOG_LEV_ERROR) c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "Link 0x%08x failed to send initial connect.\n", _session_tag);
        }
        break;

      // Entry into PENDING_AUTH means we have successfully dispatched an
      //   authentication message.
      case M2MLinkState::PENDING_AUTH:
        state_entry_success = true;   // TODO: This entire feature.
        break;

      // Entry into LIVE means we reset any sync-related flags.
      //   Entry always succeeds.
      case M2MLinkState::LIVE:
        _flags.clear(M2MLINK_FLAG_SYNC_INCOMING | M2MLINK_FLAG_SYNC_REPLY_RXD);
        _send_who_message();
        state_entry_success = true;
        break;

      // Entry into PENDING_HANGUP means we have successfully dispatched a
      //   message notifying our counterparty of our desire to hang-up, and are
      //   now waiting on the handshake to complete.
      case M2MLinkState::PENDING_HANGUP:
        state_entry_success = (0 == _send_hangup_message(true));
        if (!state_entry_success) {
          if (_verbosity >= LOG_LEV_ERROR) c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "Link 0x%08x failed to send initial HANGUP.\n", _session_tag);
        }
        break;

      // Entry into HUNGUP involves clearing/releasing any buffers and states
      //   from the prior session. Entry always succeeds.
      case M2MLinkState::HUNGUP:
        //_reset_class();
        _flags.set(M2MLINK_FLAG_ON_HOOK);
        _flags.clear(M2MLINK_FLAG_ESTABLISHED);
        state_entry_success = true;
        break;

      // Entry into any other state is disallowed.
      default:
        break;
    }

    if (state_entry_success) {
      if (_verbosity >= LOG_LEV_INFO) c3p_log(LOG_LEV_INFO, __PRETTY_FUNCTION__, "Link 0x%08x moved %s ---> %s\n", _session_tag, sessionStateStr(_fsm_pos), sessionStateStr(new_state));

      _fsm_pos_prior = _fsm_pos;
      _fsm_pos       = new_state;
      switch (new_state) {
        case M2MLinkState::HUNGUP:        // Entry into these states might
        case M2MLinkState::PENDING_AUTH:  // be an event worth passing to
        case M2MLinkState::LIVE:          // the application.
          _invoke_state_callback();
        default:
          break;
      }
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
int8_t M2MLink::_advance_state_machine() {
  int8_t ret = -1;
  if (M2MLinkState::UNINIT != _fsm_waypoints[0]) {
    if (0 == _set_fsm_position(_fsm_waypoints[0])) {
      ret = 0;
      for (int i = 0; i < (M2MLINK_FSM_WAYPOINT_DEPTH-1); i++) {
        _fsm_waypoints[i] = _fsm_waypoints[i+1];
      }
      _fsm_waypoints[M2MLINK_FSM_WAYPOINT_DEPTH-1] = M2MLinkState::UNINIT;
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
int8_t M2MLink::_set_fsm_route(int arg_count, ...) {
  int8_t ret = -1;
  const int PARAM_COUNT = strict_min((int8_t) arg_count, (int8_t) M2MLINK_FSM_WAYPOINT_DEPTH);
  if (PARAM_COUNT > 0) {
    va_list args;
    va_start(args, arg_count);
    M2MLinkState test_values[PARAM_COUNT] = {M2MLinkState::UNINIT, };
    ret = 0;
    for (int i = 0; i < PARAM_COUNT; i++) {
      test_values[i] = (M2MLinkState) va_arg(args, int);
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
      for (int i = 0; i < M2MLINK_FSM_WAYPOINT_DEPTH; i++) {
        _fsm_waypoints[i] = (i < PARAM_COUNT) ? test_values[i] : M2MLinkState::UNINIT;
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
int8_t M2MLink::_append_fsm_route(int arg_count, ...) {
  int8_t ret = -1;
  const int PARAM_COUNT = strict_min((int8_t) arg_count, (int8_t) M2MLINK_FSM_WAYPOINT_DEPTH);
  if (PARAM_COUNT > 0) {
    va_list args;
    va_start(args, arg_count);
    M2MLinkState test_values[PARAM_COUNT] = {M2MLinkState::UNINIT, };
    ret = 0;
    for (int i = 0; i < PARAM_COUNT; i++) {
      test_values[i] = (M2MLinkState) va_arg(args, int);
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
      while ((fidx < M2MLINK_FSM_WAYPOINT_DEPTH) && (M2MLinkState::UNINIT != _fsm_waypoints[fidx])) {
        fidx++;
      }
      const uint8_t PARAMS_TO_COPY = strict_min((uint8_t)(M2MLINK_FSM_WAYPOINT_DEPTH - fidx), (uint8_t) PARAM_COUNT);
      for (int i = 0; i < PARAMS_TO_COPY; i++) {
        _fsm_waypoints[i + fidx] = test_values[i];
      }
    }
  }
  return ret;
}


int8_t M2MLink::_prepend_fsm_state(M2MLinkState nxt) {
  int8_t ret = -1;
  if (_link_fsm_code_valid(nxt)) {
    ret--;
    // If everything looks good, seek to the end of the state traversal list,
    //   and append.
    uint8_t fidx = 0;
    M2MLinkState last = _fsm_waypoints[0];
    while ((fidx < M2MLINK_FSM_WAYPOINT_DEPTH) && (M2MLinkState::UNINIT != _fsm_waypoints[fidx])) {
      last = _fsm_waypoints[fidx];
      _fsm_waypoints[fidx] = nxt;
      nxt = last;
      fidx++;
    }
    if (fidx < M2MLINK_FSM_WAYPOINT_DEPTH) {
      _fsm_waypoints[fidx] = nxt;
      ret = 0;
    }
  }
  return ret;
}


bool M2MLink::_fsm_is_waiting() {
  bool ret = false;
  if (0 != _fsm_lockout_ms) {
    ret = !(millis() >= _fsm_lockout_ms);
    if (!ret) {
      _fsm_lockout_ms = 0;
    }
  }
  return ret;
}


int8_t M2MLink::_fsm_insert_sync_states() {
  int8_t ret = -1;
  if (0 == _prepend_fsm_state(M2MLinkState::SYNC_TENTATIVE)) {
    if (0 == _prepend_fsm_state(M2MLinkState::SYNC_RESYNC)) {
      ret = 0;
    }
  }
  return ret;
}


/*******************************************************************************
* M2MMsg memory lifecycle functions                                         *
*******************************************************************************/

/**
* Allocate a M2MMsg for this class, and imbue it with the given properties.
*
* @param hdr is the M2MMsg to clean up. Possibly for re-use.
* @return A M2MMsg* ready for use, or nullptr on failure.
*/
M2MMsg* M2MLink::_allocate_m2mmsg(M2MMsgHdr* hdr, BusOpcode op) {
  M2MMsg* ret = new M2MMsg(hdr, op);
  if (nullptr != ret) {
    ret->encoding(_opts.encoding);
  }
  return ret;
}


/**
* Dispose of M2MMsgs for this class.
*
* @param msg is the M2MMsg to clean up. Possibly for re-use.
*/
void M2MLink::_reclaim_m2mmsg(M2MMsg* msg) {
  if (nullptr != msg) {
    msg->wipe();
    delete msg;
  }
}


/*******************************************************************************
* Console callback
* These are built-in handlers for using this instance via a console.
*******************************************************************************/

int8_t M2MLink::console_handler(StringBuilder* text_return, StringBuilder* args) {
  int ret = 0;
  char* cmd = args->position_trimmed(0);
  if (0 == StringBuilder::strcasecmp(cmd, "info")) {
    printDebug(text_return);
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "queues")) {
    printQueues(text_return);
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "fsm")) {
    printFSM(text_return);
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "local")) {
    // Local identity and policy can be edited.
    char* subcmd = args->position_trimmed(1);
    if (0 == StringBuilder::strcasecmp(subcmd, "identity")) {
      if (_id_loc) {
        _id_loc->toString(text_return);
      }
      else {
        text_return->concat("No local Identity in use.\n");
      }
    }
    else if (0 == StringBuilder::strcasecmp(cmd, "policy")) {
    }
    else {
      text_return->concat("Usage: <identity | policy>\n");
    }
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "remote")) {
    // Remote identity and policy can only be examined.
    char* subcmd = args->position_trimmed(1);
    if (0 == StringBuilder::strcasecmp(subcmd, "identity")) {
      if (_id_remote) {
        _id_remote->toString(text_return);
      }
      else {
        text_return->concat("No remote Identity.\n");
      }
    }
    else if (0 == StringBuilder::strcasecmp(cmd, "policy")) {
    }
    else {
      text_return->concat("Usage: <identity | policy>\n");
    }
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "connect")) {
    text_return->concatf("send_connect_message() returns %d\n", _send_connect_message());
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "who")) {
    text_return->concatf("send_who_message() returns %d\n", _send_who_message());
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "reset")) {
    text_return->concatf("Link.reset() returns %d\n", reset());
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "hangup")) {
    text_return->concatf("Link.hangup() returns %d\n", hangup());
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "sync")) {
    if (1 < args->count()) {
      bool new_arg = (0 != args->position_as_int(1));
      syncCast(new_arg);
    }
    text_return->concatf("Link syncCast(%c)\n", syncCast() ? '1':'0');
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "poll")) {
    text_return->concatf("Link poll() returns %d\n", poll(text_return));
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "verbosity")) {
    switch (args->count()) {
      case 2:
        verbosity(0x07 & args->position_as_int(1));
      default:
        text_return->concatf("Link verbosity is %u\n", verbosity());
        break;
    }
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "log")) {
    //if (1 < args->count()) {
      StringBuilder tmp_log("This is a remote log test.\n");
      int8_t ret_local = writeRemoteLog(&tmp_log, false);
      text_return->concatf("Remote log write returns %d\n", ret_local);
    //}
    //else text_return->concat("Usage: link log <logText>\n");
  }
  else {
    text_return->concat("Usage: [info|local|remote|reset|hangup|sync|poll|log|verbosity]\n");
    ret = -1;
  }

  return ret;
}

#endif   // CONFIG_C3P_M2M_SUPPORT
