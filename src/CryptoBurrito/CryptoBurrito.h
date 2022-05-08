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

#ifndef __CRYPTOBURRITO_H__
#define __CRYPTOBURRITO_H__

// Try to contain wrapped header concerns in here, pl0x...
#include "CryptOptUnifier.h"


/*
* These are possible operation states.
*/
enum class CryptOpState {
  /* These are start states. */
  UNDEF    = 0,  // Freshly instanced (or wiped, if preallocated).
  IDLE     = 1,  // Op is allocated and waiting somewhere outside of the queue.

  /* These states are unstable and should decay into a "finish" state. */
  QUEUED   = 2,  // Op is idle and waiting for its turn.
  INITIATE = 3,  // Waiting for initiation phase.
  WAIT     = 7,  // Operation in-progress.
  STOP     = 10, // Operation in cleanup phase.

  /* These are finish states. */
  COMPLETE = 14, // Op complete with no problems.
  FAULT    = 15  // Fault condition.
};


/*
* These are the opcodes that we use to represent different crypt operations.
*/
enum class CryptOpcode {
  UNDEF,          // Freshly instanced (or wiped, if preallocated).
  RX,             // We are receiving without having asked for it.
  TX,             // Simple transmit. No waiting for a reply.
  TX_WAIT_RX,     // Send to the bus and capture the reply.
  TX_CMD,         // Send to the bus command register without expecting a reply.
  TX_CMD_WAIT_RX  // Send to the bus command register and capture a reply.
};


/*
* Possible fault conditions that might occur.
*/
enum class CryptoFault {
  NONE,            // No error on this operation.
  NO_REASON,       // No reason provided, but still errored.
  TIMEOUT,         // We ran out of patience.
  BAD_PARAM,       // Invalid operation parameters.
  ILLEGAL_STATE,   // The bus operation is in an illegal state.
  BUS_BUSY,        // The bus didn't get back to us in time.
  BUS_FAULT,       // The bus had a meltdown and failed this operation.
  DEV_FAULT,       // The device we we transacting with failed this operation.
  HUNG_IRQ,        // One too many IRQs happened for this operation.
  DMA_FAULT,       // Something went sideways with DMA that wasn't a timeout.
  DEV_NOT_FOUND,   // When an addressed device we expected to find is not found.
  RO_REGISTER,     // We tried to write to a register defined as read-only.
  UNDEFD_REGISTER, // The requested register was not defined.
  IO_RECALL,       // The class that spawned this request changed its mind.
  QUEUE_FLUSH      // The work queue was flushed and this was a casualty.
};

/* Forward declarations. */
class CryptOp;

/*
* This is an interface class that implements a callback path for crypt operations.
*/
class CryptOpCallback {
  public:
    virtual int8_t op_callahead(CryptOp*) =0;  // Called ahead of op.
    virtual int8_t op_callback(CryptOp*)  =0;  // Called behind completed op.
    virtual int8_t queue_job(CryptOp*)    =0;  // Queue an operation.
};


/*
*/
class CryptOp {
  public:
    CryptOpCallback* callback = nullptr;  // Which class gets pinged when we've finished?
    uint8_t* buf            = 0;        // Pointer to the data buffer for the transaction.
    uint16_t buf_len        = 0;        // How large is the above buffer?
    //uint32_t time_began     = 0;        // This is the time when bus access begins.
    //uint32_t time_ended     = 0;        // This is the time when bus access stCryptOp (or is aborted).

    /* Mandatory overrides from the CryptOp interface... */
    //virtual CryptoFault advance() =0;
    virtual CryptoFault begin() =0;
    virtual void wipe()  =0;
    virtual void printDebug(StringBuilder*)  =0;

    /**
    * @return true if this operation is idle.
    */
    inline bool isIdle() {     return (CryptOpState::IDLE == op_state);  };

    /**
    * @return true if this operation is idle.
    */
    inline int8_t execCA() {   return ((callback) ? callback->op_callahead(this) : 0);  };

    /**
    * @return true if this operation is idle.
    */
    inline int8_t execCB() {   return ((callback) ? callback->op_callback(this) : 0);  };

    /**
    * This only works because of careful defines. Tread lightly.
    *
    * @return true if this operation completed without problems.
    */
    inline bool has_bus_control() {
      return (
        (op_state == CryptOpState::STOP) | (op_state == CryptOpState::WAIT) | (op_state == CryptOpState::INITIATE)
      );
    };

    /**
    * @return true if this operation completed without problems.
    */
    inline bool isComplete() {   return (CryptOpState::COMPLETE <= op_state);  };

    /**
    * @return true if this operation is enqueued and inert.
    */
    inline bool isQueued() {     return (CryptOpState::QUEUED == op_state);    };
    inline void markQueued() {   set_state(CryptOpState::QUEUED);                };

    /**
    * @return true if this operation has been intiated, but is not yet complete.
    */
    inline bool inProgress() {
      return (CryptOpState::INITIATE <= op_state) && (CryptOpState::COMPLETE > op_state);
    };

