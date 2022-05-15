/*
File:   CryptoBurrito.h
Author: J. Ian Lindsay
Date:   2017.07.16

Copyright 2017 J. Ian Lindsay

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This is the application-facing API.
*/

#include "Cryptographic.h"
#include "../StringBuilder.h"
#include "../PriorityQueue.h"
#include "../AbstractPlatform.h"

#ifndef __CRYPTOBURRITO_H__
#define __CRYPTOBURRITO_H__

/*
* Flags for CryptOp.
*/
#define CRYPTOP_FLAG_FREE_BUFFER   0x40    // If set in a job's flags field, the buffer will be free()'d, if present.
#define CRYPTOP_FLAG_NO_FREE       0x80    // If set in a job's flags field, it will not be free()'d.

#define JOB_Q_CALLBACK_ERROR         -1    // If set in a job's flags field, it will not be free()'d.
#define JOB_Q_CALLBACK_NOMINAL        0    // If set in a job's flags field, it will not be free()'d.
#define JOB_Q_CALLBACK_RECYCLE        1    // If set in a job's flags field, it will not be free()'d.


/*
* These are possible operation states.
*/
enum class CryptOpState {
  /* These are start states. */
  UNDEF = 0,  // Freshly instanced (or wiped, if preallocated).
  IDLE,       // Op is allocated and waiting somewhere outside of the queue.

  /* These states are unstable and should decay into the end state. */
  QUEUED,     // Op is idle and waiting for its turn.
  INITIATE,   // Waiting for initiation phase.
  WAIT,       // Operation in-progress.
  CLEANUP,    // Operation in cleanup phase.

  /* These are end states. */
  COMPLETE    // Op complete with no problems.
};


/*
* These are the opcodes that we use to represent different crypt operations.
* This value impacts the semantics of the buffer. Each should have its own
*   class derived from CryptOp that handles it.
* Every cryptographic operation handled by this library must fit into this enum
*   regardless of implementation.
*/
enum class CryptOpcode {
  UNDEF,          // Freshly instanced (or wiped, if preallocated).
  DIGEST,         // Hash the buffer with the given algo.
  ENCODE,         // Perform a non-cryptographic encoding operation.
  DECODE,         // Perform a non-cryptographic decoding operation.
  ENCRYPT,        // Symmetric cipher with plaintext in the buffer.
  DECRYPT,        // Symmetric decipher with ciphertext in the buffer.
  SIGN,           // Asymmetric signature on the buffer content.
  VERIFY,         // Asymmetric verification of the buffer content.
  KEYGEN,         // Create a new cryptographic key.
  RNG_FILL        // Fill the buffer with random numbers.
};


/*
* Possible fault conditions that might occur.
*/
enum class CryptoFault {
  NONE,            // No error on this operation.
  NO_REASON,       // No reason provided, but still errored.
  UNHANDLED_ALGO,  // A cryptographic process was given a job that it didn't know how to do.
  BAD_PARAM,       // Invalid operation parameters to a known algo.
  ILLEGAL_STATE,   // The operation is in an illegal state.
  TIMEOUT,         // We ran out of patience.
  HW_FAULT,        // Hardware had a meltdown and failed this operation.
  RECALLED,        // The class that spawned this request changed its mind.
  QUEUE_FLUSH      // The work queue was flushed and this was a casualty.
};

/* Forward declarations. */
class CryptOp;
class CryptoProcessor;



/*******************************************************************************
* An interface class that implements a callback path for crypt operations.
*******************************************************************************/
class CryptOpCallback {
  public:
    virtual int8_t op_callahead(CryptOp*) =0;  // Called ahead of op.
    virtual int8_t op_callback(CryptOp*)  =0;  // Called behind completed op.
};



/*******************************************************************************
* A base class for specific crytographic operations.
*******************************************************************************/
class CryptOp {
  public:
    CryptoFault advance();
    void printOp(StringBuilder*);
    void wipe();

    /**
    * @return true if this operation is idle.
    */
    inline bool isIdle() {     return (CryptOpState::IDLE == _op_state);  };

    /**
    * @return true if this operation completed.
    */
    inline bool isComplete() {  return (CryptOpState::COMPLETE == _op_state);  };

    /**
    * @return true if this operation is enqueued and inert.
    */
    inline bool isQueued() {    return (CryptOpState::QUEUED == _op_state);    };

