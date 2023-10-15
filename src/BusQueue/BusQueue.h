/*
File:   BusQueue/BusQueue.h
Author: J. Ian Lindsay
Date:   2016.05.28

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


TODO: Finish conversion to RingBuffer, and put constraints on BusAdapter's init
  cycle WRT to memory.
*/

#ifndef __ABSTRACT_BUS_QUEUE_H__
#define __ABSTRACT_BUS_QUEUE_H__

#include "CppPotpourri.h"
#include "PriorityQueue.h"
#include "RingBuffer.h"
#include "AbstractPlatform.h"
#include "C3PLogger.h"

// TODO: I suppose it is too late to enum this....
#define BUSOP_CALLBACK_ERROR    -1
#define BUSOP_CALLBACK_NOMINAL   0
#define BUSOP_CALLBACK_RECYCLE   1

/*
* Flags for memory management
* These reside in the BusOp base class, and help automate treatment of memory.
*/
#define BUSOP_FLAG_PROFILE         0x20    // If set, this BusOp contributes to the Adapter's profiling data.
#define BUSOP_FLAG_FREE_BUFFER     0x40    // If set in a transaction's flags field, the buffer will be free()'d, if present.
#define BUSOP_FLAG_NO_FREE         0x80    // If set in a transaction's flags field, it will not be free()'d.


extern "C" {
  #include <stdio.h>   // TODO: Only needed for printf. Do something smarter.
}

/*
* These are possible transfer states.
*/
enum class XferState : uint8_t {
  /* These are start states. */
  UNDEF    = 0,  // Freshly instanced (or wiped, if preallocated).
  IDLE     = 1,  // Bus op is allocated and waiting somewhere outside of the queue.

  /* These states are unstable and should decay into a "finish" state. */
  QUEUED   = 2,  // Bus op is idle and waiting for its turn. No bus control.
  INITIATE = 3,  // Waiting for initiation phase.
  ADDR     = 5,  // Addressing phase. Sending the address.
  TX_WAIT  = 7,  // I/O operation in-progress.
  RX_WAIT  = 8,  // I/O operation in-progress.
  STOP     = 10,  // I/O operation in cleanup phase.

  /* These are finish states. */
  COMPLETE = 14, // I/O op complete with no problems.
  FAULT    = 15  // Fault condition.
};


/*
* These are the opcodes that we use to represent different bus operations.
*/
enum class BusOpcode : uint8_t {
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
enum class XferFault : uint8_t {
  NONE,            // No error on this transfer.
  NO_REASON,       // No reason provided, but still errored.
  TIMEOUT,         // We ran out of patience.
  BAD_PARAM,       // Invalid transfer parameters.
  ILLEGAL_STATE,   // The bus operation is in an illegal state.
  BUS_BUSY,        // The bus didn't get back to us in time.
  BUS_FAULT,       // The bus had a meltdown and failed this transfer.
  DEV_FAULT,       // The device we we transacting with failed this transfer.
  HUNG_IRQ,        // One too many IRQs happened for this operation.
  DMA_FAULT,       // Something went sideways with DMA that wasn't a timeout.
  DEV_NOT_FOUND,   // When an addressed device we expected to find is not found.
  RO_REGISTER,     // We tried to write to a register defined as read-only.
  UNDEFD_REGISTER, // The requested register was not defined.
  IO_RECALL,       // The class that spawned this request changed its mind.
  QUEUE_FLUSH      // The work queue was flushed and this was a casualty.
};

/* Forward declarations. */
class BusOp;


/*
* This is an interface class that implements a callback path for I/O operations.
* If a class wants to put operations into an I/O queue, it must either implement
*   this interface, or delegate its callback duties to a class that does.
* Generally-speaking, this will be a device that transacts on the bus, but is
*   not itself the bus adapter.
*/
class BusOpCallback {
  public:
    virtual int8_t io_op_callahead(BusOp*) =0;  // Called ahead of op.
    virtual int8_t io_op_callback(BusOp*)  =0;  // Called behind completed op.
    virtual int8_t queue_io_job(BusOp*)    =0;  // Queue an I/O operation.
};


/*
* This class represents a single transaction on the bus, but is devoid of
*   implementation details. This is an impure interface class that should be
*   extended by classes that require hardware-level specificity.
*
* State-bearing members in this interface are ok, but there should be no
*   function members that are not pure virtuals or inlines.
* TODO: Since we have local variables, we should try to keep alignment on MOD(4).
*/
class BusOp {
  public:
    BusOpCallback* callback = nullptr;  // Which class gets pinged when we've finished?

