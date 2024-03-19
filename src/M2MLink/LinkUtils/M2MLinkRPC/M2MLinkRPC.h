/*
File:   M2MLinkRPC.h
Author: J. Ian Lindsay
Date:   2024.03.17

M2MLinkRPC_Host and M2MLinkRPC_Client are a pair of classes for creating efficient,
  structured, remote procedure call (RPC) device APIs that have a RESTy flavor.



Constraints and expectations:
--------------------------------------------------------------------------------
MLink may handle and process several messages concurrently, but RPCs are
  executed one-at-a-time in order of their arrival.

RPCs may be split across polling cycles. This is sometimes required to handle
  complex operations that would otherwise be too extravagent of RAM, would
  exceed the counterparty's MTU, etc...
*/

#ifndef __M2MLINK_UTILS_RCP_H_
#define __M2MLINK_UTILS_RCP_H_

#include <functional>
#include "../../M2MLink.h"

#define C3PRPC_CONTEXT_BYTES   32


/*
* This class records the context for a currently-running RPC. A specific RPC can
*   use this class (or not) as it sees fit for accomplishing its tasks without
*   resorting to off-stack storage that has a life-cycle that is independent
*   from that of its workload or messaging.
* The content is wiped clean before execution of a new RPC is begun, and will
*   persist until the RPC's poll() function return indicates that it may be
*   released.
*/
class C3PRPCContext {
  public:
    C3PRPCContext();
    ~C3PRPCContext();

    int8_t init(M2MMsg*);
    void wipe();

    // Accessors to data structures
    inline bool messageHeld() {         return (nullptr != _msg);       };
    inline M2MMsg* message() {          return _msg;                    };
    inline bool hasResponse() {         return (nullptr != _response);  };
    inline KeyValuePair* response() {   return _response;               };

    // Accessors to context bytes


  private:
    M2MMsg*       _msg;                           // Single-slot queue
    KeyValuePair* _response;                      // Single-slot queue
    uint16_t      _mtu;                           // Maximum content length per-message.
    uint16_t      _poll_count;                    // Increments on every poll().
    uint16_t      _msg_id;                        // Increments on every message.
    uint8_t       _cbytes[C3PRPC_CONTEXT_BYTES];  // General scratchpad.
};


/*
* This structure holds the definition for a Remote Procedure Call (RPC). Every
*   exposed RPC should be known and fully-defined at compile time.
*/
typedef struct {
  const char* const  RP_NAME;
  const TCode* const RP_ARGS;
  // This scopeless function is "the procedure" that is "called" "remotely". The
  //   C3PRPCContext is passed in by argument to grease time-slicing.
  // Returns the usual [1, 0, -1] triad to indicate the states ["complete",
  //   "retry", "fail"] (respectively).
  std::function<int8_t(C3PRPCContext*)> POLL_FXN;
} C3PDefinedRPC;



/*******************************************************************************
* Host and Client classes
*******************************************************************************/

/*
* This is the class that is typically implemented device-side.
* There should only be one instance of this class per-Link.
* All incoming RPC requests will be handled by that instance.
*/
class M2MLinkRPC_Host : public M2MService {
  public:
    M2MLinkRPC_Host(M2MLink*, const C3PDefinedRPC* const);
    ~M2MLinkRPC_Host() {};

    /* Implementation of M2MService. Trades messages with a link. */
    virtual int8_t _handle_msg(uint32_t tag, M2MMsg*) =0;
    virtual int8_t _poll_for_link(M2MLink*) =0;


  private:
    const C3PDefinedRPC* const _RPC_LISTING;     // List of RPCs that we are hosting.
    C3PRPCContext  _rpc_context;
    C3PDefinedRPC* _rpc_running;
};


/*
*
*/
class M2MLinkRPC_Client {
  public:
    M2MLinkRPC_Client(M2MLink*);
    ~M2MLinkRPC_Client() {};


  private:
    const C3PDefinedRPC* const _RPC_LISTING;     // List of RPCs available from the counterparty.
    uint8_t _rpc_context[C3PRPC_CONTEXT_BYTES];
};

#endif  // __M2MLINK_UTILS_RCP_H_
