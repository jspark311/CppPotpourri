/*
File:   ManuvrLink.h
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


This class descends from ManuvrOS's Xenosession and XenoMessage classes.

Lessons learned from ManuvrOS:
--------------------------------------------------------------------------------
This class originally became a bad idea when it tried to manage the transport.
  It had too much of a baked-in bias toward being the initiating side, or the
  listening side. That situation became somewhat better once it began extending
  the BufferPipe class, since BufferPipe had an abstracted API for out-of-band
  signalling. Such a thing _might_ happen again. If it does, keep it confined.
This class originally leaned too heavilly on inheritance to achieve its
  abstractions. This worked, but added undue complexity when trying to do things
  like switch protocols mid-stream, or selecting a protocol upon connection.

This class originally did two things right, which I will try to preserve:
  1) It abstracted protocol fairly well (at higher-than-necessary cost).
  2) It maintained logical session states where such things were unsupported in
       the driver for the underlying transport.

The use of this class should be restricted to being a BufferAccepter
  implementation that faces an unspecified transport on one side,

TODO: Since this class renders large chains of function calls opaque to the
  linker, it would be nice to put bounds on binary size with pre-processor
  case-offs.
*/

#define CONFIG_MANUVR_M2M_SUPPORT 1   // TODO: Until Rationalizer.h is done.

#include <StringBuilder.h>
#include <CppPotpourri.h>
#include <KeyValuePair.h>
#include <BusQueue.h>
#include <FlagContainer.h>
#include <PriorityQueue.h>
#include <AbstractPlatform.h>

#ifndef __MANUVR_XENOSESSION_H
#define __MANUVR_XENOSESSION_H

// This value is our checksum preload. It helps prevents us from acknowledging
//   spurious data as a connection attempt.
#define MANUVRLINK_SERIALIZATION_VERSION   1  // Calculation of new checksums should start with this byte,
#define MANUVRLINK_INITIAL_SYNC_COUNT     24  // How many sync packets to send before giving up.
#define MANUVRLINK_MAX_PARSE_FAILURES      3  // How many failures-to-parse should we tolerate before SYNCing?
#define MANUVRLINK_MAX_ACK_FAILURES        3  // How many failures-to-ACK should we tolerate before SYNCing?

#define MANUVRLINK_MAX_QUEUE_PRINT         3  //
#define MANUVRLINK_FSM_WAYPOINT_DEPTH      8  // How deep is our state planning?


/* Class flags for ManuvrLink. */
#define MANUVRLINK_FLAG_AUTH_REQUIRED   0x00000001  // Set if this session requires authentication.
#define MANUVRLINK_FLAG_AUTHD           0x00000002  // Set if this session has been authenticated.
#define MANUVRLINK_FLAG_OVERFLOW_GUARD  0x00000004  // Set to protect the session buffer from overflow.
#define MANUVRLINK_FLAG_SYNCD           0x00000008  // The link is connected and sync'd.
#define MANUVRLINK_FLAG_SYNC_INCOMING   0x00000040  // We've seen a sync on this resync cycle.
#define MANUVRLINK_FLAG_SYNC_CASTING    0x00000080  // We're sending sync on this resync cycle.

/* These ManuvrLink flags are allowed to be passed in as configuration. */
#define MANUVRLINK_FLAG_ALLOWABLE_DEFAULT_MASK (MANUVRLINK_FLAG_AUTH_REQUIRED | \
                                                0)

/* Class flags for XenoMessage. These will be sent with each message. */
#define XENOMSG_FLAG_EXPECTING_REPLY       0x01  // This message needs to be ACKd.
#define XENOMSG_FLAG_IS_REPLY              0x02  // This message IS a reply.
#define XENOMSG_FLAG_RESERVED_0            0x04  // Must be 0.
#define XENOMSG_FLAG_RESERVED_1            0x08  // Must be 0.
#define XENOMSG_FLAG_ENCODES_LENGTH_BYTES  0x30  // Mask for 2-bit field. How many bytes of length are encoded?
#define XENOMSG_FLAG_ENCODES_ID_BYTES      0xC0  // Mask for 2-bit field. How many bytes of ID are encoded?

#define XENOMSG_FLAG_RESERVED_MASK (XENOMSG_FLAG_RESERVED_0 | XENOMSG_FLAG_RESERVED_1)

// This value is used to mask-off bytes that are not considered when
//   testing for a sync packet.
#define XENOMSG_FLAG_SYNC_MASK  ~(XENOMSG_FLAG_IS_REPLY | XENOMSG_FLAG_EXPECTING_REPLY)

