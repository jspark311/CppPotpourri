/*
File:   C3PStack.h
Author: J. Ian Lindsay (LLM assisted)
Date:   2025.08.28

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


Template for a stack.
*/

#ifndef __DS_C3P_STACK_H
#define __DS_C3P_STACK_H

#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include "CppPotpourri.h"


/**
 * A simple LIFO stack template for trivially copyable types.
 * Zero-copy and C-style allocation to match RingBuffer idioms.
 */

template <class T>
class C3PStack {
  public:
    /** Constructor: reserve capacity for c elements */
    C3PStack(const unsigned int c)
      : _CAPAC(c), _E_SIZE(sizeof(T)), _count(0), _pool(nullptr) {}
    ~C3PStack();

    bool allocated();                // allocate backing store if needed
    bool contains(T d);             // search for a value
    void clear();                   // wipe all elements (and zero memory)

    inline unsigned int capacity() { return _CAPAC; }
    inline unsigned int count()    { return _count; }
    inline bool         isEmpty()  { return (0 == _count); }
    inline unsigned int vacancy()  { return (_CAPAC - _count); }

    int  push(T d);                 // push a single element
    int  push(T* d_ptr, unsigned int len); // push multiple elements
    inline int pushIfAbsent(T x) { return (contains(x) ? -2 : push(x)); }

    T    pop();                     // pop top element
    int  pop(T* buf, unsigned int len); // pop multiple into buffer

    T    peek(unsigned int idx);           // peek idx-from-top
    int  peek(T* buf, unsigned int len);   // peek multiple

  private:
    const unsigned int _CAPAC;    // maximum number of elements
    const unsigned int _E_SIZE;   // sizeof(T)
    unsigned int       _count;    // current element count
    uint8_t*           _pool;     // raw byte storage
};


// Destructor
template <class T> C3PStack<T>::~C3PStack() {
    _count = 0;
    if (nullptr != _pool) free(_pool);
    _pool = nullptr;
}


// Allocate and zero memory
template <class T> bool C3PStack<T>::allocated() {
    if (nullptr == _pool) {
        const unsigned int s = _CAPAC * _E_SIZE;
        _pool = (uint8_t*)malloc(s);
        if (nullptr == _pool) return false;
        memset(_pool, 0, s);
    }
    return true;
}

// Linear search for a value
template <class T> bool C3PStack<T>::contains(T d) {
    bool found = false;
    if (allocated() && (_count > 0)) {
        uint8_t* compare = (uint8_t*)&d;
        for (unsigned int idx = 0; idx < _count; idx++) {
            uint8_t* current = _pool + (idx * _E_SIZE);
            bool eq = true;
            for (unsigned int i = 0; i < _E_SIZE; i++) {
                eq &= (*(current + i) == *(compare + i));
            }
            if (eq) { found = true; break; }
        }
    }
    return found;
}


// Reset stack and zero memory
template <class T> void C3PStack<T>::clear() {
    _count = 0;
    if (allocated()) {
        memset(_pool, 0, _CAPAC * _E_SIZE);
    }
}


// Push one element
template <class T> int C3PStack<T>::push(T d) {
    if (!allocated() || (_count >= _CAPAC)) return -1;
    uint8_t* dest = _pool + (_count * _E_SIZE);
    memcpy(dest, &d, _E_SIZE);
    _count++;
    return 0;
}


// Push multiple elements from array
template <class T> int C3PStack<T>::push(T* d_ptr, unsigned int len) {
    if ((nullptr == d_ptr) || (0 == len)) return -1;
    if (!allocated() || (_count >= _CAPAC)) return -1;
    uint32_t to_take = strict_min((uint32_t)len, (uint32_t)vacancy());
    int taken = 0;
    for (uint32_t i = 0; i < to_take; i++) {
        if (0 != push(*(d_ptr + i))) break;
        taken++;
    }
    return taken;
}


// Pop top element
template <class T> T C3PStack<T>::pop() {
    if (!allocated() || (_count == 0)) return (T)0;
    uint8_t* src = _pool + ((_count - 1) * _E_SIZE);
    T val;
    memcpy(&val, src, _E_SIZE);
    _count--;
    return val;
}


// Pop multiple into buffer
template <class T> int C3PStack<T>::pop(T* buf, unsigned int len) {
    if (!allocated() || (0 == len) || (nullptr == buf)) return -1;
    uint32_t to_take = strict_min((uint32_t)_count, (uint32_t)len);
    for (uint32_t i = 0; i < to_take; i++) {
        *(buf + i) = pop();
    }
    return to_take;
}


// Peek at an element idx from top (0 = top)

template <class T> T C3PStack<T>::peek(unsigned int idx) {
    if (!allocated() || (_count == 0) || (idx >= _count)) return (T)0;
    uint8_t* src = _pool + ((_count - 1 - idx) * _E_SIZE);
    T val;
    memcpy(&val, src, _E_SIZE);
    return val;
}


// Peek multiple into buffer
template <class T> int C3PStack<T>::peek(T* buf, unsigned int len) {
    if (!allocated() || (0 == len) || (nullptr == buf)) return -1;
    uint32_t to_take = strict_min((uint32_t)_count, (uint32_t)len);
    for (uint32_t i = 0; i < to_take; i++) {
        *(buf + i) = peek(i);
    }
    return to_take;
}

#endif // __DS_C3P_STACK_H
