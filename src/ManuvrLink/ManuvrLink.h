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


This class descends from ManuvrOS's XenoSession and XenoMessage classes.

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
  implementation that faces an unspecified transport on one side, and the
  application on the other.

TODO: Since this class renders large chains of function calls opaque to the
  linker, it would be nice to put bounds on binary size with pre-processor
  case-offs.

NOTE: This has been a mad binge to port over all this work from ManuvrOS. Five
  days after having begun it, I think I'm going to leave it alone and try to
  build something with it. Testing coverage at this date:
    Filename          Line Coverage       Functions
    ------------------------------------------------------
    ManuvrLink.cpp     82.4%  570 / 692    95.0%  38 / 40
    ManuvrLink.h       98.3%   57 / 58    100.0%  34 / 34
    ManuvrMsg.cpp      90.4%  132 / 146    93.8%  15 / 16
    ManuvrMsgHdr.cpp  100.0%   92 / 92    100.0%   7 / 7
  Taking the mandatory build flag out ahead of release.
                                           ---J. Ian Lindsay 2021.10.16 20:27:54
*/


#include "../StringBuilder.h"
#include "../CppPotpourri.h"
#include "../KeyValuePair.h"
#include "../BusQueue.h"
#include "../FlagContainer.h"
#include "../PriorityQueue.h"
#include "../ElementPool.h"
#include "../Identity/Identity.h"
#include "../AbstractPlatform.h"

#ifndef __MANUVR_XENOSESSION_H
#define __MANUVR_XENOSESSION_H


/*******************************************************************************
* Parameters from the build system                                             *
*******************************************************************************/

//#define CONFIG_C3P_M2M_SUPPORT 1   // TODO: Until Rationalizer.h is done.

//#ifndef CONFIG_MANUVRMSG_PREALLOC_COUNT
//  #define CONFIG_MANUVRMSG_PREALLOC_COUNT   4
//#endif

/*
* We must bound the growth on memory usage, or have our stack be at the mercy
*   of our counterparty's good behavior.
*/
#ifndef CONFIG_C3PLINK_MAX_QUEUE_DEPTH
  #define CONFIG_C3PLINK_MAX_QUEUE_DEPTH   12
#endif


/*******************************************************************************
* Fixed definitions for the ManuvrLink subsystem                               *
*******************************************************************************/

// This value is our checksum preload. Calculation of new checksums should start
//   with this byte. It helps prevents us from acknowledging spurious data as a
//   connection attempt.
#define MANUVRLINK_SERIALIZATION_VERSION   1

#define MANUVRLINK_MAX_PARSE_FAILURES      3  // How many failures-to-parse should we tolerate before SYNCing?
#define MANUVRLINK_MAX_ACK_FAILURES        3  // How many failures-to-ACK should we tolerate before SYNCing?
#define MANUVRLINK_MAX_QUEUE_PRINT         3  //
#define MANUVRLINK_FSM_WAYPOINT_DEPTH      8  // How deep is our state planning?

/* Class flags for ManuvrLink. */
#define MANUVRLINK_FLAG_AUTH_REQUIRED   0x00000001  // Set if this session requires authentication.
#define MANUVRLINK_FLAG_AUTHD           0x00000002  // Set if this session has been authenticated.
#define MANUVRLINK_FLAG_SYNC_INCOMING   0x00000004  // We've seen a sync on this resync cycle.
#define MANUVRLINK_FLAG_SYNC_CASTING    0x00000008  // We're sending sync on this resync cycle.
#define MANUVRLINK_FLAG_SYNC_REPLY_RXD  0x00000010  // We've seen a reply to our syncs on this resync cycle.
#define MANUVRLINK_FLAG_ESTABLISHED     0x00000020  // We've exchanged CONNECT messages.
#define MANUVRLINK_FLAG_HANGUP_RXD      0x00000040  // We received a HANGUP message.
#define MANUVRLINK_FLAG_HANGUP_TXD      0x00000080  // Sent a HANGUP message on this session.
#define MANUVRLINK_FLAG_SEND_KA         0x00000100  // We will send a keep-alive on a defined interval.
#define MANUVRLINK_FLAG_ON_HOOK         0x00000200  // Following HANGUP, the app needs to reset this.
#define MANUVRLINK_FLAG_ALLOW_LOG_WRITE 0x00000400  // Do we allow a counterparty to write to our log?

// These ManuvrLink flags are allowed to be passed in as configuration.
#define MANUVRLINK_FLAG_ALLOWABLE_DEFAULT_MASK (MANUVRLINK_FLAG_AUTH_REQUIRED | \
                                                MANUVRLINK_FLAG_SEND_KA | \
                                                MANUVRLINK_FLAG_ALLOW_LOG_WRITE)
