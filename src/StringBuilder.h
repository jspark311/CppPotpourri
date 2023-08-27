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
      consider building this with LightLinkedList.

TODO: Following the removal of zero-copy-on-const, there is no longer any reason
  to handle StrLL allocation as two seperate steps. Much simplicity will be
  gained by doing so. If zero-copy-on-const is to make a return, it will be in
  the context of a pluggable memory model, and won't belong here anyway.

TODO: Retrospective on the fragment structure...
  The reap member costs more memory (by way of alignmnet and padding) than it
  was saving in non-replication of const char* const. Under almost all usage
  patterns, and certainly the most common of them.

TODO: Direct-castablity to a string ended up being a non-value. Re-order to
  support merged allocation.

TODO: Merge the memory allocations for StrLLs, as well as their content. The
  following functions are hotspots for absurdities. Pay close attention to them
  before making a choice:
  1) concatHandoff(uint8_t*, int)
     Might be easy and safe to assume a split reap if we keep the pointer member
     and it isn't at the correct offset. A fragment created with merged
     allocation will always have a value for str that is a constant offset from
     its own.
  2) _null_term_check()
     This function does a regional re-allocation to accomodate a null-terminator
     for safety's sake. This extra byte is _not_ accounted for in the string's
     reported length. It is strictly a safety measure that the API otherwise
     ignores. This (and a few other things that are sketchy) could be solved by
     always allocating one extra byte and assuring that it is always null.

TODO: Style binge...
  1) Remove "this->" and use the same convention as elsewhere in the library for
     concealed members.
  2) Replace "unsigned char" with "uint8".
  3) Homogenize orders of test-for-equality to not risk accidental assignment.

TODO: Re-instate semaphores via the Semaphore class, rather than the ugly
  platform-specific preprocessor case-offs.
*/


#ifndef __MANUVR_DS_STRING_BUILDER_H
#define __MANUVR_DS_STRING_BUILDER_H

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


/*
*/
class StringBuilder {
  public:
    StringBuilder();
    StringBuilder(char *initial);
    StringBuilder(unsigned char *initial, int len);
    StringBuilder(const char*);
    ~StringBuilder();

    int length();
    bool isEmpty(const bool strict = true);
    uint8_t* string();
    void prepend(unsigned char *nu, int len);

    /**
    * Overrides to cleanly support C-style strings..
    */
    inline void concat(char* nu) {         concat((uint8_t*) nu, strlen(nu));   };
    inline void concat(const char* nu) {   concat((uint8_t*) nu, strlen(nu));   };
    inline void prepend(char* nu) {        prepend((uint8_t*) nu, strlen(nu));  };
    inline void prepend(const char* nu) {  prepend((uint8_t*) nu, strlen(nu));  };

    /* Variadic concat. Semantics are the same as printf. */
    int concatf(const char* format, ...);
    int concatf(const char* format, va_list);

    void concat(StringBuilder *nu);
    void concat(unsigned char *nu, int len);
    void concat(char nu);
    void concat(unsigned char nu);
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
    void concatHandoff(StringBuilder *nu);
    void concatHandoffLimit(StringBuilder *nu, unsigned int len_limit);
    void prependHandoff(StringBuilder *nu);

    /* Same idea as above, but takes a malloc'd buffer, alleviating the caller
       of responsibility for managing it. */
    void concatHandoff(uint8_t* buf, int len);

    void cull(int offset, int length);  // Use to throw away all but the specified range of this string.
    void cull(int length);              // Use to discard the first X characters from the string.
    void trim();                        // Trim whitespace off the ends of the string.
    void clear();                       // Clears the string and frees the memory that was used to hold it.
    void toUpper();                     // Convert all printable characters to the given case.
    void toLower();                     // Convert all printable characters to the given case.

    /* The functions below are meant to aid basic tokenization. They all consider the collapsed
       root string (if present) to be index zero. This detail is concealed from client classes. */
    int      chunk(int);                // Split the string into tokens by a uniform length.
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

    static void styleHeader1(StringBuilder*, const char*);
    static void styleHeader2(StringBuilder*, const char*);


  private:
    StrLL *root;         // The root of the linked-list.
    unsigned char* str;  // The collapsed string.
    int col_length;      // The length of the collapsed string.

    #if defined(__BUILD_HAS_PTHREADS)
      // If we are on linux, we control for concurrency with a mutex...
      pthread_mutex_t   _mutex;
    #elif defined(__BUILD_HAS_FREERTOS)
      SemaphoreHandle_t _mutex;
    #endif

    int    _total_str_len(StrLL*);
    StrLL* _stack_str_onto_list(StrLL* current, StrLL* nu);
    StrLL* _stack_str_onto_list(StrLL*);
    void   _null_term_check();
    StrLL* _create_str_ll(uint8_t*, int, StrLL* nxt_ll = nullptr);
    void   _destroy_str_ll(StrLL*);
    StrLL* _promote_collapsed_into_ll();
    int8_t _collapse_into_buffer();
};
#endif  // __MANUVR_DS_STRING_BUILDER_H
