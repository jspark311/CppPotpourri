/*
File:   M2MLink.h
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
    M2MLink.cpp     82.4%  570 / 692    95.0%  38 / 40
    M2MLink.h       98.3%   57 / 58    100.0%  34 / 34
    M2MMsg.cpp      90.4%  132 / 146    93.8%  15 / 16
    M2MMsgHdr.cpp  100.0%   92 / 92    100.0%   7 / 7
  Taking the mandatory build flag out ahead of release.
                                           ---J. Ian Lindsay 2021.10.16 20:27:54
*/

#ifndef __C3P_XENOSESSION_H
#define __C3P_XENOSESSION_H

#include "../Meta/Rationalizer.h"     // Include build options checking.
#include "../StringBuilder.h"
#include "../CppPotpourri.h"
#include "../AbstractPlatform.h"
#include "../C3PValue/KeyValuePair.h"
#include "../BusQueue/BusQueue.h"
#include "../FlagContainer.h"
#include "../FiniteStateMachine.h"
#include "../PriorityQueue.h"
#include "../ElementPool.h"
#include "../Identity/Identity.h"


/*******************************************************************************
* Parameters from the build system                                             *
*******************************************************************************/

//#define CONFIG_C3P_M2M_SUPPORT 1   // TODO: Until Rationalizer.h is done.

//#ifndef CONFIG_M2MMSG_PREALLOC_COUNT
//  #define CONFIG_M2MMSG_PREALLOC_COUNT   4
//#endif

/*
* We must bound the growth on memory usage, or have our stack be at the mercy
*   of our counterparty's good behavior.
*/
#ifndef CONFIG_C3PLINK_MAX_QUEUE_DEPTH
  #define CONFIG_C3PLINK_MAX_QUEUE_DEPTH   4
#endif

/*
* How many service slots should a link support? Most firmware isn't anticipated
*   to need more than a few, if any.
*/
#ifndef CONFIG_C3PLINK_SERVICE_SLOTS
  #define CONFIG_C3PLINK_SERVICE_SLOTS     2
#endif

#if (CONFIG_C3PLINK_SERVICE_SLOTS > 16)
  #error The value of CONFIG_C3PLINK_SERVICE_SLOTS cannot exceed 16.
#endif

/*******************************************************************************
* Fixed definitions for the M2MLink subsystem                               *
*******************************************************************************/

// This value is our checksum preload. Calculation of new checksums should start
//   with this byte. It helps prevents us from acknowledging spurious data as a
//   connection attempt.
#define M2MLINK_SERIALIZATION_VERSION   1

#define M2MLINK_MAX_PARSE_FAILURES      3  // How many failures-to-parse should we tolerate before SYNCing?
#define M2MLINK_MAX_ACK_FAILURES        3  // How many failures-to-ACK should we tolerate before SYNCing?
#define M2MLINK_MAX_QUEUE_PRINT         3  //
#define M2MLINK_FSM_WAYPOINT_DEPTH      8  // How deep is our state planning?

/* Class flags for M2MLink. */
#define M2MLINK_FLAG_AUTH_REQUIRED   0x00000001  // Set if this session requires authentication.
#define M2MLINK_FLAG_AUTHD           0x00000002  // Set if this session has been authenticated.
#define M2MLINK_FLAG_SYNC_INCOMING   0x00000004  // We've seen a sync on this resync cycle.
#define M2MLINK_FLAG_SYNC_CASTING    0x00000008  // We're sending sync on this resync cycle.
#define M2MLINK_FLAG_SYNC_REPLY_RXD  0x00000010  // We've seen a reply to our syncs on this resync cycle.
#define M2MLINK_FLAG_ESTABLISHED     0x00000020  // We've exchanged CONNECT messages.
#define M2MLINK_FLAG_HANGUP_RXD      0x00000040  // We received a HANGUP message.
#define M2MLINK_FLAG_HANGUP_TXD      0x00000080  // Sent a HANGUP message on this session.
#define M2MLINK_FLAG_SEND_KA         0x00000100  // We will send a keep-alive on a defined interval.
#define M2MLINK_FLAG_ON_HOOK         0x00000200  // Following HANGUP, the app needs to reset this.
#define M2MLINK_FLAG_ALLOW_LOG_WRITE 0x00000400  // Do we allow a counterparty to write to our log?

