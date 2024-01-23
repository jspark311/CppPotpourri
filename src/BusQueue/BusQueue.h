/*
File:   BusQueue.h
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

#include "../Meta/Rationalizer.h"
#include "../CppPotpourri.h"
#include "../StringBuilder.h"
#include "../PriorityQueue.h"
#include "../ElementPool.h"
#include "../AbstractPlatform.h"
#include "../FlagContainer.h"
#include "../C3PLogger.h"
#include "../TimerTools/TimerTools.h"

/*******************************************************************************
* Types and definitions
*******************************************************************************/
class BusOp;   // For-dec

/* Possible transfer states */
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


/* Possible bus operations. */
enum class BusOpcode : uint8_t {
  UNDEF,          // Freshly instanced (or wiped, if preallocated).
  RX,             // We are receiving without having asked for it.
  TX,             // Simple transmit. No waiting for a reply.
  TX_WAIT_RX,     // Send to the bus and capture the reply.
  TX_CMD,         // Send to the bus command register without expecting a reply.
  TX_CMD_WAIT_RX  // Send to the bus command register and capture a reply.
};


/* Possible fault conditions that might occur */
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


/*******************************************************************************
* BusOpCallback (the callback interface)
* This is an interface class that implements a callback path for I/O operations.
* If a class wants to put operations into an I/O queue, it must either implement
*   this interface, or delegate its callback duties to a class that does.
* Generally-speaking, this will be a device that transacts on the bus, but is
*   not itself the bus adapter.
*******************************************************************************/
// When the BusOpCallback object is called at the end of a job, it can return one
//   of these codes to instruct the BusAdpater what to do next regarding the job.
// TODO: I suppose it is too late to enum this....
#define BUSOP_CALLBACK_ERROR   -1  // Fail the transfer. Contributes to stats. Prints job.
#define BUSOP_CALLBACK_NOMINAL  0  // Nominal callback return code.
#define BUSOP_CALLBACK_RECYCLE  1  // Requeue the job for another transfer.

class BusOpCallback {
  public:
    virtual int8_t io_op_callahead(BusOp*) =0;  // Called ahead of op.
    virtual int8_t io_op_callback(BusOp*)  =0;  // Called behind completed op.
    virtual int8_t queue_io_job(BusOp*)    =0;  // Queue an I/O operation.
};


/*******************************************************************************
* BusOp
* This class represents a single transaction on the bus, but is devoid of
*   implementation details. This is an impure interface class that should be
*   extended by classes that require hardware-level specificity.
*
* NOTE: State-bearing members in this interface are ok, but there should be no
*   function members that are not pure virtuals or inlines.
*
* TODO: Make this use the FSM template?
*******************************************************************************/
// Flags for BusOps. These reside in the BusOp base class, and help automate
//   treatment of memory, profiling, etc.
#define BUSOP_FLAG_PROFILE         0x20    // If set, this BusOp contributes to the Adapter's profiling data.
#define BUSOP_FLAG_FREE_BUFFER     0x40    // If set in a transaction's flags field, the buffer will be free()'d, if present.
#define BUSOP_FLAG_NO_FREE         0x80    // If set in a transaction's flags field, it will not be free()'d.

class BusOp {
  public:
    BusOpCallback* callback;  // Which class gets pinged when we've finished?

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
    uint8_t*  _buf;           // Pointer to the data buffer for the transaction.
    uint16_t  _buf_len;       // How large is the above buffer?
    uint16_t  _extnd_flags;   // Flags for the concrete class to use.

    BusOp(BusOpCallback* cb) : callback(cb), _buf(nullptr), _buf_len(0),
        _extnd_flags(0), _flags(0), _opcode(BusOpcode::UNDEF),
        _xfer_state(XferState::IDLE), _xfer_fault(XferFault::NONE) {};
    BusOp() : BusOp(nullptr) {};
    ~BusOp() {};

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
    uint8_t   _flags;        // Encapsulated flags for all BusOp instances.
    BusOpcode _opcode;       // What is the particular operation being done?
    XferState _xfer_state;   // What state is this transfer in?
    XferFault _xfer_fault;   // Fault code.
};


