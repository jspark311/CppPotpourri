/*
File:   C3PEvents.h
Author: J. Ian Lindsay
Date:   2022.04.24

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


These classes descend from ManuvrOS's core classes for asynchronous execution:
Kernel, EventReceiver, ManuvrMsg, and Scheduler.

Lessons learned from ManuvrOS:
--------------------------------------------------------------------------------
Originally, ManuvrOS's Kernel was intended to operate as pub-sub observer
  pattern, with observing drivers extending the EventReceiver virtual class and
  registering themselves with the Kernel upon instantiation. Events were handled
  by priority (first) and insertion order (second) via a callback defined in by
  the base class. Callbacks in EventReceiver would be called with a single
  ManuvrMsg as an argument.
This pattern functioned quite well for many applications, despite its inherent
  limitations with respect to event order and priority control.

All of these factors contributed to the putrefaction of the code base:
  1) EventReceiver became a catch-all for features that should have been handled
     independently. Logging (since solved), console handling (since solved),
     and asynchronicity of program operations all depended on extending an
     overly-complicated base class.
  2) The Kernel class (which was a singleton) became too intermingled with the
     Scheduler class and logging functions, thus necessitating platform
     non-generality.
  3) The event carrier class (ManuvrMsg) became too intermingled with schedules,
     and had an assigned ID map that implied hard-coded behaviors across modules
     that might respond to them. Combined with the ossification of
     EventReceiver, the class became brittle and evolved by agglomeration.
  4) ManuvrMsg became too intermingled with the Argument class (now
     KeyValuePair) for the sake of holding working data across parallel stacks.

Kernel's API allowed global event broadcasts in addition to the preferred
  pattern of publish-subscribe. But because it was a singleton, and the possible
  combination of EventReceivers in a given program couldn't be predicted, the
  only reliable practice was often to use global broadcasts. This lead to
  horrendous CPU inefficiencies as the number of EventReceivers increased.


Kernel originally did two things right:
  1) It concentrated event handling in an orderly and thread-safe manner with a
     single polling loop for the entire program.
  2) It handled event scheduling in tandem with the on-demand event system, and
     did so with a simple API.

The architectural solution to all of this should be...
  1) Make tightly-restricted interface classes (or callback patterns) to replace
     the legitimate roles that EventReceiver/ManuvrMsg filled.
  2) Do away with the pub-sub pattern in Kernel, and replace it with a filter,
     or possibly nothing.
  3) Drop the singleton pattern in Kernel, and allow a B-tree relationship
     of (possibly) heterogeneous "event queues" to allow for isolation of
     concerns within each module that uses them. Kernel's replacement class
     should not rely on the heap. Use RingBuffer rather than PriorityQueue?
  4) Re-introduce the Scheduler more-or-less as it stood prior to its
     integration into ManuvrOS.

Ideally, the eventing system ought to be usable to cleanly implement either
  promise or observer patterns without reliance of those specific features in
  the C++ standard library. Lambdas are acceptable if they yield good results.
  Lambdas had worked well with ManuvrOS near the end of its evolution (see the
  BufferPipe setup in the linux demos).
  Experimentation is required on this front before the API is determined.

This arrangement ought to eventually replace the complexity in BusAdapter/BusOp.

                                           ---J. Ian Lindsay 2022.04.24 22:16:52
*/

#include "../StringBuilder.h"
#include "../CppPotpourri.h"
#include "../FlagContainer.h"
#include "../PriorityQueue.h"
#include "../ElementPool.h"
#include "../AbstractPlatform.h"
#include "../C3PLogger.h"

#ifndef __C3P_EVENTS_H
#define __C3P_EVENTS_H


/*******************************************************************************
* Fixed definitions                                                            *
*******************************************************************************/
/* Class flags for C3PEvent */
#define C3PEVENT_FLAG_RESERVED_01    0x00000001  //
#define C3PEVENT_FLAG_RESERVED_02    0x00000002  //
#define C3PEVENT_FLAG_RESERVED_04    0x00000004  //
#define C3PEVENT_FLAG_RESERVED_08    0x00000008  //

/* Class flags for C3PWorker */
#define C3PWORKER_FLAG_RESERVED_01   0x00000001  //
#define C3PWORKER_FLAG_RESERVED_02   0x00000002  //
#define C3PWORKER_FLAG_RESERVED_04   0x00000004  //
#define C3PWORKER_FLAG_RESERVED_08   0x00000008  //


class C3PEvent;
class C3PObserver;
class C3PWorker;


/*******************************************************************************
* Types                                                                         *
*******************************************************************************/
/*
* These are possible identifiers for events internal to the firmware.
* The semantics of enum should be confined to life-cycle and CPU allocation.
*/
enum class C3PEventCode : uint8_t {
  /*
  Code            Value      Semantics
  ----------------------------------------------------------------------------*/
  UNDEFINED     = 0x00,   // This is the invalid-in-use default code.
  SVC_ANNOUCE   = 0x01,   // An observer is announcing itself.
  SVC_INIT      = 0x02,   // Initialize an observer.
  SVC_POLL      = 0x03,   // Offer cycles to an observer class.
  SVC_DEINIT    = 0x04,   // Shutdown an observer.
  SVC_FAREWELL  = 0x05    // An observer is warning of its pending destruction.
};

/*
* These are possible return codes that might result from delivering an event
*   to an observer.
*/
enum class C3PEventResult : int8_t {
  /*
  Code            Value      Semantics
  ----------------------------------------------------------------------------*/
  ERROR       = -1,   // Event ignored.
  NOMINAL     =  0,   // Event noted.
  RECYCLE     =  1    // Re-poll immediately.
};

/* Callback for application-directed messages from a link. */
typedef void (*C3PEventCallback)(uint32_t now_us, C3PEvent*);


/*******************************************************************************
* Class definitions                                                            *
*******************************************************************************/


/**
* This class represents an singular Event.
*/
class C3PEvent {
  public:
    C3PEvent(C3PEventCode ID) : _id(ID), _flags(0) {};
    ~C3PEvent() {};

    C3PEventCode id() {    return _id;  };


  protected:
    C3PObserver* _source = nullptr;
    void  _print_event(StringBuilder*);


  private:
    C3PEventCode _id;
    uint8_t   _flags;

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
* This class represents an interface class for an event observer.
*/
class C3PObserver {
  protected:
    C3PWorker*  _worker;

    C3PObserver(C3PWorker* W) : _worker(W) {};
    ~C3PObserver() {};

    inline void setWorker(C3PWorker* b) {   _worker = b;       };
    inline C3PWorker* getWorker() {         return _worker;    };

    void  _print_observer(StringBuilder*);
    virtual C3PEventResult _handle_event(C3PEvent*)  =0;  // Called to handle event.
};



/**
* This class represents an asynchronous event processor.
* All classes that process asynchronous events also respond to them.
*/
class C3PWorker : public C3PObserver {
  public:
    C3PWorker() : _flags(0) {};
    ~C3PWorker() {};

    inline int8_t addObserver(C3PObserver* o) {   return _observers.insertIfAbsent(o);    };


  protected:
    void  _print_worker(StringBuilder*);
    virtual int8_t _advance_queue() =0;     //


  private:
    uint8_t   _flags;
    PriorityQueue<C3PObserver*> _observers;    // A work queue to keep transactions in order.
    PriorityQueue<C3PEvent*> _work_queue;   // A work queue to keep transactions in order.

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

#endif   // __C3P_EVENTS_H