// These M2MLink flags are allowed to be passed in as configuration.
#define M2MLINK_FLAG_ALLOWABLE_DEFAULT_MASK (M2MLINK_FLAG_AUTH_REQUIRED | \
                                                M2MLINK_FLAG_SEND_KA | \
                                                M2MLINK_FLAG_ALLOW_LOG_WRITE)
// These M2MLink flags survive class reset.
#define M2MLINK_FLAG_RESET_PRESERVE_MASK (M2MLINK_FLAG_ALLOWABLE_DEFAULT_MASK)


/* Class flags for M2MMsg. These are for state tracking, and will NOT be sent with each message. */
#define M2MMSG_FLAG_ACCUMULATOR_COMPLETE     0x01  // The accumulator contains the complete message.
#define M2MMSG_FLAG_TX_COMPLETE              0x02  // This outbound message went to the transport.
#define M2MMSG_FLAG_WAS_ACKD                 0x04  // This outbound message saw a reply come back.


/* Class flags for M2MMsgHdr. These will be sent with each message. */
#define M2MMSGHDR_FLAG_EXPECTING_REPLY       0x01  // This message needs to be ACKd.
#define M2MMSGHDR_FLAG_IS_REPLY              0x02  // This message IS a reply.
#define M2MMSGHDR_FLAG_RESERVED_0            0x04  // Must be 0.
#define M2MMSGHDR_FLAG_RESERVED_1            0x08  // Must be 0.
#define M2MMSGHDR_FLAG_ENCODES_LENGTH_BYTES  0x30  // Mask for 2-bit field. How many bytes of length are encoded?
#define M2MMSGHDR_FLAG_ENCODES_ID_BYTES      0xC0  // Mask for 2-bit field. How many bytes of ID are encoded?

#define M2MMSGHDR_FLAG_RESERVED_MASK (M2MMSGHDR_FLAG_RESERVED_0 | M2MMSGHDR_FLAG_RESERVED_1)

// This value is used to mask-off bytes that are not considered when
//   testing for a sync packet.
#define M2MMSGHDR_FLAG_SYNC_MASK  ~(M2MMSGHDR_FLAG_IS_REPLY | M2MMSGHDR_FLAG_EXPECTING_REPLY)

// The minimum header (thus, message) size.
#define M2MMSGHDR_MINIMUM_HEADER_SIZE  4

// Which bits are not automatic in the header?
#define M2MMSGHDR_SETTABLE_FLAG_BITS (M2MMSGHDR_FLAG_RESERVED_MASK | \
                                         M2MMSGHDR_FLAG_ENCODES_LENGTH_BYTES | \
                                         M2MMSGHDR_FLAG_ENCODES_ID_BYTES)


/*******************************************************************************
* Types                                                                         *
*******************************************************************************/
/*
* These are possible states of the link. They confine the space of our
*   possible dialog, and bias the conversation in a given direction.
*/
enum class M2MLinkState : uint8_t {
  UNINIT         = 0x00,  // Nothing has happened. Freshly-instantiated session.
  PENDING_SETUP  = 0x01,  // Class is clean and ready for a session.
  SYNC_RESYNC    = 0x02,  // Casting sync, and awaiting like replies.
  SYNC_TENTATIVE = 0x03,  // Stop casting sync. Churn until non-sync data arrives.
  PENDING_AUTH   = 0x04,  // Waiting on optional authentication.
  LIVE           = 0x05,  // Session is in the sync'd and connected state.
  PENDING_HANGUP = 0x06,  // Session hangup is imminent.
  HUNGUP         = 0x07,  // Session is hungup and pending cleanup for re-use.
  INVALID
};

/*
* These are possible identifiers for the nature of messages we exchange
*   with a counterparty.
*/
enum class M2MMsgCode : uint8_t {
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
  SERVICE_LIST   = 0x0A,   // A listing of services.
  DHT_FXN        = 0x0E,   // TODO: Future expansion for Link-mediated DHTs.
  APPLICATION    = 0x0F,   // This message carries all interchange with the app.
  SERVICE_SLOT_0 = 0x10,   // A block of 16 codes is reserved for direct-to-service.
  SERVICE_SLOT_1 = 0x11,   //
  SERVICE_SLOT_2 = 0x12,   //
  SERVICE_SLOT_3 = 0x13,   //
  SERVICE_SLOT_4 = 0x14,   //
  SERVICE_SLOT_5 = 0x15,   //
  SERVICE_SLOT_6 = 0x16,   //
  SERVICE_SLOT_7 = 0x17,   //
  SERVICE_SLOT_8 = 0x18,   //
  SERVICE_SLOT_9 = 0x19,   //
  SERVICE_SLOT_A = 0x1A,   //
  SERVICE_SLOT_B = 0x1B,   //
  SERVICE_SLOT_C = 0x1C,   //
  SERVICE_SLOT_D = 0x1D,   //
  SERVICE_SLOT_E = 0x1E,   //
  SERVICE_SLOT_F = 0x1F    //
};