/*******************************************************************************
* BusAdapter (the adapter interface)
* This class represents a generic bus adapter. It has the queues, and the burden
*   of memory-management.
* Implemented as a template, with a BusOp-derived class that is specific to the
*   bus. See examples for details.
*******************************************************************************/
// Flags for BusAdapters. These reside in the BusAdapter base class, and track
//   flags that are common to all BusAdapters. There aren't many...
// These occupy the top of the flag space. The lower 16-bits is exposed to any
//  child class that wants to use them.
#define BUSADAPTER_FLAG_PF_ADVANCE_OPS 0x01000000   // Adapter.poll() advances BusOps?
#define BUSADAPTER_FLAG_PF_BEGIN_ASAP  0x02000000   // Synchronously dispatch BusOps?
#define BUSADAPTER_FLAG_BUS_FAULT      0x04000000   // The adapter has an unrecoverable fault.
#define BUSADAPTER_FLAG_BUS_ONLINE     0x08000000   // The adapter is online.
#define BUSADAPTER_FLAG_PF_VERBOSITY   0xE0000000   // NOTE: This is a MASK. Not a single bit.

template <class T> class BusAdapter : public BusOpCallback, public C3PPollable {
  public:
    StopWatch profiler_poll;    // Profiler for bureaucracy within BusAdapter.

    /* Inline state accessors */
    inline T*      currentJob() {     return current_job;  };
    inline uint8_t adapterNumber() {  return ADAPTER_NUM;  };
    inline bool    roomInQueue() {    return (work_queue.size() < MAX_Q_DEPTH);                    };
    inline bool    busIdle() {        return !((nullptr != current_job) || work_queue.hasNext());  };
    inline bool    busError() {       return (_flags.value(BUSADAPTER_FLAG_BUS_FAULT));            };
    inline bool    busOnline() {      return (_flags.value(BUSADAPTER_FLAG_BUS_ONLINE));           };
    inline uint8_t getVerbosity() {   return ((_flags.raw & BUSADAPTER_FLAG_PF_VERBOSITY) >> 29);  };
    inline void    setVerbosity(uint8_t v) {
      _flags.raw = ((((uint32_t) v) << 29) | (_flags.raw & ~BUSADAPTER_FLAG_PF_VERBOSITY));
    };


    /**
    * Return a vacant BusOp to the caller, allocating if necessary.
    *
    * @param  _op   The desired bus operation.
    * @param  _req  The device pointer that is requesting the job.
    * @return an BusOp to be used. Only NULL if out-of-mem.
    */
    T* new_op(BusOpcode _op, BusOpCallback* _req) {
      T* ret = preallocated.take();
      if (nullptr != ret) {
        ret->shouldReap(!preallocated.inPool(ret));
        ret->set_opcode(_op);
        ret->callback = _req;
      }
      return ret;
    };


    /*
    * Call periodically to keep the bus moving.
    * Returns the number of operations cleared.
    */
    PollResult poll() {
      profiler_poll.markStart();
      int8_t ret = _bus_poll();
      if (0 == ret) {
        return PollResult::NO_ACTION;
      }
      else if (ret > 0) {
        profiler_poll.markStop();
        return (busIdle() ? PollResult::ACTION : PollResult::REPOLL);
      }
      else {
        return PollResult::ERROR;
      }
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
        _reclaim_queue_item(current_job);
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
        _reclaim_queue_item(current);
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
          _reclaim_queue_item(current);   // Delete the queued work AND its buffer.
        }
      }
      return ret;
    };


    // TODO: I hate that I'm doing this in a template.
    void printAdapter(StringBuilder* output) {
      StringBuilder tmp_str;
      tmp_str.concatf("Adapter #%u (%sline)", ADAPTER_NUM, (busOnline() ? "on" : "off"));
      StringBuilder::styleHeader2(output, (char*) tmp_str.string());

      output->concatf("\tXfers (fail/total)  %u/%u\n", _failed_xfers, _total_xfers);
      output->concat("\tPrealloc:\n");
      output->concatf("\t  available        %d/%d\n", preallocated.available(), preallocated.capacity());
      output->concatf("\t  misses/frees     %u/%u\n", preallocated.overdraws(), preallocated.overdrawsFreed());
      output->concat("\tWork queue:\n");
      output->concatf("\t  depth/max        %u/%u\n", work_queue.size(), MAX_Q_DEPTH);
      output->concatf("\t  floods           %u\n",  _queue_floods);
      StopWatch::printDebugHeader(output);
      profiler_poll.printDebug("poll()", output);
    };


    // TODO: I hate that I'm doing this in a template.
    void printWorkQueue(StringBuilder* output, int8_t max_print) {
      StringBuilder tmp_str;
      tmp_str.concatf("Adapter #%u Queue", ADAPTER_NUM);
      StringBuilder::styleHeader2(output, (char*) tmp_str.string());

      if (current_job) {
        output->concat("-- Current active job:\n");
        current_job->printDebug(output);
      }
      else {
        output->concat("-- No active job.\n--\n");
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
    uint16_t _queue_floods;         // How many times has the queue rejected work?
    T*       current_job;
    PriorityQueue<T*> work_queue;   // A work queue to keep transactions in order.
    ElementPool<T> preallocated;    //

    BusAdapter(uint8_t anum, uint32_t PA_COUNT, uint8_t maxq) :
      ADAPTER_NUM(anum), MAX_Q_DEPTH(maxq), _queue_floods(0),
      current_job(nullptr), preallocated(PA_COUNT, MAX_Q_DEPTH) {};


    /*
    * Wipe all of our preallocated BusOps and pass them into the prealloc queue.
    */
    int8_t _memory_init() {
      int8_t ret = -1;
      if (preallocated.allocated()) {
        for (uint8_t i = 0; i < preallocated.capacity(); i++) {
          T* pa_busop = preallocated.take();
          pa_busop->shouldReap(false);
          preallocated.give(pa_busop);
        }
        ret = 0;
      }
      return ret;
    };


    /**
    * This fxn will either free() the memory associated with the BusOp object, or it
    *   will return it to the preallocation queue.
    *
    * @param item The BusOp to be reclaimed.
    */
    void _reclaim_queue_item(T* op) {
      _total_xfers++;
      if (op->hasFault()) {
        _failed_xfers++;
      }

      if (op->shouldFreeBuffer() && (nullptr != op->buffer())) {
        // Independently of the BusOp's mem management, if the I/O buffer is
        //   marked for reap, do so at this point.
        free(op->buffer());
        op->setBuffer(nullptr, 0);
        op->shouldFreeBuffer(false);
      }

      if (op->shouldReap() || preallocated.inPool(op)) {
        // If we are in this block, it means obj was preallocated. wipe and reclaim it.
        op->wipe();
        preallocated.give(op);
      }
      else {
        /* If we are here, it must mean that some other class fed us a const BusOp
        and wants us to ignore the memory cleanup. But we should at least set it
        back to IDLE.*/
        if (getVerbosity() >= LOG_LEV_DEBUG) { c3p_log(LOG_LEV_DEBUG, "BusAdapter", "BusAdapter::reclaim_queue_item(): \t Dropping...."); }
        op->set_state(XferState::IDLE);
      }
    }


    /*
    * These inlines are for convenience of extending classes.
    * The exposure is 16-bit, and it will be stored in the low half of our
    *   32-bit flags member. We do this to achieve good alignments, and prevent
    *   the need of the child class doing the same thing. It WILL have flags.
    */
    inline uint16_t _adapter_flags() {                     return ((uint16_t) _flags.raw & 0xFFFF);   };
    inline bool _adapter_flag(uint16_t x) {                return _flags.value((uint32_t) x);         };
    inline void _adapter_clear_flag(uint16_t x) {          _flags.clear((uint32_t) x);                };
    inline void _adapter_set_flag(uint16_t x) {            _flags.set((uint32_t) x);                  };
    inline void _adapter_set_flag(uint16_t x, bool nu) {   _flags.set((uint32_t) x, nu);              };

    /* BusAdapter's common flags occupy the high half of the flags. */
    inline void _bus_error(bool x) {            _flags.set(BUSADAPTER_FLAG_BUS_FAULT, x);             };
    inline void _bus_online(bool x) {           _flags.set(BUSADAPTER_FLAG_BUS_ONLINE, x);            };
    inline void _pf_needs_op_advance(bool x) {  _flags.set(BUSADAPTER_FLAG_PF_ADVANCE_OPS, x);        };
    inline bool _pf_needs_op_advance() {        return _flags.value(BUSADAPTER_FLAG_PF_ADVANCE_OPS);  };
    inline void _pf_dispatch_on_queue(bool x) { _flags.set(BUSADAPTER_FLAG_PF_BEGIN_ASAP, x);         };
    inline bool _pf_dispatch_on_queue() {       return _flags.value(BUSADAPTER_FLAG_PF_BEGIN_ASAP);   };

    /*
    * Mandatory overrides for extending classes. The nature of the bus dictates
    *   how these are implementated.
    */
    virtual int8_t _bus_init()   =0;
    virtual int8_t _bus_poll()   =0;
    virtual int8_t _bus_deinit() =0;


  private:
    uint32_t _total_xfers   = 0;  // Transfer stats.
    uint32_t _failed_xfers  = 0;  // Transfer stats.
    FlagContainer32 _flags;
};

#endif  // __ABSTRACT_BUS_QUEUE_H__