// These ManuvrLink flags survive class reset.
#define MANUVRLINK_FLAG_RESET_PRESERVE_MASK (MANUVRLINK_FLAG_ALLOWABLE_DEFAULT_MASK)


/* Class flags for ManuvrMsg. These are for state tracking, and will NOT be sent with each message. */
#define MANUVRMSG_FLAG_ACCUMULATOR_COMPLETE     0x01  // The accumulator contains the complete message.
#define MANUVRMSG_FLAG_TX_COMPLETE              0x02  // This outbound message went to the transport.
#define MANUVRMSG_FLAG_WAS_ACKD                 0x04  // This outbound message saw a reply come back.


/* Class flags for ManuvrMsgHdr. These will be sent with each message. */
#define MANUVRMSGHDR_FLAG_EXPECTING_REPLY       0x01  // This message needs to be ACKd.
#define MANUVRMSGHDR_FLAG_IS_REPLY              0x02  // This message IS a reply.
#define MANUVRMSGHDR_FLAG_RESERVED_0            0x04  // Must be 0.
#define MANUVRMSGHDR_FLAG_RESERVED_1            0x08  // Must be 0.
#define MANUVRMSGHDR_FLAG_ENCODES_LENGTH_BYTES  0x30  // Mask for 2-bit field. How many bytes of length are encoded?
#define MANUVRMSGHDR_FLAG_ENCODES_ID_BYTES      0xC0  // Mask for 2-bit field. How many bytes of ID are encoded?

#define MANUVRMSGHDR_FLAG_RESERVED_MASK (MANUVRMSGHDR_FLAG_RESERVED_0 | MANUVRMSGHDR_FLAG_RESERVED_1)

// This value is used to mask-off bytes that are not considered when
//   testing for a sync packet.
#define MANUVRMSGHDR_FLAG_SYNC_MASK  ~(MANUVRMSGHDR_FLAG_IS_REPLY | MANUVRMSGHDR_FLAG_EXPECTING_REPLY)

// The minimum header (thus, message) size.
#define MANUVRMSGHDR_MINIMUM_HEADER_SIZE  4

// Which bits are not automatic in the header?
#define MANUVRMSGHDR_SETTABLE_FLAG_BITS (MANUVRMSGHDR_FLAG_RESERVED_MASK | \
                                         MANUVRMSGHDR_FLAG_ENCODES_LENGTH_BYTES | \
                                         MANUVRMSGHDR_FLAG_ENCODES_ID_BYTES)


/*******************************************************************************
* Types                                                                         *
*******************************************************************************/
/*
* These are possible states of the link. They confine the space of our
*   possible dialog, and bias the conversation in a given direction.
*/
enum class ManuvrLinkState : uint8_t {
  UNINIT         = 0x00,  // Nothing has happened. Freshly-instantiated session.
  PENDING_SETUP  = 0x01,  // Class is clean and ready for a session.
  SYNC_RESYNC    = 0x02,  // Casting sync, and awaiting like replies.
  SYNC_TENTATIVE = 0x03,  // Stop casting sync. Churn until non-sync data arrives.
  PENDING_AUTH   = 0x04,  // Waiting on optional authentication.
  LIVE           = 0x05,  // Session is in the sync'd and connected state.
  PENDING_HANGUP = 0x06,  // Session hangup is imminent.
  HUNGUP         = 0x07   // Session is hungup and pending cleanup for re-use.
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
  LOG            = 0x08,   // Write a string to the counterparty's log.
  WHO            = 0x09,   // An announcement of Identity.
  DHT_FXN        = 0x0E,   // TODO: Future expansion for Link-mediated DHTs.
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

class ManuvrLink;
class ManuvrMsg;

/* Callback for notifications of link state change. */
typedef void (*ManuvrLinkCB)(ManuvrLink*);

/* Callback for application-directed messages from a link. */
typedef void (*ManuvrMsgCB)(uint32_t tag, ManuvrMsg*);


/*******************************************************************************
* Class definitions                                                            *
*******************************************************************************/

/*
* Conf representation for a ManuvrLink.
*
* TODO: Any incompatabilities with counterparties induced by these settings
*   should be reported in the Link's local log.
*/
class ManuvrLinkOpts {
  public:
    uint32_t ms_timeout;     // How many ms of dead air constitutes a timeout?
    uint32_t ms_keepalive;   // How often in a connected session ought we ping?
    uint32_t mtu;            // Largest message we will accept.
    uint32_t default_flags;  // Largest message we will accept.
    uint8_t  max_outbound;   // Maximum number of messages enqueue before rejection.
    uint8_t  max_inbound;    // Maximum number of messages enqueue before rejection.
    uint8_t  max_parse_errs; // How many sequential parse failures before resync?
    uint8_t  max_ack_fails;  // How many message retries before abort?
    uint8_t  prealloc_count; // How many messages should we preallocate?
    TCode    encoding;       // Start out as basic as possible.