/*
* These are the enumerations of the protocols we intend to support.
* TODO: This might be wrong-headed. Don't use it yet.
*/
enum class M2MLinkProto : uint8_t {
  NATIVE       = 0x00,   // CppPotpourri's native types.
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

class M2MLink;
class M2MMsg;

/* Callback for notifications of link state change. */
typedef void (*M2MLinkCB)(M2MLink*);

/* Callback for application-directed messages from a link. */
typedef void (*M2MMsgCB)(uint32_t tag, M2MMsg*);


/*******************************************************************************
* Class definitions                                                            *
*******************************************************************************/

/*
* Conf representation for a M2MLink.
*
* TODO: Any incompatabilities with counterparties induced by these settings
*   should be reported in the Link's local log.
*/
class M2MLinkOpts {
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

    M2MLinkOpts(const M2MLinkOpts* obj) :
      ms_timeout(obj->ms_timeout),
      ms_keepalive(obj->ms_keepalive),
      mtu(obj->mtu),
      default_flags(M2MLINK_FLAG_ALLOWABLE_DEFAULT_MASK & obj->default_flags),
      max_outbound(obj->max_outbound),
      max_inbound(obj->max_inbound),
      max_parse_errs(obj->max_parse_errs),
      max_ack_fails(obj->max_ack_fails),
      prealloc_count(obj->prealloc_count),
      encoding(obj->encoding) {};

    M2MLinkOpts(uint32_t msto, uint32_t mska, uint32_t m, TCode enc = TCode::BINARY, uint32_t def_flgs = 0) :
      ms_timeout(msto),
      ms_keepalive(mska),
      mtu(m),
      default_flags(M2MLINK_FLAG_ALLOWABLE_DEFAULT_MASK & def_flgs),
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
class M2MMsgHdr {
  public:
    M2MMsgCode    msg_code;  // The message code.
    uint8_t       flags;     // Flags to be encoded into the message.
    uint8_t       chk_byte;  // The expected checksum for the header.
    uint32_t      msg_len;   // Total length of the message (header + payload).
    uint32_t      msg_id;    // A unique ID for this message.

    /* Copy constructor */
    M2MMsgHdr(const M2MMsgHdr* obj) :
      msg_code(obj->msg_code),
      flags(obj->flags),
      chk_byte(obj->chk_byte),
      msg_len(obj->msg_len),
      msg_id(obj->msg_id) {};

    /* General constructor */
    M2MMsgHdr(M2MMsgCode, uint8_t pl_len, uint8_t flags, uint32_t i = 0);

    /* Convenience overloads */
    M2MMsgHdr(M2MMsgCode m) : M2MMsgHdr(m, 0, 0, 0) {};
    //M2MMsgHdr() : M2MMsgHdr(M2MMsgCode::UNDEFINED, 0, 0, 0) {};
    M2MMsgHdr() : msg_code(M2MMsgCode::UNDEFINED), flags(0), chk_byte(0), msg_len(0), msg_id(0) {};


    inline bool isReply() {        return (flags & M2MMSGHDR_FLAG_IS_REPLY);         };
    inline bool expectsReply() {   return (flags & M2MMSGHDR_FLAG_EXPECTING_REPLY);  };
    inline void isReply(bool x) {
      if (x) flags |= M2MMSGHDR_FLAG_IS_REPLY;
      else   flags &= ~M2MMSGHDR_FLAG_IS_REPLY;
    };
    inline void expectsReply(bool x) {
      if (x) flags |= M2MMSGHDR_FLAG_EXPECTING_REPLY;
      else   flags &= ~M2MMSGHDR_FLAG_EXPECTING_REPLY;
    };