    /* Mandatory overrides from the BusOp interface... */
    virtual XferFault begin() =0;
    virtual void wipe() =0;
    virtual void printDebug(StringBuilder*) =0;

    /**
    * @return true if this operation is idle.
    */
    inline bool isIdle() {     return (XferState::IDLE == _xfer_state);  };

    /**
    * @return The initiator's return from the callahead, or zero if no callback object is defined.
    */
    inline int8_t execCA() {   return ((callback) ? callback->io_op_callahead(this) : 0);  };

    /**
    * @return The initiator's return from the callback, or zero if no callback object is defined.
    */
    inline int8_t execCB() {   return ((callback) ? callback->io_op_callback(this) : 0);  };

    /*
    * Accessors for private members.
    */
    inline uint16_t bufferLen() {   return _buf_len;  };
    inline uint8_t* buffer() {      return _buf;      };

    /**
    * Set the buffer.
    * NOTE: There is only ONE buffer, despite the fact that a bus may be full-duplex.
    *
    * @param  buf The transfer buffer.
    * @param  len The length of the buffer.
    */
    inline void setBuffer(uint8_t* b, unsigned int bl) {   _buf = b; _buf_len = bl;   };

    /**
    * This only works because of careful defines. Tread lightly.
    *
    * @return true if this operation completed without problems.
    */
    inline bool has_bus_control() {
      return (
        (_xfer_state == XferState::STOP) | inIOWait() | \
        (_xfer_state == XferState::INITIATE) | (_xfer_state == XferState::ADDR)
      );
    };

    /**
    * @return true if this operation completed without problems.
    */
    inline bool isComplete() {   return (XferState::COMPLETE <= _xfer_state); };

    /**
    * @return true if this operation is enqueued and inert.
    */
    inline bool isQueued() {     return (XferState::QUEUED == _xfer_state);   };
    inline void markQueued() {   set_state(XferState::QUEUED);                };

    /**
    * @return true if this operation is waiting for IO to complete.
    */
    inline bool inIOWait() {
      return (XferState::RX_WAIT == _xfer_state) || (XferState::TX_WAIT == _xfer_state);
    };

    /**
    * @return true if this operation has been intiated, but is not yet complete.
    */
    inline bool inProgress() {
      return (XferState::INITIATE <= _xfer_state) && (XferState::COMPLETE > _xfer_state);
    };

    /**
    * @return true if this operation experienced any abnormal condition.
    */
    inline bool hasFault() {       return (XferFault::NONE != _xfer_fault);  };

    /**
    * @return The current transfer fault.
    */
    inline XferFault getFault() {  return _xfer_fault;                       };

    /**
    * Set the state-bearing members in preparation for re-queue.
    */
    inline void markForRequeue() {
      _xfer_fault = XferFault::NONE;
      _xfer_state = XferState::IDLE;
    };

    /**
    * The bus manager calls this fxn to decide if it ought to free this object
    *   after completion.
    *
    * @return true if the bus manager class should free() this object. False otherwise.
    */
    inline bool shouldReap() {  return ((_flags & BUSOP_FLAG_NO_FREE) == 0);  };

    /**
    * The client class calls this fxn to set this object's post-callback behavior.
    * If this fxn is never called, the default behavior of the class is to allow itself to be free()'d.
    *
    * This flag is preserved by wipe().
    *
    * @param  nu_reap_state Pass false to cause the bus manager to leave this object alone.
    */
    inline void shouldReap(bool x) {
      _flags = x ? (_flags & (uint8_t) ~BUSOP_FLAG_NO_FREE) : (_flags | BUSOP_FLAG_NO_FREE);
    };