// The minimum header (thus, message) size.
#define XENOMSG_MINIMUM_HEADER_SIZE  4

// Which bits are not automatic in the header?
#define XENOMSG_SETTABLE_FLAG_BITS (XENOMSG_FLAG_RESERVED_MASK | XENOMSG_FLAG_ENCODES_LENGTH_BYTES | XENOMSG_FLAG_ENCODES_ID_BYTES)


/*
* These are possible states of the link. They confine the space of our
*   possible dialog, and bias the conversation in a given direction.
*/
enum class ManuvrLinkState : uint8_t {
  UNINIT         = 0x00,  // Nothing has happened. Freshly-instantiated session.
  PENDING_SETUP  = 0x01,  // Class is clean and ready for a session.
  SYNC_BEGIN     = 0x02,  // Session has not demonstrated itself to be in sync.
  SYNC_CASTING   = 0x03,  // Casting sync, and awaiting like replies.
  SYNC_TENTATIVE = 0x04,  // Stop casting sync. Churn until non-sync data arrives.
  PENDING_AUTH   = 0x05,  // Waiting on optional authentication.
  ESTABLISHED    = 0x06,  // Session is in the nominal state.
  PENDING_HANGUP = 0x07,  // Session hangup is imminent.
  HUNGUP         = 0x08,  // Session is hungup and pending cleanup for re-use.
  DISCONNECTED   = 0x09   // Session is broken, but we have hope of recovery.
};

/*
* These are possible identifiers for the nature of messages we exchange
*   with a counterparty.
*/
enum class ManuvrMsgCode : uint8_t {
  UNDEFINED      = 0x00,   // This is the invalid-in-use default code.
  SYNC_KEEPALIVE = 0x01,   // Minimal message to keep a channel alive.
  CONNECT        = 0x02,   // Connection request.
  PROTOCOL       = 0x03,   // Protocol negotiation.
  AUTH_CHALLENGE = 0x04,   // Prove you are who you claim to be.
  HANGUP         = 0x05,   // Orderly termination of an active link.
  DESCRIBE       = 0x06,   // Exchange definitions of objects.
  MSG_FORWARD    = 0x07,   // A request for relay to a 3rd party.
  LOG            = 0x08,   // General information.
  APPLICATION    = 0x0F    // This message carries all interchange with the app.
};

/*
* These are the enumerations of the protocols we intend to support.
* TODO: This might be wrong-headed. Don't use it yet.
*/
enum class ManuvrLinkProto : uint8_t {
  MANUVR       = 0x00,   // CppPotpourri's native types.
  CONSOLE      = 0x01,   // Textual output for a user.
  MQTT         = 0x02,   //
  TCP          = 0x03,   //
  UDP          = 0x04,   //
  APRS         = 0x05,   //
  PSK          = 0x06,   //
  BLE_PROFILE  = 0x07,   //
  COAP         = 0x08,   //
  HTTP         = 0x09,   //
};


/*
* Conf representation for a ManuvrLink.
*
* TODO: Any incompatabilities with counterparties induced by these settings
*   should be reported in the Link's local log.
*/
class ManuvrLinkOpts {
  public:
    uint32_t ms_timeout;     // How many ms of dead air constitutes a timeout?
    uint32_t mtu;            // Largest message we will accept.
    uint32_t default_flags;  // Largest message we will accept.
    uint8_t  PADDING;
    uint8_t  max_parse_errs; // How many sequential parse failures before resync?
    uint8_t  max_ack_fails;  // How many message retries before abort?
    TCode    encoding;       // Start out as basic as possible.

    ManuvrLinkOpts(const ManuvrLinkOpts* obj) :
      ms_timeout(obj->ms_timeout),
      mtu(obj->mtu),
      default_flags(MANUVRLINK_FLAG_ALLOWABLE_DEFAULT_MASK & obj->default_flags),
      PADDING(0),
      max_parse_errs(obj->max_parse_errs),
      max_ack_fails(obj->max_ack_fails),
      encoding(obj->encoding) {};

    ManuvrLinkOpts(uint32_t msto, uint32_t m, uint32_t def_flgs = 0) :
      ms_timeout(msto),
      mtu(m),
      default_flags(MANUVRLINK_FLAG_ALLOWABLE_DEFAULT_MASK & def_flgs),
      PADDING(0),
      max_parse_errs(3),
      max_ack_fails(3),
      encoding(TCode::BINARY) {};
};