    void wipe();
    bool isValid();
    bool isSync();
    bool serialize(StringBuilder*);
    bool set_payload_length(uint32_t);
    int  header_length();
    inline int payload_length() {  return (msg_len - header_length());    };
    inline int total_length() {    return msg_len;  };
    inline uint8_t len_length() {  return ((flags & M2MMSGHDR_FLAG_ENCODES_LENGTH_BYTES) >> 4);  };
    inline uint8_t id_length() {   return ((flags & M2MMSGHDR_FLAG_ENCODES_ID_BYTES) >> 6);      };
    inline uint8_t calc_hdr_chcksm() {
      return (uint8_t) (flags + msg_len + (uint8_t)msg_code + M2MLINK_SERIALIZATION_VERSION);
    };
    inline void rebuild_checksum() {   chk_byte = calc_hdr_chcksm();    };
};


/**
* This class represents an singular message between us and a counterparty.
*/
class M2MMsg {
  public:
    //M2MMsg(KeyValuePair*);  // Construct this way for outbound KVP.
    M2MMsg(M2MMsgHdr*, BusOpcode dir = BusOpcode::RX);
    M2MMsg() {};            // Featureless constructor for static allocation.
    ~M2MMsg();

    /* Accessors for the message header */
    inline void expectsReply(bool);
    inline bool expectsReply() {        return _header.expectsReply();   };
    inline bool isReply() {             return _header.isReply();        };
    inline M2MMsgCode msgCode() {    return _header.msg_code;         };
    inline uint32_t uniqueId() {        return _header.msg_id;           };

    /* Inlines for message options, flags, and status markers. */
    inline TCode     encoding() {     return _encoding;                                           };
    inline BusOpcode direction() {    return _op;                                                 };
    inline uint32_t  msSinceSend() {  return millis_since(_ms_io_mark);                           };
    inline bool      rxComplete() {   return (_accumulator.length() == _header.payload_length()); };
    inline bool      wasSent() {      return _class_flag(M2MMSG_FLAG_TX_COMPLETE);             };
    inline bool      wasACKd() {      return _class_flag(M2MMSG_FLAG_WAS_ACKD);                };
    inline void      markACKd() {     _class_set_flag(M2MMSG_FLAG_WAS_ACKD);                   };

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

    static M2MMsg* unserialize(StringBuilder*);
    static int8_t attempt_header_parse(M2MMsgHdr*, StringBuilder*);


  private:
    M2MMsgHdr _header;
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

    //static const EnumDefList<M2MMsgCode> _MSG_CODES;
};



/*
* This is the interface class that should be implemented by any class that wants
*   to transact over an M2MLink. Such classes are typically purpose-specific
*   modules that respond on a key, and handle the nuances of the message layer.
* Examples of such classes can be found in the LinkUtils folder.
*/
class M2MService {
  public:
    const char* const SVC_TAG;

    void printM2MService(StringBuilder*);


  protected:
    friend class M2MLink;
    M2MLink*     _link;

    M2MService(const char* const ST, M2MLink* l, const uint8_t OBQ_LEN = 2) :
      SVC_TAG(ST), _link(l), _outbound(OBQ_LEN) {};

    ~M2MService();

    inline uint32_t _messages_waiting() {      return _outbound.count();  };
    inline M2MMsg*  _take_msg() {              return _outbound.get();    };

    virtual int8_t _handle_msg(uint32_t tag, M2MMsg*) =0;


  private:
    RingBuffer<M2MMsg*> _outbound;
};


/**
* This class represents an open comm session with a foreign device via an
*   unspecified transport. All we care about is the byte stream.
*/
class M2MLink : public StateMachine<M2MLinkState>, public BufferCoDec {
  public:
    M2MLink(const M2MLinkOpts* opts);
    virtual ~M2MLink();

    /* Implementation of BufferAccepter. Accepts data from a transport. */
    int8_t  pushBuffer(StringBuilder*);
    int32_t bufferAvailable();

    /* Application glue */
    int8_t poll(StringBuilder* log = nullptr);
    int8_t hangup(bool graceful = true);
    int8_t reset();
    int8_t writeRemoteLog(StringBuilder*, bool need_reply = false);
    bool   linkIdle();
    int    send(KeyValuePair*, bool need_reply = false);

    inline bool isConnected() {        return _flags.value(M2MLINK_FLAG_ESTABLISHED);    };
    inline bool requireAuth() {        return _flags.value(M2MLINK_FLAG_AUTH_REQUIRED);  };
    inline void requireAuth(bool x) {  _flags.set(M2MLINK_FLAG_AUTH_REQUIRED, x);        };
    inline bool syncCast() {           return _flags.value(M2MLINK_FLAG_SYNC_CASTING);   };
    inline void syncCast(bool x) {     _flags.set(M2MLINK_FLAG_SYNC_CASTING, x);         };

    /* Debugging */
    void printDebug(StringBuilder*);
    void printQueues(StringBuilder*);

