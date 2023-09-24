/*
File:   RingBuffer.h
Author: J. Ian Lindsay
Date:   2014.04.28

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


Template for a ring buffer.

TODO: Rework modulus operations into bit mask, and make element count pow(2).
TODO: Audit for best-practices for a lock-free design.

NOTE: RingBuffer will not allow excursions past its declared buffer limit. In
  the event that it is requested, it will take all that it can, and report how
  much it actually took.
*/

#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>

#ifndef __DS_RING_BUFFER_H
#define __DS_RING_BUFFER_H

template <class T> class RingBuffer {
  public:
    /*
    * Constructor takes the number of slots as its sole argument.
    */
    RingBuffer(const unsigned int c) :
      _CAPAC(c), _E_SIZE(sizeof(T)),
      _count(0), _w(0), _r(0), _pool(nullptr) {};
    ~RingBuffer();

    bool allocated();
    bool contains(T);
    void clear();                   // Wipe the buffer.
    int  cull(const unsigned int);  // Discard so many elements.

    inline unsigned int capacity() {   return _CAPAC;              };
    inline unsigned int heap_use() {   return (_E_SIZE * _CAPAC);  };
    inline unsigned int vacancy() {    return (_CAPAC - _count);   };

    /* Returns an integer representing how many items are buffered. */
    inline unsigned int count() {      return _count;              };
    inline bool         isEmpty() {    return (0 == count());      };

    int  insert(T);           // Insert an element.
    int  insert(T*, unsigned int len);   // Insert many elements.
    inline int  insertIfAbsent(T x) {  return (contains(x) ? -2 : insert(x));  };

    inline T get() {         return _get(true);        };
    inline T peek() {        return _get(false);       };
    int  get(T*, unsigned int len);   // Get many elements.
    T    peek(unsigned int idx, bool absolute_index = false);
    int  peek(T*, unsigned int len);  // Get many elements.


  private:
    const unsigned int _CAPAC;
    const unsigned int _E_SIZE;
    unsigned int _count;
    unsigned int _w;
    unsigned int _r;
    uint8_t* _pool;

    T _get(bool also_remove);
};


/*
* Destructor
*/
template <class T> RingBuffer<T>::~RingBuffer() {
  _count = 0;
  free(_pool);
  _pool = nullptr;
}

template <class T> bool RingBuffer<T>::allocated() {
  if (nullptr == _pool) {
    const unsigned int s = _E_SIZE * _CAPAC;
    _pool = (uint8_t*) malloc(s);
    clear();
    return (nullptr != _pool);
  }
  return true;
}


/**
* Drop all items from the buffer. Zeros all memory, if it is allocated.
*/
template <class T> void RingBuffer<T>::clear() {
  _w = 0;
  _r = 0;
  _count = 0;
  if (allocated()) {
    for (unsigned int i = 0; i < (_E_SIZE * _CAPAC); i++) {
      // TODO: We were almost certainly allocated on an alignment we
      // can write longwords over...
      *((uint8_t*) _pool + i) = 0x00;
    }
  }
}


/**
* Drop a certain number of items from the buffer.
* TODO: Does not zero the affected memory.
*
* @param CULL_COUNT the number of elements to drop.
* @returns the number of elements dropped.
*/
template <class T> int RingBuffer<T>::cull(const unsigned int CULL_COUNT) {
  int ret = 0;
  if (allocated()) {
    const int SAFE_CULL_COUNT = strict_min(CULL_COUNT, _count);
    _r = ((_r + SAFE_CULL_COUNT) % _CAPAC);
    _count -= SAFE_CULL_COUNT;
    ret = SAFE_CULL_COUNT;
  }
  return ret;
}


/**
* This template makes copies of whatever is passed into it. There is no reason
*   for the caller to maintain local copies of data (unless T is a pointer).
*
* @return 0 on success, or negative on error.
*/
template <class T> int RingBuffer<T>::insert(T d) {
  if (!allocated() || (_count >= _CAPAC)) {
    return -1;
  }
  T* ref = &d;

  unsigned int offset = _E_SIZE * _w;
  for (unsigned int i = 0; i < _E_SIZE; i++) {
    *((uint8_t*) _pool + offset + i) = *((uint8_t*)ref + i);
  }
  _w = ((_w + 1) % _CAPAC);   // TODO: Convert to pow(2) later and convert to bitmask.
  _count++;
  return 0;
}