    /**
    * @return true if this operation experienced any abnormal condition.
    */
    inline bool hasFault() {       return (CryptoFault::NONE != _op_fault);  };

    /* Inlines for protected access... */
    inline CryptOpcode  opcode() {                   return _opcode;         };
    inline CryptOpState state() {                    return _op_state;       };
    inline CryptoFault  fault() {                    return _op_fault;       };
    inline CryptOp*     nextStep() {                 return _nxt_step;       };
    inline void nextStep(CryptOp* n_op) {            _nxt_step = n_op;       };
    inline void setBuffer(uint8_t* b, unsigned int bl) {   _buf = b; _buf_len = bl;   };

    /**
    * Set the state-bearing members in preparation for re-queue.
    */
    inline void markForRequeue() {
      _op_fault = CryptoFault::NONE;
      _op_state = CryptOpState::IDLE;
    };

    inline void abort(CryptoFault flt) {
      _op_fault = flt;
      _op_state = CryptOpState::COMPLETE;
    }

    /**
    * CryptoProcessor calls this fxn to decide if it ought to free this object
    *   after completion.
    *
    * @return true if the CryptoProcessor should free() this object. False otherwise.
    */
    inline bool shouldReap() {  return ((_flags & CRYPTOP_FLAG_NO_FREE) == 0);  };

    /**
    * The client class calls this fxn to set this object's post-callback behavior.
    * If this fxn is never called, the default behavior of the class is to allow itself to be free()'d.
    *
    * This flag is preserved by wipe().
    *
    * @param  nu_reap_state Pass false to cause the CryptoProcessor to leave this object alone.
    */
    inline void shouldReap(bool x) {
      _flags = x ? (_flags & (uint8_t) ~CRYPTOP_FLAG_NO_FREE) : (_flags | CRYPTOP_FLAG_NO_FREE);
    };

    /**
    * CryptoProcessor calls this fxn to decide if it ought to free this object's
    *   buffer after callback completion.
    *
    * @return true if the CryptoProcessor should free() this object's buffer. False otherwise.
    */
    inline bool shouldFreeBuffer() {  return ((_flags & CRYPTOP_FLAG_FREE_BUFFER) != 0);  };

    /**
    * The client class calls this fxn to set this object's post-callback behavior.
    * If this fxn is never called, the default behavior of the class is to not
    *   free() the buffer.
    *
    * This flag is preserved by wipe().
    *
    * @param x Pass true to cause the CryptoProcessor to free the buffer.
    */
    inline void shouldFreeBuffer(bool x) {
      _flags = x ? (_flags | CRYPTOP_FLAG_FREE_BUFFER) : (_flags & (uint8_t) ~CRYPTOP_FLAG_FREE_BUFFER);
    };

    /* Statics */
    static const char* opcodeString(CryptOpcode);
    static const char* stateString(CryptOpState);
    static const char* errorString(CryptoFault);
    static void        printCryptOp(const char*, CryptOp*, StringBuilder*);


  protected:
    friend class CryptoProcessor;   // We allow CryptoProcessor to access CryptOps.

    CryptOpCallback* _cb       = nullptr;    // Which class gets pinged when we've finished?
    CryptOpcode      _opcode   = CryptOpcode::UNDEF;   // What is the particular operation being done?
    CryptOpState     _op_state = CryptOpState::UNDEF;  // What state is this operation in?
    CryptoFault      _op_fault = CryptoFault::NONE;    // Fault code.
    CryptOp*         _nxt_step = nullptr;    // Additional jobs following this one.
    uint8_t*         _buf      = nullptr;    // Pointer to the data buffer for the transaction.
    uint32_t         _buf_len  = 0;          // How large is the above buffer?

    /* Protected constructor */
    CryptOp(CryptOpCallback* cb_obj, CryptOpcode o) :
      _cb(cb_obj),
      _opcode(o),
      _op_state(CryptOpState::IDLE),
      _op_fault(CryptoFault::NONE) {};

    virtual ~CryptOp();

    inline int8_t _exec_call_ahead() {   return ((_cb) ? _cb->op_callahead(this) : 0);  };
    inline int8_t _exec_call_back() {    return ((_cb) ? _cb->op_callback(this) : 0);   };