    /**
    * The bus manager calls this fxn to decide if it ought to free this object's
    *   buffer after callback completion.
    *
    * @return true if the bus manager class should free() this object's buffer. False otherwise.
    */
    inline bool shouldFreeBuffer() {  return ((_flags & BUSOP_FLAG_FREE_BUFFER) != 0);  };

    /**
    * The client class calls this fxn to set this object's post-callback behavior.
    * If this fxn is never called, the default behavior of the class is to not
    *   free() the buffer.
    *
    * This flag is preserved by wipe().
    *
    * @param x Pass true to cause the bus manager to free the I/O buffer.
    */
    inline void shouldFreeBuffer(bool x) {
      _flags = x ? (_flags | BUSOP_FLAG_FREE_BUFFER) : (_flags & (uint8_t) ~BUSOP_FLAG_FREE_BUFFER);
    };

    /* Inlines for protected access... */
    inline void      set_state(XferState nu) {   _xfer_state = nu;    };   // TODO: Bad. Should be protected. Why isn't it?
    inline void      set_opcode(BusOpcode nu) {  _opcode = nu;        };   // TODO: Bad. Should be protected. Why isn't it?
    inline XferState get_state() {               return _xfer_state;  };
    inline BusOpcode get_opcode() {              return _opcode;      };
    inline bool      hasCallback() {             return (nullptr != callback);  };

    /* Inlines for object-style usage of static functions... */
    inline const char* getOpcodeString() {  return BusOp::getOpcodeString(_opcode);     };
    inline const char* getStateString() {   return BusOp::getStateString(_xfer_state);  };
    inline const char* getErrorString() {   return BusOp::getErrorString(_xfer_fault);  };

    static const char* getStateString(XferState);
    static const char* getOpcodeString(BusOpcode);
    static const char* getErrorString(XferFault);
    static void        printBusOp(const char*, BusOp*, StringBuilder*);


  protected:
    uint8_t*  _buf          = 0;        // Pointer to the data buffer for the transaction.
    uint16_t  _buf_len      = 0;        // How large is the above buffer?
    uint16_t  _extnd_flags  = 0;        // Flags for the concrete class to use.

    inline void set_fault(XferFault nu) {   _xfer_fault = nu;    };

    /* Flag accessor inlines */
    inline uint16_t _busop_flags() {                 return _extnd_flags;            };
    inline bool _busop_flag(uint16_t _flag) {        return (_extnd_flags & _flag);  };
    inline void _busop_clear_flag(uint16_t _flag) {  _extnd_flags &= ~_flag;         };
    inline void _busop_set_flag(uint16_t _flag) {    _extnd_flags |= _flag;          };
    inline void _busop_set_flag(uint16_t _flag, bool nu) {
      if (nu) _extnd_flags |= _flag;
      else    _extnd_flags &= ~_flag;
    };

    static void _busop_wipe(BusOp*);