    ManuvrLinkOpts(const ManuvrLinkOpts* obj) :
      ms_timeout(obj->ms_timeout),
      ms_keepalive(obj->ms_keepalive),
      mtu(obj->mtu),
      default_flags(MANUVRLINK_FLAG_ALLOWABLE_DEFAULT_MASK & obj->default_flags),
      max_outbound(obj->max_outbound),
      max_inbound(obj->max_inbound),
      max_parse_errs(obj->max_parse_errs),
      max_ack_fails(obj->max_ack_fails),
      prealloc_count(obj->prealloc_count),
      encoding(obj->encoding) {};

    ManuvrLinkOpts(uint32_t msto, uint32_t mska, uint32_t m, TCode enc = TCode::BINARY, uint32_t def_flgs = 0) :
      ms_timeout(msto),
      ms_keepalive(mska),
      mtu(m),
      default_flags(MANUVRLINK_FLAG_ALLOWABLE_DEFAULT_MASK & def_flgs),
      max_outbound(8),
      max_inbound(8),
      max_parse_errs(3),
      max_ack_fails(3),
      prealloc_count(4),
      encoding(enc) {};
};


/*
* Class representation for a message header.
* All messages have this data structure in common.
*/
class ManuvrMsgHdr {
  public:
    ManuvrMsgCode msg_code;  // The message code.
    uint8_t       flags;     // Flags to be encoded into the message.
    uint8_t       chk_byte;  // The expected checksum for the header.
    uint32_t      msg_len;   // Total length of the message (header + payload).
    uint32_t      msg_id;    // A unique ID for this message.

    /* Copy constructor */
    ManuvrMsgHdr(const ManuvrMsgHdr* obj) :
      msg_code(obj->msg_code),
      flags(obj->flags),
      chk_byte(obj->chk_byte),
      msg_len(obj->msg_len),
      msg_id(obj->msg_id) {};

    /* General constructor */
    ManuvrMsgHdr(ManuvrMsgCode, uint8_t pl_len, uint8_t flags, uint32_t i = 0);

    /* Convenience overloads */
    ManuvrMsgHdr(ManuvrMsgCode m) : ManuvrMsgHdr(m, 0, 0, 0) {};
    //ManuvrMsgHdr() : ManuvrMsgHdr(ManuvrMsgCode::UNDEFINED, 0, 0, 0) {};
    ManuvrMsgHdr() : msg_code(ManuvrMsgCode::UNDEFINED), flags(0), chk_byte(0), msg_len(0), msg_id(0) {};


    inline bool isReply() {        return (flags & MANUVRMSGHDR_FLAG_IS_REPLY);         };
    inline bool expectsReply() {   return (flags & MANUVRMSGHDR_FLAG_EXPECTING_REPLY);  };
    inline void isReply(bool x) {
      if (x) flags |= MANUVRMSGHDR_FLAG_IS_REPLY;
      else   flags &= ~MANUVRMSGHDR_FLAG_IS_REPLY;
    };
    inline void expectsReply(bool x) {
      if (x) flags |= MANUVRMSGHDR_FLAG_EXPECTING_REPLY;
      else   flags &= ~MANUVRMSGHDR_FLAG_EXPECTING_REPLY;
    };

    void wipe();
    bool isValid();
    bool isSync();
    bool serialize(StringBuilder*);
    bool set_payload_length(uint32_t);
    int  header_length();
    inline int payload_length() {  return (msg_len - header_length());    };
    inline int total_length() {    return msg_len;  };
    inline uint8_t len_length() {  return ((flags & MANUVRMSGHDR_FLAG_ENCODES_LENGTH_BYTES) >> 4);  };
    inline uint8_t id_length() {   return ((flags & MANUVRMSGHDR_FLAG_ENCODES_ID_BYTES) >> 6);      };
    inline uint8_t calc_hdr_chcksm() {
      return (uint8_t) (flags + msg_len + (uint8_t)msg_code + MANUVRLINK_SERIALIZATION_VERSION);
    };
    inline void rebuild_checksum() {   chk_byte = calc_hdr_chcksm();    };
};


/**
* This class represents an singular message between us and a counterparty.
*/
class ManuvrMsg {
  public:
    //ManuvrMsg(KeyValuePair*);  // Construct this way for outbound KVP.
    ManuvrMsg(ManuvrMsgHdr*, BusOpcode dir = BusOpcode::RX);
    ManuvrMsg() {};            // Featureless constructor for static allocation.
    ~ManuvrMsg();

