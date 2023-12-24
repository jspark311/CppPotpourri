/*
File:   StringBuilder.h
Author: J. Ian Lindsay
Date:   2011.06.18

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


===< Overview >=================================================================

StringBuilder is the basis for string composition and interchange for the
  entire library. It is intended to ease the routine burdens of string
  manipulation, and uses dynamic memory internally. Some of this use is
  exposed as public members, so care needs to be taken when such features are
  used. Risks will be called-out per-function.

This class has two modes of string storage: fragmented or collapsed, which are
  mutal negations of one-another.
While fragmented, the string is stored as an ordered set of elements that are
  scattered across memory locations. These fragments ease heap loads during
  composition, make length calculation faster, and allows for arbitrary carving.
While collapsed, the entire string is stored in the str member as a single
  contiguous allocation, and has a NULL root for its linked-list. That is: it
  has no fragments. This is a minimal-overhead means of storing the string, and
  is suitable for APIs that employ the ubiquitous (uint8*, length) pattern.

Strings are stored as fragments natively, and by invocation of the tokenizer.
  When the full string is requested, the class will collapse the fragments
  into the str member, respecting the fact that part of the string may already
  have been collapsed into str, in which case, str will be prepended to the
  linked-list, and the entire linked-list then collapsed. Needless to say,
  this shuffling act might cause the class to double its memory usage while
  the string is being reorganized (in the worst case). So be aware of your
  memory usage.

===< Useful lemmata >===========================================================
1) A string that is fragmented is not collapsed, and vice-versa. (Seat of excluded middle).
2) A string that is fragmented is ipso facto not empty.
3) An empty string is also collapsed.


===< Class-wide TODOs >=========================================================

TODO: This API badly needs return codes in the memory-allocating functions. It
  cannot be safely used as unbounded heap space until that is done. As it
  stands, client classes have no means of detecting heap exhaustion, and are
  individually (and completely) responsible for their own memory safety.

TODO: It might be desirable to break StringBuilder apart along one (or more)
  lines. Namely....
  1) Memory model per-instance. Presently, this is a linked-list on the heap.
      but it might be good to have avaialble the option of assigning a
      pre-existing flat buffer space and only operating within it.
  2) Formatting and tokenizing handled as seperate pieces?
  3) Static styling methods have always felt wrong here...
  4) If we stick with a linked-list, and merged allocation doesn't interfere,
      consider building this with LightLinkedList, and doing the concurrency
      control there, instead.

TODO: Style binge...
  3) Homogenize orders of test-for-equality to not risk accidental assignment.

TODO: Re-instate semaphores via the Semaphore class, rather than the ugly
  platform-specific preprocessor case-offs.
*/


#ifndef __C3P_STRING_BUILDER_H
#define __C3P_STRING_BUILDER_H

#include <inttypes.h>
#include <stdarg.h>
#include <string.h>

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <stdio.h>
  #include <stdlib.h>
  #include <ctype.h>
#endif

// TODO: This concern belongs in the Semaphore class.
#if defined(__BUILD_HAS_PTHREADS)
  #include <pthread.h>
#elif defined(__BUILD_HAS_FREERTOS)
  #include "freertos/FreeRTOS.h"
  #include "freertos/semphr.h"
#endif

#if !defined(_GNU_SOURCE)
int strcasestr(char *a, const char *b);
#endif


/*
* This is the struct that tracks a single fragment of a string.
* It is a linked-list. For now.
*/
typedef struct str_ll_t {
  struct str_ll_t* next;  // The next element.
  int              len;   // The length of this element.
  uint8_t*         str;   // The string.
} StrLL;


/* Class for dynamic strings and buffers. */
class StringBuilder {
  public:
    /* There is a single base constructor that is shimmed by wrapper
       constructors for convenience of loading initial content. */
    StringBuilder();
    StringBuilder(char*);
    StringBuilder(const char*);
    StringBuilder(uint8_t*, int);
    StringBuilder(const uint8_t*, int);
    ~StringBuilder();

    uint8_t* string();   // Flattens the memory layout and returns its pointer.
    int length();
    bool isEmpty(const bool strict = true);
    uint8_t byteAt(const int);        // Returns the byte at the given offset.

    /* Canonical implementations of concat() and prepend(). */
    void concat(uint8_t* nu, int len);
    void prepend(uint8_t* nu, int len);

    /* Overrides to cleanly support C-style printable strings. */
    inline void concat(const uint8_t* nu, const int len) {  return concat((uint8_t*) nu, len);  };

    inline void concat(char* nu) {         concat((uint8_t*) nu, strlen(nu));   };
    inline void concat(const char* nu) {   concat((uint8_t*) nu, strlen(nu));   };
    inline void prepend(char* nu) {        prepend((uint8_t*) nu, strlen(nu));  };
    inline void prepend(const char* nu) {  prepend((uint8_t*) nu, strlen(nu));  };

    /* Variadic concat. Semantics are the same as printf. */
    int concatf(const char* format, ...);
    int concatf(const char* format, va_list);

    void concat(StringBuilder* src);
    void concat(char nu);
    void concat(uint8_t nu);
    void concat(int nu);
    void concat(unsigned int nu);
    void concat(double nu);