    /* Functions for planning message handling. */
    inline void setCallback(M2MLinkCB cb) {           _lnk_callback = cb;   };
    inline void setCallback(M2MMsgCB cb) {            _msg_callback = cb;   };
    int8_t setCallback(M2MService*);

    /* Inline accessors. */
    inline uint32_t  linkTag() {                      return _session_tag;  };
    inline uint16_t  replyTimeouts() {                return _unackd_sends; };
    inline void      verbosity(uint8_t x) {           _verbosity = x;       };
    inline uint8_t   verbosity() {                    return _verbosity;    };
    inline void      localIdentity(Identity* id) {    _id_loc = id;         };
    inline Identity* localIdentity() {                return _id_loc;       };
    inline Identity* remoteIdentity() {               return _id_remote;    };

    /* Built-in per-instance console handlers. */
    int8_t console_handler(StringBuilder* text_return, StringBuilder* args);

    /* Static support fxns for enums */
    static const char* const sessionStateStr(const M2MLinkState);
    static const char* const msgCodeStr(const M2MMsgCode);
    static const bool  msgCodeValid(const M2MMsgCode);


  private:
    M2MLinkOpts    _opts;    // These are the application-provided options for the link.
    //M2MMsg                 _msg_pool[4];
    //ElementPool<M2MMsg*>   _preallocd(4, &_msg_pool);
    PriorityQueue<M2MMsg*> _outbound_messages;   // Messages that are bound for the counterparty.
    PriorityQueue<M2MMsg*> _inbound_messages;    // Messages that came from the counterparty.
    M2MService*     _svc_list[CONFIG_C3PLINK_SERVICE_SLOTS];  // A list of modules that transact on the link.
    FlagContainer32 _flags;
    uint8_t         _verbosity      = 0;        // By default, this class won't generate logs.
    uint8_t         _seq_parse_errs = 0;
    uint8_t         _seq_ack_fails  = 0;
    uint32_t        _session_tag    = 0;        // Allows the application to keep track of our callbacks.
    uint32_t        _ms_last_send   = 0;        // At what time did the last message go out?
    uint32_t        _ms_last_rec    = 0;        // At what time did the last message come in?
    uint16_t        _sync_losses    = 0;        // How many times this session have we lost sync?
    uint16_t        _unackd_sends   = 0;        // How many messages that needed an ACK failed to get one?
    M2MMsg*         _working        = nullptr;  // If we are in the middle of receiving a message,
    Identity*       _id_loc         = nullptr;
    Identity*       _id_remote      = nullptr;
    M2MLinkCB       _lnk_callback   = nullptr;  // The application-provided callback for state changes.
    M2MMsgCB        _msg_callback   = nullptr;  // The application-provided callback for incoming messages.
    StringBuilder   _inbound_buf;   // TODO: Replace with a RingBuffer<uint8_t> and finally resolve the MTU feature.
    StringBuilder   _remote_log;

    /* Message queue management */
    int8_t _send_msg(M2MMsg*);
    int    _purge_inbound();
    int    _purge_outbound();
    int8_t _churn_inbound();
    int8_t _churn_outbound();
    int8_t _clear_waiting_send_by_id(uint32_t);

    /* Internal handlers for receiving messages confined to this class. */
    int8_t   _handle_msg_log(M2MMsg*);

    /* Buffers, parsing, and scattered low-level functions */
    void   _reset_class();
    int8_t _relay_to_output_target(StringBuilder*);
    int8_t _invoke_msg_callback(M2MMsg*);
    int8_t _offer_msg_to_service(uint8_t slot, M2MMsg*);
    void   _invoke_state_callback();
    int8_t _process_input_buffer();
    int8_t _process_for_sync();

    /* Internal macros for sending messages confined to this class. */
    int8_t _send_sync_packet(bool need_reply);
    int8_t _send_connect_message();
    int8_t _send_hangup_message(bool graceful);
    int8_t _send_who_message();

    /* Mandatory overrides from StateMachine. */
    int8_t _fsm_poll();
    int8_t _fsm_set_position(M2MLinkState);
    int8_t _fsm_insert_sync_states();

    /* Message lifecycle */
    M2MMsg* _allocate_m2mmsg(M2MMsgHdr*, BusOpcode);
    void    _reclaim_m2mmsg(M2MMsg*);


    static const EnumDefList<M2MLinkState> _FSM_STATES;
};

#endif   // __C3P_XENOSESSION_H