    /* Accessors for the message header */
    inline void expectsReply(bool);
    inline bool expectsReply() {        return _header.expectsReply();   };
    inline bool isReply() {             return _header.isReply();        };
    inline ManuvrMsgCode msgCode() {    return _header.msg_code;         };
    inline uint32_t uniqueId() {        return _header.msg_id;           };

    /* Inlines for message options, flags, and status markers. */
    inline TCode     encoding() {     return _encoding;                                           };
    inline BusOpcode direction() {    return _op;                                                 };
    inline uint32_t  msSinceSend() {  return wrap_accounted_delta(_ms_io_mark, (uint32_t)millis()); };
    inline bool      rxComplete() {   return (_accumulator.length() == _header.payload_length()); };
    inline bool      wasSent() {      return _class_flag(MANUVRMSG_FLAG_TX_COMPLETE);             };
    inline bool      wasACKd() {      return _class_flag(MANUVRMSG_FLAG_WAS_ACKD);                };
    inline void      markACKd() {     _class_set_flag(MANUVRMSG_FLAG_WAS_ACKD);                   };

    void  markSent();
    void  wipe();                      // Put this object into a fresh state for re-use.
    bool  isValidMsg();
    inline int   ack() {  return reply(nullptr);  };
    int   reply(KeyValuePair*, bool reply_expected = false);
    int   getPayload(KeyValuePair**);  // Application calls this to gain access to the message payload.
    int   setPayload(KeyValuePair*);   // Application calls this to set the message payload.
    int   encoding(TCode);
    int   serialize(StringBuilder*);   // Link calls this to render this message as a buffer for the transport.
    int   accumulate(StringBuilder*);  // Link calls this to feed the message parser.
    void  printDebug(StringBuilder*);

    bool attemptRetry();

    static ManuvrMsg* unserialize(StringBuilder*);
    static int8_t attempt_header_parse(ManuvrMsgHdr*, StringBuilder*);


  private:
    ManuvrMsgHdr _header;
    BusOpcode _op         = BusOpcode::UNDEF;  // Differentiate between inbound and outbount messages.
    TCode     _encoding   = TCode::CBOR;     // Start out as basic as possible.
    uint8_t   _retries    = 3;    // No retries by default.
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
    virtual ~ManuvrLink();

    /* Implementation of BufferAccepter. Accepts data from a transport. */
    int8_t provideBuffer(StringBuilder*);

    /* Application glue */
    int8_t poll(StringBuilder* log = nullptr);
    int8_t hangup(bool graceful = true);
    int8_t reset();
    int8_t writeRemoteLog(StringBuilder*, bool need_reply = false);
    bool   linkIdle();
    int    send(KeyValuePair*, bool need_reply = false);
    inline bool     isConnected() {    return _flags.value(MANUVRLINK_FLAG_ESTABLISHED);   };

    inline bool     requireAuth() {        return _flags.value(MANUVRLINK_FLAG_AUTH_REQUIRED);   };
    inline void     requireAuth(bool x) {  _flags.set(MANUVRLINK_FLAG_AUTH_REQUIRED, x);         };
    inline bool     syncCast() {        return _flags.value(MANUVRLINK_FLAG_SYNC_CASTING);   };
    inline void     syncCast(bool x) {  _flags.set(MANUVRLINK_FLAG_SYNC_CASTING, x);         };

    //int8_t ping();

    /* Debugging */
    void printDebug(StringBuilder*);
    void printQueues(StringBuilder*);
    void printFSM(StringBuilder*);

    /* Inline accessors. */
    inline ManuvrLinkState getState() {               return _fsm_pos;      };
    inline void setCallback(ManuvrLinkCB cb) {        _lnk_callback = cb;   };
    inline void setCallback(ManuvrMsgCB cb) {         _msg_callback = cb;   };
    inline void setOutputTarget(BufferAccepter* o) {  _output_target = o;   };
    inline uint32_t  linkTag() {                      return _session_tag;  };
    inline uint16_t  replyTimeouts() {                return _unackd_sends;  };
    inline void      verbosity(uint8_t x) {           _verbosity = x;        };
    inline uint8_t   verbosity() {                    return _verbosity;     };
    inline void      localIdentity(Identity* id) {    _id_loc = id;          };
    inline Identity* localIdentity() {                return _id_loc;        };
    inline Identity* remoteIdentity() {               return _id_remote;     };