/**
* This template makes copies of whatever is passed into it. There is no reason
*   for the caller to maintain local copies of data (unless T is a pointer).
* NOTE: In the event that the number of additional elements in the request
*   exceeds the avaiable capacity of the buffer, this function will take as many
*   elements as it can from the provided buffer. It is the caller's
*   responsibility to know how to handle partial takes.
*
* @return The number of elements taken on success, or negative on error.
*/
template <class T> int RingBuffer<T>::insert(T* d_ptr, unsigned int added_elements) {
  if ((nullptr == d_ptr) | (0 >= added_elements)) {  return -1;   }  // Parameter sanity.
  if (!allocated() || (_count >= _CAPAC)) {          return -1;   }  // Allocation and capacity.

  const int32_t COUNT_TO_TAKE = strict_min(added_elements, vacancy());
  int count_taken = 0;
  // TODO: It would be marginally more efficient to copy the inner-loop into
  //   this function, and re-write it to handle chunks of multiples of _E_SIZE
  //   up to the boundaries of the ring. That way, an insertion of any number
  //   of elements would make two calls to memcpy(), at most. And no outer loop.
  //   Until someone cares, we keep with an outer loop (the one below), and wrap
  //   our byte-wise function.
  for (int32_t i = 0; i < COUNT_TO_TAKE; i++) {
    if (0 != insert(*(d_ptr + i))) {   break;   }
    else {    count_taken++;    }
  }
  return count_taken;
}


/**
* Cycles through the ring and looks for any instances of the argument.
*
* @return 0 on success, or negative on error.
*/
template <class T> bool RingBuffer<T>::contains(T d) {
  bool found = false;
  if (allocated() && (0 < _count)) {
    unsigned int cur_idx = _r;
    uint8_t* compare = (uint8_t*) &d;
    uint32_t elements_tested = 0;

    while (!found & (elements_tested < _count)) {
      uint8_t* current = (uint8_t*) (_pool + (cur_idx * _E_SIZE));
      bool current_is_compare = true;
      for (uint32_t i = 0; i < _E_SIZE; i++) {
        current_is_compare &= (*(current + i) == *(compare + i));
      }
      found |= current_is_compare;

      cur_idx = ((cur_idx + 1) % _CAPAC);   // TODO: Convert to pow(2) later and convert to bitmask.
      elements_tested++;
    }
  }
  return found;
}


/**
* Remove and return the head of the ring.
*
* @return T(0) on failure, or the data at the front of the ring.
*/
template <class T> T RingBuffer<T>::_get(bool also_remove) {
  if (!allocated() || (0 == _count)) {
    return (T)0;
  }
  T* return_value = (T*) (_pool + (_r * _E_SIZE));
  if (also_remove) {
    _r = ((_r + 1) % _CAPAC);   // TODO: Convert to pow(2) later and convert to bitmask.
    _count--;
  }
  return *return_value;
}


/**
* Peek at a specific index in the ring without changing anything.
*
* @param idx The position in the ring.
* @param absolute_index Index is from memory offset 0 if true, or the next in line otherwise.
* @return T(0) on failure, or the data at the given index.
*/
template <class T> T RingBuffer<T>::peek(unsigned int idx, bool absolute_index) {
  if (!allocated() || (0 == _count) || (idx > _CAPAC)) {
    return (T)0;
  }
  T* return_value = (T*) (_pool + ((((absolute_index ? 0 : _r) + idx) % _CAPAC) * _E_SIZE));
  return *return_value;
}


/**
* Take up to a specified number of items from the buffer.
*
* @param buf is the buffer to receive the elements.
* @param len is the maximum number of elements to be transfered.
* @return The number of elements taken on success, or negative on error.
*/
template <class T> int RingBuffer<T>::get(T* buf, unsigned int len) {
  if (!allocated() || (0 == len) || (nullptr == buf)) {
    return -1;
  }
  const uint32_t XFER_LEN = strict_min((uint32_t) _count, (uint32_t) len);
  for (uint32_t i = 0; i < XFER_LEN; i++) {
    *(buf + i) = get();
  }
  return (int) XFER_LEN;
}


/**
* Peek at a specified number of items from the buffer.
*
* @param buf is the buffer to receive the elements.
* @param len is the maximum number of elements to be transfered.
* @return The number of elements coped on success, or negative on error.
*/
template <class T> int RingBuffer<T>::peek(T* buf, unsigned int len) {
  if (!allocated() || (0 == len) || (nullptr == buf)) {
    return -1;
  }
  const uint32_t XFER_LEN = strict_min((uint32_t) _count, (uint32_t) len);
  for (uint32_t i = 0; i < XFER_LEN; i++) {
    *(buf + i) = peek(i);
  }
  return (int) XFER_LEN;
}

#endif // __DS_RING_BUFFER_H