/*
* Class representation for a message header.
*/
class XenoMsgHeader {
  public:
    ManuvrMsgCode msg_code;  // The message code.
    uint8_t       flags;     // Flags to be encoded into the message.
    uint8_t       chk_byte;  // The expected checksum for the header.
    uint32_t      msg_len;   // Total length of the message (header + payload).
    uint32_t      msg_id;    // A unique ID for this message.

    XenoMsgHeader(const XenoMsgHeader* obj) :
      msg_code(obj->msg_code),
      flags(obj->flags),
      chk_byte(obj->chk_byte),
      msg_len(obj->msg_len),
      msg_id(obj->msg_id) {};

    XenoMsgHeader(ManuvrMsgCode, uint8_t pl_len, uint8_t flags, uint32_t i = 0);
    XenoMsgHeader(ManuvrMsgCode);
    XenoMsgHeader();


    inline bool isReply() {        return (flags & XENOMSG_FLAG_IS_REPLY);                     };
    inline bool expectsReply() {   return (flags & XENOMSG_FLAG_EXPECTING_REPLY);              };
    inline void expectsReply(bool x) {
      if (x) flags |= XENOMSG_FLAG_EXPECTING_REPLY;
      else   flags &= ~XENOMSG_FLAG_EXPECTING_REPLY;
    };

    void wipe();
    bool isValid();
    bool serialize(StringBuilder*);
    bool set_payload_length(uint32_t);
    int header_length();
    inline int payload_length() {  return (msg_len - header_length());    };
    inline int total_length() {    return msg_len;  };


  private:
    inline uint8_t len_length() {  return ((flags & XENOMSG_FLAG_ENCODES_LENGTH_BYTES) >> 4);  };
    inline uint8_t id_length() {   return ((flags & XENOMSG_FLAG_ENCODES_ID_BYTES) >> 6);      };
};



class XenoMessage;

/* Callback for application-directed messages from a link. */
typedef void (*ManuvrMsgCallback)(uint32_t tag, XenoMessage*);


/**
* This class represents an singular message between us and a counterparty.
* TODO: Extend KeyValuePair? Might make things way simpler...
*/
class XenoMessage {
  public:
    XenoMessage(KeyValuePair*);   // Construct this way for outbound KVP.
    XenoMessage(XenoMsgHeader*);  // Construct this way for inbound data.
    XenoMessage() {};   // Featureless constructor for static allocation.

    ~XenoMessage();

    inline bool isReply() {             return _header.isReply();          };
    inline bool expectsReply() {        return _header.expectsReply();     };
    inline void expectsReply(bool x) {  return _header.expectsReply(x);    };
    inline ManuvrMsgCode msgCode() {    return _header.msg_code;           };
    inline uint32_t uniqueId() {        return _header.msg_id;             };

    inline void     markSent() {     _ms_io_mark = millis();                                     };
    inline bool     wasSent() {      return (_ms_io_mark != 0);                                  };
    inline uint32_t msSinceSend() {  return wrap_accounted_delta(_ms_io_mark, millis());         };
    inline bool     rxComplete() {   return (_accumulator.length() == _header.payload_length()); };

    bool   isValidMsg();
    int    getPayload(KeyValuePair**);  // Application calls this to gain access to the message payload.
    int    setPayload(KeyValuePair*);   // Application calls this to set the message payload.
    int    reply(KeyValuePair*);

    int serialize(StringBuilder*);   // Link calls this to render this message as a buffer for the transport.
    int accumulate(StringBuilder*);  // Link calls this to feed the message parser.

    void wipe();         // Put this object into a fresh state for re-use.
    void printDebug(StringBuilder*);


  private:
    XenoMsgHeader _header;
    BusOpcode _op         = BusOpcode::UNDEF;  // TODO: This might be an unwise suggestion.
    TCode     _encoding   = TCode::BINARY;     // Start out as basic as possible.
    uint8_t   _retries    = 0;    // No retries by default.
    uint8_t   _flags      = 0;    // These are NOT sent with the message.
    uint32_t  _ms_io_mark = 0;    // This is the millis() reading when we sent or received.
    KeyValuePair* _kvp    = nullptr;
    StringBuilder _accumulator;

    /* Flag manipulation inlines */
    inline uint8_t _class_flags() {                return _flags;           };
    inline bool _class_flag(uint8_t _flag) {       return (_flags & _flag); };
    inline void _class_clear_flag(uint8_t _flag) { _flags &= ~_flag;        };
    inline void _class_set_flag(uint8_t _flag) {   _flags |= _flag;         };
    inline void _class_set_flag(uint8_t _flag, bool nu) {
      if (nu) _flags |= _flag;
      else    _flags &= ~_flag;
    };
};