    /**
    * @return true if this operation experienced any abnormal condition.
    */
    inline bool hasFault() {     return (CryptoFault::NONE != op_fault);     };


    /* Inlines for protected access... TODO: These should be eliminated over time. */
    inline CryptOpState get_state() {                 return op_state;       };
    inline void         set_state(CryptOpState nu) {  op_state = nu;         };
    inline CryptOpcode  get_opcode() {                return opcode;         };
    inline void         set_opcode(CryptOpcode nu) {  opcode = nu;           };

    /* Inlines for object-style usage of static functions... */
    inline const char* getOpcodeString() {  return CryptOp::getOpcodeString(opcode);     };
    inline const char* getStateString() {   return CryptOp::getStateString(op_state);  };
    inline const char* getErrorString() {   return CryptOp::getErrorString(op_fault);  };


    static const char* getStateString(CryptOpState);
    static const char* getOpcodeString(CryptOpcode);
    static const char* getErrorString(CryptoFault);
    static void        printCryptOp(const char*, CryptOp*, StringBuilder*);


  protected:
    uint8_t   _flags   = 0;                 // Specifics are left to the extending class.
    CryptOpcode opcode = CryptOpcode::UNDEF;  // What is the particular operation being done?
    CryptOpState op_state = CryptOpState::UNDEF;  // What state is this operation in?
    CryptoFault op_fault = CryptoFault::NONE;   // Fault code.

    //static void        initCryptOp(const char*, CryptOp*, StringBuilder*);


  private:
};


template <class T> class RNGDevice {
};


/*
*/
template <class T> class CryptoProvider : public CryptOpCallback {
  public:
    inline T* currentJob() {  return current_job;  };


  protected:
    T*       current_job      = nullptr;
    uint32_t _total_ops     = 0;  // Transfer stats.
    uint32_t _failed_ops    = 0;  // Transfer stats.
    uint16_t _prealloc_misses = 0;  // How many times have we starved the preallocation queue?
    uint16_t _heap_frees      = 0;  // How many times have we freed a CryptOp?
    uint16_t _queue_floods    = 0;  // How many times has the queue rejected work?
    const uint16_t MAX_Q_DEPTH;     // Maximum tolerable queue depth.
    //TODO: const uint8_t  MAX_Q_PRINT;     // Maximum tolerable queue depth.
    //TODO: const uint8_t  PREALLOC_SIZE;   // Maximum tolerable queue depth.
    PriorityQueue<T*> work_queue;   // A work queue to keep transactions in order.
    PriorityQueue<T*> preallocated; // TODO: Convert to ring buffer. This is the whole reason you embarked on this madness.

    CryptoProvider(uint16_t max) : MAX_Q_DEPTH(max) {};

    /* Mandatory overrides... */
    virtual int8_t advance_work_queue() =0;  // The nature of the bus dictates this implementation.
    virtual int8_t bus_init()           =0;  // Hardware-specifics.
    virtual int8_t bus_deinit()         =0;  // Hardware-specifics.

    void return_op_to_pool(T* obj) {
      obj->wipe();
      preallocated.insert(obj);
    };

    /**
    * Return a vacant CryptOp to the caller, allocating if necessary.
    *
    * @return an CryptOp to be used. Only NULL if out-of-mem.
    */
    T* new_op() {
      T* return_value = preallocated.dequeue();
      if (nullptr == return_value) {
        _prealloc_misses++;
        return_value = new T();
      }
      return return_value;
    };

    ///**
    //* Return a vacant CryptOp to the caller, allocating if necessary.
    //*
    //* @param  _op   The desired bus operation.
    //* @param  _req  The device pointer that is requesting the job.
    //* @return an CryptOp to be used. Only NULL if out-of-mem.
    //*/
    //T* new_op(CryptOpcode _op, CryptOpCallback* _req) {
    //  T* return_value = new T(_op, _req);
    //  return return_value;
    //};

    ///*
    //* Purges only the work_queue. Leaves the currently-executing job.
    //*/
    //void purge_queued_work() {
    //  T* current = work_queue.dequeue();
    //  while (current) {
    //    current->abort(CryptoFault::QUEUE_FLUSH);
    //    if (current->callback) {
    //      current->callback->io_op_callback(current);
    //    }
    //    reclaim_queue_item(current);
    //    current = work_queue.dequeue();
    //  }
    //};


    /* Convenience function for guarding against queue floods. */
    inline bool roomInQueue() {    return !(work_queue.size() < MAX_Q_DEPTH);  }



  private:
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
      error_fxn(nullptr)
      {};

    BurritoPlate(const BurritoPlate* p) :
      random_fill(p->random_fill),
      malloc_fxn(p->malloc_fxn),
      free_fxn(p->free_fxn),
      error_fxn(p->error_fxn)
      {};

    BurritoPlate(
      random_fill_fxn _rnd,
      malloc_wrap_fxn _ma,
      free_wrap_fxn _mf,
      error_cb _ecb) :
      random_fill(_rnd),
      malloc_fxn(_ma),
      free_fxn(_mf),
      error_fxn(_ecb)
      {};


  private:
};


#endif  // __CRYPTOBURRITO_H__
