/*
File:   ElementPool.h
Author: J. Ian Lindsay
Date:   2017.08.13

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


Template for a preallocated pool manager, with optional overdraw support.

Constraints:
--------------------------------------------------------------------------------
1) Any type used with this pool must have a trivial (no arguments) constructor.
2) Operators new and delete is used to create and destroy overdrawn objects.
3) Apart from point (2), no operations of the objects themselves will be
    attempted. So objects returned by the pool will be in an undefined state. It
    is the responsibility of the client software to impart known states to
    objects before using them.
4) If a poll pointer was passed into the class constructor, this class will not
    attempt to allocate the pool. Otherwise, it will malloc() on first-use.
*/

#ifndef __C3P_ELEMENT_POOL_H
#define __C3P_ELEMENT_POOL_H

#include "RingBuffer.h"
#include "StringBuilder.h"
#include <new>

template <class T> class ElementPool {
  public:
    ElementPool(const uint32_t COUNT, const uint32_t OD_LIMIT = 0, T* POOL_PTR = nullptr);
    ~ElementPool();

    bool    allocated();
    bool    inPool(T*);
    int8_t  give(T*);
    T*      take();

    inline void     overdrawLimit(uint32_t x) {  _overdraw_limit = x;      };
    inline uint32_t overdrawLimit() {            return _overdraw_limit;   };
    inline uint32_t overdraws() {                return _overdraws;        };
    inline uint32_t overdrawsFreed() {           return _overdraws_freed;  };
    inline uint32_t lowWaterMark() {             return _low_watermark;    };
    inline uint32_t capacity() {                 return _list.capacity();  };
    inline uint32_t available() {                return _list.count();     };

    void printDebug(StringBuilder* output) {
      output->concatf("ElementPool (%sReady)\n", allocated() ? "" : "Not ");
      output->concatf("\tPool(%p): %u bytes\n", (uintptr_t) _pool, (_list.capacity() * sizeof(T)));
      output->concatf("\tCapacity:       %u/%u\n\tLow Watermark:  %u\n", _list.count(), _list.capacity(), _low_watermark);
      if (_overdraw_limit > 0) {
        output->concatf("\tStarves/Frees:  %u/%u\n", _overdraws, _overdraws_freed);
      }
      else {
        output->concat("\tOverdraw is disallowed.\n");
      }
    };


  private:
    RingBuffer<T*>   _list;       // The list of elements that are not in-use.
    T*               _pool;       // A pointer to the memory pool.
    uint32_t _overdraw_limit;     // Constrains heap growth against runaway overdraw.
    uint32_t _overdraws;          // How many overdraws have happened?
    uint32_t _overdraws_freed;    // How many overdraws have come back for destruction?
    uint32_t _low_watermark;      // The minimum level of our pool.
    bool     _we_own_the_pool;    // Set if this class was asked to handle pool allocation.
    bool     _list_populated;     // Set once the preallocation pool has been itemized in the list.

    inline uint32_t _overdraws_outstanding() {  return (_overdraws - _overdraws_freed);  };
};




/**
* Constructor
*/
template <class T> ElementPool<T>::ElementPool(const uint32_t COUNT, const uint32_t OD_LIMIT, T* _mem) :
  _list(COUNT), _pool(_mem),
  _overdraw_limit(OD_LIMIT), _overdraws(0), _overdraws_freed(0),
  _low_watermark(COUNT), _we_own_the_pool(false), _list_populated(false) {}


/**
* Destructor
*/
template <class T> ElementPool<T>::~ElementPool() {
  if (_we_own_the_pool & (nullptr != _pool)) {
    free(_pool);
    _pool = nullptr;
  }
}



/**
* This class follows an allocate-on-demand pattern. This function will attempt
*   pool allocation and initial population of the element list, if necessary.
*
* @return true if the pool is ready for use. False otherwise.
*/
template <class T> bool ElementPool<T>::allocated() {
  bool ret = _list.allocated();
  if (ret) {
    if (nullptr == _pool) {
      // Try to allocate the pool from the heap.
      _pool = (T*) malloc(_list.capacity() * sizeof(T));
      ret &= (nullptr != _pool);
      if (ret) {
        _we_own_the_pool = true;
      }
    }
  }

  if (ret & !_list_populated) {
    for (uint32_t i = 0; i < _list.capacity(); i++) {
      // Whatever the type is, we should initialize the memory by doing an
      //   in-place constructor call, and insert the pointer into our list.
      T* current = new (_pool + i) T();
      _list.insert(current);
    }
    _list_populated = true;
  }
  return ret;
}



/**
* Reclaims the given object so its memory can be re-used.
*
* @param T* obj is the pointer to the object to be reclaimed.
* @return 1 if the object was returned to the pool, or 0 if it was free()'d.
*/
template <class T> int8_t ElementPool<T>::give(T* e) {
  if (inPool(e)) {
    // If we are in this block, it means obj was preallocated. reclaim it.
    // Note that the object is not zeroed, or otherwise informed that it has
    //   been put on the shelf. It is the responsibility of the software using
    //   this class to take those steps, if required.
    _list.insert(e);
    return 1;
  }
  else {
    // We were created because our prealloc was starved. we are therefore a transient heap object.
    _overdraws_freed++;
    delete e;
    return 0;
  }
}


/**
* At present, our criteria for preallocation is if the pointer address passed in
*   falls within the range of our _pool array. I see nothing "non-portable"
*   about this, it doesn't require a flag or class member, and it is fast to check.
* However, this strategy only works for types that are never used in DMA or code
*   execution on the STM32F4. It may work for other architectures (PIC32, x86?).
*   I also feel like it ought to be somewhat slower than a flag or member, but not
*   by such an amount that the memory savings are not worth the CPU trade-off.
* Consider writing all new cyclical queues with preallocated members to use this
*   strategy. Also, consider converting the most time-critical types to this strategy
*   up until we hit the boundaries of the STM32 CCM.
*                                 ---J. Ian Lindsay   Mon Apr 13 10:51:54 MST 2015
*
* @return true if the given element is part of this preallocation pool.
*/
template <class T> bool ElementPool<T>::inPool(T* e) {
  const uintptr_t obj_addr = ((uintptr_t) e);
  const uintptr_t pre_min  = ((uintptr_t) _pool);
  const uintptr_t pre_max  = pre_min + (uintptr_t) (_list.capacity() * sizeof(T));
  return ((obj_addr < pre_max) & (obj_addr >= pre_min));
}


/**
* Remove an item from the preallocation pool an return it.
*
* @return a reference to the preallocated element.
*/
template <class T> T* ElementPool<T>::take() {
  T* ret = _list.get();
  if (nullptr == ret) {
    ret = new T();
    _overdraws++;
  }
  if (_low_watermark > _list.count()) {
    _low_watermark = _list.count();
  }
  return ret;
}

#endif // __C3P_ELEMENT_POOL_H