  private:
    uint8_t   _flags        = 0;        // Encapsulated flags for all BusOp instances.
    BusOpcode _opcode       = BusOpcode::UNDEF;  // What is the particular operation being done?
    XferState _xfer_state   = XferState::UNDEF;  // What state is this transfer in?
    XferFault _xfer_fault   = XferFault::NONE;   // Fault code.
};


/*
* This class represents a generic bus adapter. It has the queues, and the burden
*   of memory-management.
* Implemented as a template, with a BusOp-derived class that is specific to the
*   bus. See examples for details.
*/
template <class T> class BusAdapter : public BusOpCallback {
  public:
    inline T*      currentJob() {             return current_job;  };
    inline uint8_t adapterNumber() {          return ADAPTER_NUM;  };
    inline uint8_t getVerbosity() {           return _verbosity;   };
    inline void    setVerbosity(uint8_t v) {  _verbosity = v;      };

    /**
    * Return a vacant BusOp to the caller, allocating if necessary.
    *
    * @param  _op   The desired bus operation.
    * @param  _req  The device pointer that is requesting the job.
    * @return an BusOp to be used. Only NULL if out-of-mem.
    */
    T* new_op(BusOpcode _op, BusOpCallback* _req) {
      T* ret = preallocated.get();
      if (nullptr == ret) {
        _prealloc_misses++;
        ret = new T();
      }
      if (nullptr != ret) {
        ret->set_opcode(_op);
        ret->callback = _req;
      }
      return ret;
    };


    /*
    * Call periodically to keep the bus moving.
    * Returns the number of operations cleared.
    */
    inline int8_t poll() {
      return advance_work_queue();
    };


    inline bool busIdle() {
      return !((nullptr != current_job) || work_queue.hasNext());
    };


    /**
    * Purges a stalled job from the active slot.
    */
    void purge_current_job() {
      if (current_job) {
        current_job->abort(XferFault::QUEUE_FLUSH);
        if (current_job->callback) {
          current_job->callback->io_op_callback(current_job);
        }
        reclaim_queue_item(current_job);
        current_job = nullptr;
      }
    };

    /**
    * Purges only the work_queue. Leaves the currently-executing job.
    */
    void purge_queued_work() {
      T* current = work_queue.dequeue();
      while (current) {
        current->abort(XferFault::QUEUE_FLUSH);
        if (current->callback) {
          current->callback->io_op_callback(current);
        }
        reclaim_queue_item(current);
        current = work_queue.dequeue();
      }
    };


    /**
    * Purges only those jobs from the work_queue that are owned by the specified
    *   callback object. Leaves the currently-executing job.
    *
    * @param  dev  The device pointer that owns jobs we wish purged.
    * @return The number of jobs purged.
    */
    int8_t purge_queued_work_by_dev(BusOpCallback* cb_obj) {
      int8_t ret = 0;
      for (int i = 0; i < work_queue.size(); i++) {
        T* current = work_queue.get(i);
        if ((nullptr != current) && (current->callback == cb_obj)) {
          ret++;
          work_queue.remove(current);
          current->abort(XferFault::QUEUE_FLUSH);
          if (current->callback) {
            current->callback->io_op_callback(current);
          }
          reclaim_queue_item(current);   // Delete the queued work AND its buffer.
        }
      }
      return ret;
    };


    /**
    * This fxn will either free() the memory associated with the BusOp object, or it
    *   will return it to the preallocation queue.
    *
    * @param item The BusOp to be reclaimed.
    */
    void reclaim_queue_item(T* op) {
      _total_xfers++;
      if (op->hasFault()) {
        _failed_xfers++;
      }

      uintptr_t obj_addr = ((uintptr_t) op);
      uintptr_t pre_min  = ((uintptr_t) preallocated_bus_jobs);
      uintptr_t pre_max  = pre_min + sizeof(preallocated_bus_jobs);
      if (op->shouldFreeBuffer() && (nullptr != op->buffer())) {
        // Independently of the BusOp's mem management, if the I/O buffer is
        //   marked for reap, do so at this point.
        free(op->buffer());
        op->setBuffer(nullptr, 0);
        op->shouldFreeBuffer(false);
      }

      if ((obj_addr < pre_max) && (obj_addr >= pre_min)) {
        // If we are in this block, it means obj was preallocated. wipe and reclaim it.
        return_op_to_pool(op);
      }
      else if (op->shouldReap()) {
        // We were created because our prealloc was starved. we are therefore a transient heap object.
        if (getVerbosity() >= LOG_LEV_DEBUG) c3p_log(LOG_LEV_DEBUG, __PRETTY_FUNCTION__, "About to reap.");
        delete op;
        _heap_frees++;
      }
      else {
        /* If we are here, it must mean that some other class fed us a const BusOp
        and wants us to ignore the memory cleanup. But we should at least set it
        back to IDLE.*/
        if (getVerbosity() >= LOG_LEV_DEBUG) c3p_log(LOG_LEV_DEBUG, __PRETTY_FUNCTION__, "Dropping...");
        op->set_state(XferState::IDLE);
      }
    }


    /* Convenience function for guarding against queue floods. */
    inline bool roomInQueue() {    return (work_queue.size() < MAX_Q_DEPTH);  };


    // TODO: I hate that I'm doing this in a template.
    void printAdapter(StringBuilder* output) {
      output->concatf("-- Adapter #%u\n", ADAPTER_NUM);
      output->concatf("-- Xfers (fail/total)  %u/%u\n", _failed_xfers, _total_xfers);
      output->concat("-- Prealloc:\n");
      output->concatf("--\tavailable        %d\n",  preallocated.count());
      output->concatf("--\tmisses/frees     %u/%u\n", _prealloc_misses, _heap_frees);
      output->concat("-- Work queue:\n");
      output->concatf("--\tdepth/max        %u/%u\n", work_queue.size(), MAX_Q_DEPTH);
      output->concatf("--\tfloods           %u\n",  _queue_floods);
    };

    // TODO: I hate that I'm doing this in a template.
    void printWorkQueue(StringBuilder* output, int8_t max_print) {
      if (current_job) {
        output->concat("--\n- Current active job:\n");
        current_job->printDebug(output);
      }
      else {
        output->concat("--\n-- No active job.\n--\n");
      }
      int wqs = work_queue.size();
      if (wqs > 0) {
        int print_depth = strict_min((int8_t) wqs, max_print);
        output->concatf("-- Queue Listing (top %d of %d total)\n", print_depth, wqs);
        for (int i = 0; i < print_depth; i++) {
          work_queue.get(i)->printDebug(output);
        }
      }
      else {
        output->concat("-- Empty queue.\n");
      }
    };


  protected:
    const uint8_t  ADAPTER_NUM;     // The platform-relatable index of the adapter.
    const uint8_t  MAX_Q_DEPTH;     // Maximum tolerable queue depth.
    uint16_t _queue_floods    = 0;  // How many times has the queue rejected work?
    uint16_t _prealloc_misses = 0;  // How many times have we starved the preallocation queue?
    uint16_t _heap_frees      = 0;  // How many times have we freed a BusOp?
    T*       current_job      = nullptr;
    PriorityQueue<T*> work_queue;   // A work queue to keep transactions in order.
    RingBuffer<T*> preallocated;    //
    T preallocated_bus_jobs[14];


    BusAdapter(uint8_t anum, uint8_t maxq) : ADAPTER_NUM(anum), MAX_Q_DEPTH(maxq), preallocated(8) {};

    /*
    * Wipe all of our preallocated BusOps and pass them into the prealloc queue.
    */
    void _memory_init() {
      if (preallocated.allocated()) {
        for (uint8_t i = 0; i < (sizeof(preallocated_bus_jobs) / sizeof(preallocated_bus_jobs[0])); i++) {
          preallocated_bus_jobs[i].wipe();
          preallocated.insert(&preallocated_bus_jobs[i]);
        }
      }
    };

    /*
    * Returns a BusOp to the preallocation pool.
    */
    inline void return_op_to_pool(T* obj) {
      obj->wipe();
      preallocated.insert(obj);
    };

    /* Mandatory overrides for extending classes... */
    virtual int8_t advance_work_queue() =0;  // The nature of the bus dictates this implementation.
    virtual int8_t bus_init()           =0;  // Hardware-specifics.
    virtual int8_t bus_deinit()         =0;  // Hardware-specifics.
    //virtual int8_t io_op_callback(T*)   =0;  // From BusOpCallback
    //virtual int8_t queue_io_job(T*)     =0;  // From BusOpCallback

    // These inlines are for convenience of extending classes.
    inline uint16_t _adapter_flags() {                 return _extnd_state;            };
    inline bool _adapter_flag(uint16_t _flag) {        return (_extnd_state & _flag);  };
    inline void _adapter_clear_flag(uint16_t _flag) {  _extnd_state &= ~_flag;         };
    inline void _adapter_set_flag(uint16_t _flag) {    _extnd_state |= _flag;          };
    inline void _adapter_set_flag(uint16_t _flag, bool nu) {
      if (nu) _extnd_state |= _flag;
      else    _extnd_state &= ~_flag;
    };


  private:
    uint32_t _total_xfers   = 0;  // Transfer stats.
    uint32_t _failed_xfers  = 0;  // Transfer stats.
    uint16_t _extnd_state   = 0;  // Flags for the concrete class to use.
    uint8_t  _verbosity     = 0;  // How much log noise do we make?
};

#endif  // __ABSTRACT_BUS_QUEUE_H__