/**
* This class represents an open comm session with a foreign device via an
*   unspecified transport. All we care about is the byte stream.
*/
class ManuvrLink : public BufferAccepter {
  public:
    ManuvrLink(const ManuvrLinkOpts* opts);
    ~ManuvrLink();

    /* Implementation of BufferAccepter. Accepts data from a transport. */
    int8_t provideBuffer(StringBuilder*);

    int8_t sendMessage(KeyValuePair*);

    void printDebug(StringBuilder*);
    void printFSM(StringBuilder*);

    inline uint32_t linkTag() {      return _session_tag;      };
    inline ManuvrLinkState getState() {      return _fsm_pos;    };
    inline void setCallback(ManuvrMsgCallback cb) {     _msg_callback = cb;    };
    inline void setOutputTarget(BufferAccepter* obj) {  _output_target = obj;  };

    int8_t hangup(bool graceful = true);
    int8_t poll(StringBuilder* log = nullptr);

    int8_t scan_buffer_for_sync();
    void   mark_session_desync(uint8_t desync_source);
    void   mark_session_sync(bool pending);

    bool isConnected();


    static const bool  msgCodeValid(const ManuvrMsgCode);
    static const char* manuvMsgCodeStr(const ManuvrMsgCode);
    static const char* sessionStateStr(const ManuvrLinkState);


  private:
    ManuvrLinkOpts    _opts;    // These are the application-provided options for the link.
    PriorityQueue<XenoMessage*> _outbound_messages;   // Messages that are bound for the counterparty.
    PriorityQueue<XenoMessage*> _inbound_messages;    // Messages that came from the counterparty.
    FlagContainer32   _flags;
    ManuvrLinkState   _fsm_waypoints[MANUVRLINK_FSM_WAYPOINT_DEPTH] = {ManuvrLinkState::UNINIT, };
    uint32_t          _fsm_lockout_ms = 0;        // Used to enforce a delay between state transitions.
    ManuvrLinkState   _fsm_pos        = ManuvrLinkState::UNINIT;
    ManuvrLinkState   _fsm_pos_prior  = ManuvrLinkState::UNINIT;
    uint8_t           _verbosity      = 7;
    uint32_t          _session_tag    = 0;        // Allows the application to keep track of our callbacks.
    uint32_t          _ms_last_send   = 0;        // At what time did the last message go out?
    uint32_t          _ms_last_rec    = 0;        // At what time did the last message come in?
    uint8_t           _seq_parse_errs = 0;
    uint8_t           _seq_ack_fails  = 0;
    uint16_t          _sync_losses    = 0;

    XenoMessage*      _working        = nullptr;  // If we are in the middle of receiving a message,
    BufferAccepter*   _output_target  = nullptr;  // A pointer to the transport for outbound bytes.
    ManuvrMsgCallback _msg_callback   = nullptr;  // The application-provided callback for incoming messages.
    //int8_t            _expected_chk   = 0;
    StringBuilder     _inbound_buf;
    StringBuilder     _local_log;

    /* State machine functions */
    int8_t   _poll_fsm();
    int8_t   _set_fsm_position(ManuvrLinkState);
    int8_t   _set_fsm_route(int count, ...);
    int8_t   _append_fsm_route(int count, ...);
    int8_t   _prepend_fsm_state(ManuvrLinkState);
    int8_t   _advance_state_machine();
    inline ManuvrLinkState _fsm_pos_next() {   return _fsm_waypoints[0];   };
    inline bool _fsm_is_stable() {   return (ManuvrLinkState::UNINIT == _fsm_waypoints[0]);   };
    bool     _fsm_is_waiting();

    /* High-level state accessors */
    bool _link_syncd();
    int8_t _fsm_insert_sync_states();

    /* Message queue management */
    int _purge_inbound();
    int _purge_outbound();
    int8_t _churn_inbound();
    int8_t _churn_outbound();
    int8_t _clear_waiting_reply_by_id(uint32_t);
    int8_t _clear_waiting_send_by_id(uint32_t);
    int8_t _send_message(XenoMessage*);


    /* Buffers, parsing, and scattered low-level functions */
    void   _reset_class();
    int8_t _relay_to_output_target(StringBuilder*);
    int8_t _invoke_msg_callback(XenoMessage*);
    int    _process_for_sync(StringBuilder*);
    int8_t _send_sync_packet(bool need_reply);
};

#endif   // __MANUVR_XENOSESSION_H