    /* Built-in per-instance console handlers. */
    int8_t console_handler(StringBuilder* text_return, StringBuilder* args);

    /* Static support fxns for enums */
    static const char* sessionStateStr(const ManuvrLinkState);
    static const char* manuvMsgCodeStr(const ManuvrMsgCode);
    static const bool  msgCodeValid(const ManuvrMsgCode);


  private:
    ManuvrLinkOpts    _opts;    // These are the application-provided options for the link.
    //ManuvrMsg                 _msg_pool[4];
    //ElementPool<ManuvrMsg*>   _preallocd(4, &_msg_pool);
    PriorityQueue<ManuvrMsg*> _outbound_messages;   // Messages that are bound for the counterparty.
    PriorityQueue<ManuvrMsg*> _inbound_messages;    // Messages that came from the counterparty.
    FlagContainer32   _flags;
    ManuvrLinkState   _fsm_waypoints[MANUVRLINK_FSM_WAYPOINT_DEPTH] = {ManuvrLinkState::UNINIT, };
    uint32_t          _fsm_lockout_ms = 0;        // Used to enforce a delay between state transitions.
    ManuvrLinkState   _fsm_pos        = ManuvrLinkState::UNINIT;  // TODO: Optimize this away.
    ManuvrLinkState   _fsm_pos_prior  = ManuvrLinkState::UNINIT;  // TODO: Remove? Never used in logic.
    uint8_t           _verbosity      = 0;        // By default, this class won't generate logs.
    uint8_t           _seq_parse_errs = 0;
    uint8_t           _seq_ack_fails  = 0;
    uint32_t          _session_tag    = 0;        // Allows the application to keep track of our callbacks.
    uint32_t          _ms_last_send   = 0;        // At what time did the last message go out?
    uint32_t          _ms_last_rec    = 0;        // At what time did the last message come in?
    uint16_t          _sync_losses    = 0;        // How many times this session have we lost sync?
    uint16_t          _unackd_sends   = 0;        // How many messages that needed an ACK failed to get one?
    ManuvrMsg*        _working        = nullptr;  // If we are in the middle of receiving a message,
    Identity*         _id_loc         = nullptr;
    Identity*         _id_remote      = nullptr;
    BufferAccepter*   _output_target  = nullptr;  // A pointer to the transport for outbound bytes.
    ManuvrLinkCB      _lnk_callback   = nullptr;  // The application-provided callback for state changes.
    ManuvrMsgCB       _msg_callback   = nullptr;  // The application-provided callback for incoming messages.
    StringBuilder     _inbound_buf;
    StringBuilder     _remote_log;

    /* Message queue management */
    int8_t _send_msg(ManuvrMsg*);
    int    _purge_inbound();
    int    _purge_outbound();
    int8_t _churn_inbound();
    int8_t _churn_outbound();
    int8_t _clear_waiting_send_by_id(uint32_t);

    /* Internal handlers for receiving messages confined to this class. */
    int8_t   _handle_msg_log(ManuvrMsg*);

    /* Buffers, parsing, and scattered low-level functions */
    void   _reset_class();
    int8_t _relay_to_output_target(StringBuilder*);
    int8_t _invoke_msg_callback(ManuvrMsg*);
    void   _invoke_state_callback();
    int8_t _process_input_buffer();
    int8_t _process_for_sync();

    /* Internal macros for sending messages confined to this class. */
    int8_t _send_sync_packet(bool need_reply);
    int8_t _send_connect_message();
    int8_t _send_hangup_message(bool graceful);
    int8_t _send_who_message();

    /* State machine functions */
    int8_t   _poll_fsm();
    int8_t   _set_fsm_position(ManuvrLinkState);
    int8_t   _set_fsm_route(int count, ...);
    int8_t   _append_fsm_route(int count, ...);
    int8_t   _prepend_fsm_state(ManuvrLinkState);
    int8_t   _advance_state_machine();
    bool     _fsm_is_waiting();
    int8_t   _fsm_insert_sync_states();
    inline ManuvrLinkState _fsm_pos_next() {   return _fsm_waypoints[0];   };
    inline bool _fsm_is_stable() {   return (ManuvrLinkState::UNINIT == _fsm_waypoints[0]);   };

    /* Message lifecycle */
    ManuvrMsg* _allocate_manuvrmsg(ManuvrMsgHdr*, BusOpcode);
    void       _reclaim_manuvrmsg(ManuvrMsg*);
};

#endif   // __MANUVR_XENOSESSION_H