    inline void concat(float nu) {  concat((double) nu);    };  // Floats are upgraded to doubles.
    inline void concat(bool nu) {   concat(nu ? "T" : "F"); };

    #ifdef ARDUINO
    void concat(String);
    #endif   // ARDUINO

    /* These fxns allow for memory-tight hand-off of StrLL chains. Useful for merging
       StringBuilder instances. */
    void concatHandoff(StringBuilder* src);
    void concatHandoffLimit(StringBuilder* src, unsigned int len_limit);
    void prependHandoff(StringBuilder* src);
    void concatLimit(StringBuilder* src, unsigned int len_limit);

    /* Same idea as above, but takes a malloc'd buffer, alleviating the caller
       of responsibility for managing it. */
    void concatHandoff(uint8_t* buf, int len);

    /* Writes the len_limit number of bytes to the indicated pointer, starting
       from the optionally-defined offset. */
    int  copyToBuffer(uint8_t* buf, const uint32_t LIMIT_LEN, const uint32_t START_OFFSET = 0);

    /* Same idea as above, but also consumes the given range. */
    //int  moveToBuffer(uint8_t* buf, unsigned int len_limit, unsigned int start_offset = 0);

    void cull(int offset, int length);  // Use to throw away all but the specified range of this string.
    void cull(int length);              // Use to discard the first X characters from the string.
    void trim();                        // Trim whitespace off the ends of the string.
    void clear();                       // Clears the string and frees the memory that was used to hold it.
    void toUpper();                     // Convert all printable characters to the given case.
    void toLower();                     // Convert all printable characters to the given case.

    /* The functions below are meant to aid basic tokenization. They all consider the collapsed
       root string (if present) to be index zero. This detail is concealed from client classes. */
    int      chunk(const int);          // Split the string into tokens by a uniform length.
    int      split(const char*);        // Split the string into tokens by the given string.
    int      implode(const char*);      // Given a delimiter, form a single string from all StrLLs.
    uint16_t count();                   // Count the tokens.
    char*    position(int);             // If the string has been split, get tokens with this.
    char*    position_trimmed(int);     // Same as position(int), but trims whitespace from the return.
    int      position_as_int(int);      // Same as position(int), but uses atoi() to return an integer.
    uint64_t position_as_uint64(int);   // Same as position(int), but uses atoi() to return an integer.
    double   position_as_double(int);   // Same as position(int), but uses atof() to return a double.
    uint8_t* position(int, int*);       // ...or this, if you need the length and a binary string.
    bool     drop_position(unsigned int pos);   // And use this to reap the tokens that you've used.

    /* Comparison and search. */
    int  locate(const uint8_t*, int len, int start_offset = 0);  // Returns the offset of the given string.
    int  locate(const char*, int start_offset = 0);  // Returns the offset of the given string.
    bool contains(char);                // Does the buffer contain the given character?
    bool contains(const char*);         // Does the buffer contain the given string?
    int cmpBinString(uint8_t*, int);    // Compare byte-wise a given length.
    int replace(const char*, const char*); // Replace the former argument with the latter.
    // int cmpCaseless(const char* unknown);

    void printDebug(StringBuilder*);

    int memoryCost(bool deep = false);   // Get the memory use for this string.

    /* Statics */
    static void printBuffer(StringBuilder* output, uint8_t* buf, uint32_t len, const char* indent = "\t");
    // Wrapper for high-level string functions that we may or may not have.
    static char* strcasestr(const char* haystack, const char* needle);
    static int   strcasecmp(const char*, const char*);
    static char* trim(char*);

    static void styleHeader1(StringBuilder*, const char*);
    static void styleHeader2(StringBuilder*, const char*);


  private:
    StrLL*   _root;         // The root of the linked-list.
    #if defined(__BUILD_HAS_CONCURRENT_STRINGBUILDER)
      // TODO: Do concurrency control with a semaphore if the build requested it.
    #endif

    // TODO: Strike the platform-specific semaphore support.
    #if defined(__BUILD_HAS_PTHREADS)
      // If we are on linux, we control for concurrency with a mutex...
      pthread_mutex_t   _mutex;
    #elif defined(__BUILD_HAS_FREERTOS)
      SemaphoreHandle_t _mutex;
    #endif

    int    _total_str_len(StrLL*);
    StrLL* _get_ll_containing_offset(StrLL*, int* offset);
    int    _deepcopy_ll_bytes(StrLL* src, StrLL* dest, int length, int initial_ll_offset = 0);
    //int    _move_ll_bytes(StrLL* src, StrLL* dest, int length);
    StrLL* _stack_str_onto_list(StrLL* current, StrLL* nu);
    StrLL* _stack_str_onto_list(StrLL*);
    StrLL* _create_str_ll(int, uint8_t* buf = nullptr, StrLL* nxt_ll = nullptr);
    StrLL* _create_str_ll(int, StrLL* src, int initial_ll_offset = 0);
    void   _destroy_str_ll(StrLL*);
    int8_t _collapse();       // Flatten the string into a single allocation.
    bool   _fragged();        // Is the string fragmented?
};
#endif  // __C3P_STRING_BUILDER_H