    /* Mandatory overrides... */
    /* Mandatory overrides from the CryptOp interface... */
    virtual CryptoFault _advance() =0;
    virtual void _print(StringBuilder*) =0;
    virtual void _wipe() =0;  // Implementation-specific.


  private:
    uint8_t _flags = 0;        // Encapsulated flags for all instances.
};



/*******************************************************************************
* A virtual class for churnning through cryptographic operations.
*******************************************************************************/
class CryptoProcessor {
  public:
    CryptoProcessor(uint16_t max) : MAX_Q_DEPTH(max) {};
    ~CryptoProcessor() {};

    inline uint8_t  verbosity() {              return _verbosity;    };
    inline void     verbosity(uint8_t v) {     _verbosity = v;       };

    /* Optional overrides. */
    virtual int8_t poll();
    virtual int8_t init();
    virtual int8_t deinit();

    void printDebug(StringBuilder*);
    void printQueues(StringBuilder*, uint8_t max_print = 3);
    inline bool initialized() {  return true;  };

    int8_t queue_job(CryptOp*, int priority = 0);
    int8_t purge_current_job();
    int8_t purge_queued_work();
    int8_t purge_queued_work_by_dev(CryptOpCallback* cb_obj);

    /* Convenience function for guarding against queue floods. */
    inline bool roomInQueue() {    return !(work_queue.size() < MAX_Q_DEPTH);  }


  protected:
    CryptOp* _current_job     = nullptr;
    uint16_t _queue_floods    = 0;  // How many times has the queue rejected work?
    const uint16_t MAX_Q_DEPTH;     // Maximum tolerable queue depth.
    uint32_t _total_jobs      = 0;
    uint32_t _failed_jobs     = 0;
    uint32_t _heap_frees      = 0;
    uint8_t  _verbosity       = 0;  // How much log noise do we make?
    PriorityQueue<CryptOp*> work_queue;      // A work queue to keep transactions in order.
    PriorityQueue<CryptOp*> callback_queue;  // A work queue to keep transactions in order.

    int8_t _advance_work_queue();
    int8_t _advance_callback_queue();
    void _reclaim_queue_item(CryptOp*);
};




/*******************************************************************************
* Platform and application interface.                                          *
*******************************************************************************/

/* typedef'd platform wrapper functions. */
// NOTE: Heap implementations that do not free() need not supply a free_wrap_fxn.
typedef void* (*malloc_wrap_fxn)(size_t);
typedef void (*free_wrap_fxn)(void*);

/*
* Platform should provide a function that the cryptowrapper can call to get an
*   arbitrarily-long random number.
*/
typedef int (*random_fill_fxn)(uint8_t* buf, size_t len, uint32_t opts);

/* typedef'd application callback functions. */
typedef void (*error_cb)(struct oneid_ident_struct_t*, int, const char*, ...);


typedef int (*wrapped_keygen_operation)(
  enum Cipher,      // Algorithm class
  enum CryptoKey,   // Key parameters
  uint8_t* pub,     // Buffer to hold public key.
  size_t* pub_len,  // Length of buffer. Modified to reflect written length.
  uint8_t* priv,    // Buffer to hold private key.
  size_t* priv_len  // Length of buffer. Modified to reflect written length.
);


/* Platform functions. */
class BurritoPlate {
  public:
    const random_fill_fxn  random_fill;     // Mandatory. we call this to get random numbers.
    const malloc_wrap_fxn  malloc_fxn;      // If provided, we call this to heap allocate.
    const free_wrap_fxn    free_fxn;        // If provided, we call this to free heap-alloc'd mem.
    const error_cb         error_fxn;       // If provided, library will report errors this way.

    BurritoPlate() :
      random_fill(nullptr),
      malloc_fxn(nullptr),
      free_fxn(nullptr),
      error_fxn(nullptr) {};

    BurritoPlate(const BurritoPlate* p) :
      random_fill(p->random_fill),
      malloc_fxn(p->malloc_fxn),
      free_fxn(p->free_fxn),
      error_fxn(p->error_fxn) {};

    BurritoPlate(
      random_fill_fxn _rnd,
      malloc_wrap_fxn _ma,
      free_wrap_fxn _mf,
      error_cb _ecb) :
      random_fill(_rnd),
      malloc_fxn(_ma),
      free_fxn(_mf),
      error_fxn(_ecb) {};
};


#endif  // __CRYPTOBURRITO_H__
