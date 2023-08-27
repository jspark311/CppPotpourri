/*
File:   StringBuilder.cpp
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
*/

#include "StringBuilder.h"
#include "CppPotpourri.h"
#include <alloca.h>    // TODO: Deprecate over time.


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

/*
* Locate a substring within a string (case insensitive).
*/
char* StringBuilder::strcasestr(const char* haystack, const char* needle) {
  if ((nullptr != haystack) && (nullptr != needle) && (0 != *needle)) {
    char* H_ptr = (char*) haystack;
    char* N_ptr = (char*) needle;
    bool there_is_still_hope = true;

    while (there_is_still_hope) {
      there_is_still_hope = false;

      // Scan the haystack until the first character of the needle is found.
      const char NEEDLE_FIRST_CHAR = toupper(*N_ptr);
      while ((*H_ptr) && (toupper(*H_ptr) != NEEDLE_FIRST_CHAR)) {  H_ptr++;   }

      if (0 != *H_ptr) {
        // Nominal case. Character match. Continue iteration on N_ptr.
        int hs_idx = 0;
        while ((*(H_ptr + hs_idx) && *N_ptr) && (toupper(*(H_ptr + hs_idx)) == toupper(*N_ptr))) {
          N_ptr++;
          hs_idx++;
        }
        if (0 == *N_ptr) { // Search succeeded.
          return H_ptr;    // Bailout with success.
        }
        H_ptr++;
        there_is_still_hope = true;
        N_ptr = (char*) needle;  // Reset to the first char of needle.
      }
    }
  }
  return nullptr;
}


/*
* We might choose to roll-our-own so that we don't bring in enormous dependencies.
*
* Taken from
* http://c-for-dummies.com/blog/?p=1359
*/
int StringBuilder::strcasecmp(const char *a, const char *b) {
  if ((nullptr != a) && (nullptr != b)) {
    char c = 0;
    while (*a && *b) {
      c = toupper(*a) - toupper(*b);
      if (c != 0) {
        return c;
      }
      a++;
      b++;
    }
    c = *a - *b;  // If the strings are equal, these will both be null-terms.
    return c;
  }
  return -1;
}


/**
* Static utility function for dumping buffers for humans to read.
*
* @param output is the StringBuilder* that should receive this function's output
* @param buf contains the buffer we wish to print
* @param len is how many bytes to print
* @param indent is a string to prepend to each rendered line.
*/
void StringBuilder::printBuffer(StringBuilder* output, uint8_t* buf, uint32_t len, const char* indent) {
  if ((nullptr != buf) & (len > 0)) {
    uint32_t i = 0;
    while (len >= 16) {
      output->concatf("%s0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        indent, i,
        *(buf + i +  0), *(buf + i +  1), *(buf + i +  2), *(buf + i +  3),
        *(buf + i +  4), *(buf + i +  5), *(buf + i +  6), *(buf + i +  7),
        *(buf + i +  8), *(buf + i +  9), *(buf + i + 10), *(buf + i + 11),
        *(buf + i + 12), *(buf + i + 13), *(buf + i + 14), *(buf + i + 15)
      );
      i += 16;
      len -= 16;
    }
    if (len > 0) {
      output->concatf("%s0x%04x: ", indent, i);
      while (len > 0) {
        output->concatf("%02x ", *(buf + i));
        i++;
        len--;
      }
      output->concat("\n");
    }
  }
  else {
    output->concat(indent);
    output->concat("(NULL BUFFER)\n");
  }
}


/*******************************************************************************
* Static string styling
*******************************************************************************/

// TODO: Both build sizes and consistency of output get a boost from central
//   styling functions of this sort. For now, they are kept too-simple, and as
//   statics in StringBuilder. It was a tremendous improvement over having this
//   scattered in every printDebug() function that wanted a header.
// It would be nice to have a central string styling class (or a prototype for)
//   one so that complexity can be moved from everywhere into a single place
//   which can have features like soft word-wrap, columnar output, color markup,
//   and so on. If ever such a class is created, these should be migrated to it.

/**
* Static utility function for uniformly formatting output strings.
*
* @param output is the StringBuilder* that should receive this function's output
* @param text contains the content of the header
*/
void StringBuilder::styleHeader1(StringBuilder* output, const char* text) {
  output->concatf("--- %s\n---------------------------------------\n", text);
}

/**
* Static utility function for uniformly formatting output strings.
*
* @param output is the StringBuilder* that should receive this function's output
* @param text contains the content of the header
*/
void StringBuilder::styleHeader2(StringBuilder* output, const char* text) {
  output->concatf("===< %s >=================================================\n", text);
}



/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* Vanilla constructor.
*/
StringBuilder::StringBuilder() : root(nullptr), str(nullptr), col_length(0) {
  #if defined(__BUILD_HAS_PTHREADS)
    #if defined (PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP)
    _mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
    #else
    _mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
    #endif
  #elif defined(__BUILD_HAS_FREERTOS)
    //_mutex = xSemaphoreCreateRecursiveMutex();
  #endif
}

StringBuilder::StringBuilder(char* initial) : StringBuilder() {
  this->concat(initial);
}

StringBuilder::StringBuilder(uint8_t* initial, int len) : StringBuilder() {
  this->concat(initial, len);
}


StringBuilder::StringBuilder(const char* initial) : StringBuilder() {
  this->concat(initial);
}


/**
* Destructor.
*/
StringBuilder::~StringBuilder() {
  if (this->root != nullptr) _destroy_str_ll(this->root);

  if (this->str != nullptr) {
    free(this->str);
    this->str = nullptr;
  }
  #if defined(__BUILD_HAS_PTHREADS)
    pthread_mutex_destroy(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
    //vSemaphoreDelete(&_mutex);
  #endif
}


/*******************************************************************************
*******************************************************************************/

/**
* Public fxn to get the length of the string represented by this class. This is
*   called before allocating mem to flatten the string into a single string.
*
* @return The total length of the string.
*/
int StringBuilder::length() {
  int return_value = this->col_length;
  if (this->root != nullptr) {
    return_value  = return_value + this->_total_str_len(this->root);
  }
  return return_value;
}

/**
* Public fxn to quickly decide if a String is empty. This is significantly
*   faster than testing the actual length.
* A string is considered empty if either...
*   a) The total length is zero.
*   b) The total length is one, but the only byte is a null-terminator and checking is not strict.
*
* @param strict will consider the presence of a null-terminator to be "not-empty".
* @return True if this StringBuilder is empty (or zero-length).
*/
bool StringBuilder::isEmpty(const bool strict) {
  const int TLEN = this->col_length + ((nullptr == this->root) ? 0 : root->len);
  if (strict) {
    return (0 == TLEN);
  }
  switch (TLEN) {
    case 0:   return true;  // No length means no string.
    case 1:
      // An allocated string with only a null-terminator is considered empty if
      //   there are fewer than 2 bytes in the StringBuilder.
      // That single byte either came from the collapsed string or the list.
      return (0 == col_length) ? (*str != 0) : (*(root->str) != 0);
    default:  return false;  // Many bytes means "not-empty" in all cases.
  }
}



/**
* How many discrete elements are in this object?
* Includes the base str member in the count.
*
* @return The number of linked-lists that are being used to hold this string
*/
unsigned short StringBuilder::count() {
  unsigned short return_value = (nullptr != this->str) ? 1 : 0;
  StrLL *current = this->root;
  while (current != nullptr) {
    return_value++;
    current = current->next;
  }
  return return_value;
}


/**
* Public fxn to retrieve the flattened string as an uint8_t*.
* Will never return nullptr. Even for an empty string. Will return a
*   null-terminated zero-length string in the worst-case.
* The caller must not try to free() the value returned.
*
* @return A pointer to the content of the StringBuilder, made contiguous.
*/
unsigned char* StringBuilder::string() {
  if ((this->str == nullptr) && (this->root == nullptr)) {
    // Nothing in this object. Return a zero-length string.
    // this->str = (uint8_t*) malloc(1);
    // this->str[0] = '\0';
    // this->col_length  = 0;
    // NOTE: We would previously allocate an empty string to satisfy the caller,
    //   but this causes undue heap thrash, and serves no real purpose.
    // Besides... If the caller is going to take the risk of direct value
    //   manipulation of the memory at the location returned by this function,
    //   then that code needs to at least have enough sense to check a length
    //   first. So the fib we are telling by casting in the manner below should
    //   not expose the user of this class to any illegal memory accesses
    //   (writing to read-only mem) that their bounds-checking wouldn't already
    //   prevent.            --- J. Ian Lindsay Sat 24 Sep 2022 01:03:08 AM MDT
    return (uint8_t*) "";
  }
  else {
    this->_collapse_into_buffer();
    return this->str;
  }
}


/**
* Wipes the StringBuilder, free'ing memory as appropriate.
*/
void StringBuilder::clear() {
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  if (this->root != nullptr) _destroy_str_ll(this->root);
  if (this->str != nullptr) {
    free(this->str);
  }
  this->root   = nullptr;
  this->str    = nullptr;
  this->col_length = 0;
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
}


/**
* Convert all printable characters to uppercase. Use only on ASCII strings.
*/
void StringBuilder::toUpper() {
  for (int i = 0; i < col_length; i++) {
    *(str + i) = toupper(*(str + i));  // Process the collapsed string (if any).
  }
  StrLL *current = this->root;
  while (current != nullptr) {   // Process the fragments (if any).
    for (int i = 0; i < current->len; i++) {
      *(current->str + i) = toupper(*(current->str + i));
    }
    current = current->next;
  }
}


/**
* Convert all printable characters to lowercase. Use only on ASCII strings.
*/
void StringBuilder::toLower() {
  for (int i = 0; i < col_length; i++) {
    *(str + i) = tolower(*(str + i));  // Process the collapsed string (if any).
  }
  StrLL *current = this->root;
  while (current != nullptr) {   // Process the fragments (if any).
    for (int i = 0; i < current->len; i++) {
      *(current->str + i) = tolower(*(current->str + i));
    }
    current = current->next;
  }
}


/**
* Return a castable pointer for the string at position <pos>.
*
* @param pos is the index of the desired token.
* @return A pointer to the requested token, or nullptr on failure.
*/
char* StringBuilder::position(int pos) {
  if (this->str != nullptr) {
    if (pos == 0) {
      return (char*) this->str;
    }
    pos--;
  }
  StrLL *current = this->root;
  int i = 0;
  while ((i != pos) & (current != nullptr)){
    current = current->next;
    i++;
  }
  return (current == nullptr) ? nullptr : ((char *)current->str);
}


/**
* Convenience fxn for accessing a token as an int.
*
* @param pos is the index of the desired token.
* @return atoi()'s attempt at parsing the string at the given pos as an int.
*/
int StringBuilder::position_as_int(int pos) {
  const char* temp = (const char*) position(pos);
  if (temp != nullptr) {
    int len = strlen(temp);
    if ((len > 2) && (*(temp) == '0') && (*(temp + 1) == 'x')) {
      // Possibly dealing with a hex representation. Try to convert....
      uint32_t result = 0;
      len -= 2;
      temp += 2;
      // Only so big an int we can handle...
      int max_bytes = (len > 8) ? 8 : len;
      for (int i = 0; i < max_bytes; i++) {
        switch (*(temp + i)) {
          case '0':  case '1':  case '2':  case '3':  case '4':
          case '5':  case '6':  case '7':  case '8':  case '9':
            result = (result << 4) + (*(temp + i) - 0x30);
            break;
          case 'a':  case 'b':  case 'c':  case 'd':
          case 'e':  case 'f':
            result = (result << 4) + 10 + (*(temp + i) - 0x61);
            break;
          case 'A':  case 'B':  case 'C':  case 'D':
          case 'E':  case 'F':
            result = (result << 4) + 10 + (*(temp + i) - 0x41);
            break;
          default: return result;
        }
      }
      return result;
    }
    else {
      return atoi(temp);
    }
  }
  return 0;
}


/**
* Convenience fxn for accessing a token as an int.
*
* @param pos is the index of the desired token.
* @return atoi()'s attempt at parsing the string at the given pos as an int.
*/
uint64_t StringBuilder::position_as_uint64(int pos) {
  const char* temp = (const char*) position(pos);
  if (temp != nullptr) {
    int len = strlen(temp);
    if ((len > 2) && (*(temp) == '0') && (*(temp + 1) == 'x')) {
      // Possibly dealing with a hex representation. Try to convert....
      uint64_t result = 0;
      len -= 2;
      temp += 2;
      // Only so big an int we can handle...
      int max_bytes = (len > 16) ? 8 : len;
      for (int i = 0; i < max_bytes; i++) {
        switch (*(temp + i)) {
          case '0':  case '1':  case '2':  case '3':  case '4':
          case '5':  case '6':  case '7':  case '8':  case '9':
            result = (result << 4) + (*(temp + i) - 0x30);
            break;
          case 'a':  case 'b':  case 'c':  case 'd':
          case 'e':  case 'f':
            result = (result << 4) + 10 + (*(temp + i) - 0x61);
            break;
          case 'A':  case 'B':  case 'C':  case 'D':
          case 'E':  case 'F':
            result = (result << 4) + 10 + (*(temp + i) - 0x41);
            break;
          default: return result;
        }
      }
      return result;
    }
    else {
      char* end = nullptr;
      return strtoull(temp, &end, 10);
    }
  }
  return 0;
}


/**
* Convenience fxn for accessing a token as a double.
*
* @param pos is the index of the desired token.
* @return atof()'s attempt at parsing the string at the given pos as a double.
*/
double StringBuilder::position_as_double(int pos) {
  const char* temp = (const char*) position(pos);
  if (temp != nullptr) {
    return atof(temp);
  }
  return 0.0;
}


/**
* Return a castable pointer for the string at position <pos>.
* Null on failure.
*/
unsigned char* StringBuilder::position(int pos, int *pos_len) {
  if (this->str != nullptr) {
    if (pos == 0) {
      *pos_len = this->col_length;
      return this->str;
    }
    pos--;
  }
  StrLL *current = this->root;
  int i = 0;
  while ((i != pos) && (current != nullptr)){
    current = current->next;
    i++;
  }
  *pos_len = (current != nullptr) ? current->len : 0;
  return ((current != nullptr) ? current->str : (uint8_t*)"");
}


char* StringBuilder::position_trimmed(int pos){
  char* str = position(pos);
  if (str == nullptr) {
    return (char*) "";
  }
  char *end;
  while(isspace(*str)) str++;
  if(*str == 0) return str;
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;
  *(end+1) = 0;
  return str;
}


/**
* Returns true on success, false on failure.
*/
bool StringBuilder::drop_position(unsigned int pos) {
  if (this->str != nullptr) {
    if (pos == 0) {
      this->col_length = 0;
      free(this->str);
      this->str = nullptr;
      return true;
    }
    pos--;
  }
  StrLL *current = this->root;
  StrLL *prior = nullptr;
  unsigned int i = 0;
  while ((i != pos) && (current != nullptr)){
    prior = current;
    current = current->next;
    i++;
  }
  if (current != nullptr) {
    if (prior == nullptr) {
      this->root = current->next;
      current->next = nullptr;
      _destroy_str_ll(current);
    }
    else {
      prior->next = current->next;
      current->next = nullptr;
      _destroy_str_ll(current);
    }
    return true;
  }
  return false;
}


/*******************************************************************************
* Functions that change the data represeted by this object.
*******************************************************************************/

/**
* This fxn is meant to hand off accumulated string data to us from the StringBuilder
*   that is passed in as the parameter. This is very fast, but doesn't actually copy
*   any data. So to eliminate the chance of dangling pointers and other
*   difficult-to-trace bugs, we modify the donor SB to prevent its destruction from
*   taking the data we now have with it.
*/
void StringBuilder::concatHandoff(StringBuilder* nu) {
  #if defined(__BUILD_HAS_PTHREADS)
    pthread_mutex_lock(&_mutex);
    pthread_mutex_lock(&nu->_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
    //xSemaphoreTakeRecursive(&_mutex, 0);
    //xSemaphoreTakeRecursive(&nu->_mutex, 0);
  #endif
  if ((nullptr != nu) && (!nu->isEmpty(true))) {
    nu->_promote_collapsed_into_ll();   // Promote the previously-collapsed string.

    if (nullptr != nu->root) {
      this->_stack_str_onto_list(nu->root);
      nu->root = nullptr;  // Inform the origin instance...
    }
  }
  #if defined(__BUILD_HAS_PTHREADS)
    pthread_mutex_unlock(&nu->_mutex);
    pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
    //xSemaphoreGiveRecursive(&nu->_mutex);
    //xSemaphoreGiveRecursive(&_mutex);
  #endif
}


/**
* Like concatHandoff(StringBuilder*), but bounded. Will only transfer the given
*   number of bytes, taking full chunks if possible.
*/
void StringBuilder::concatHandoffLimit(StringBuilder* nu, unsigned int len_limit) {
  #if defined(__BUILD_HAS_PTHREADS)
    pthread_mutex_lock(&_mutex);
    pthread_mutex_lock(&nu->_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
    //xSemaphoreTakeRecursive(&_mutex, 0);
    //xSemaphoreTakeRecursive(&nu->_mutex, 0);
  #endif
  if ((nullptr != nu) && (!nu->isEmpty(true))) {
    nu->_promote_collapsed_into_ll();   // Promote any previously-collapsed string.

    unsigned int buffer_taken  = 0;
    StrLL*   current       = nu->root;
    StrLL*   first_overlen = nu->root;
    uint32_t snip_length   = 0;
    int      ll_snip_count = 0;
    // Seek through the input and find the point where we need to snip.
    while ((nullptr != current) && ((snip_length + current->len) <= len_limit)) {
      ll_snip_count++;
      snip_length += current->len;
      current = current->next;
      first_overlen = current->next;
    }
    // If we can simply move memory references around, do so. All leading StrLL
    //   that fits within the limit is taken directly.
    if (0 < snip_length) {
      StrLL* old_root = nu->root;
      nu->root = first_overlen;   // Shortens the length of nu.
      current->next = nullptr;    // Break the links in the items we are taking.
      this->_stack_str_onto_list(old_root);  // Append the assumed data.
      buffer_taken += snip_length;           // Note the bytes taken.
    }

    if (buffer_taken < len_limit) {
      // We've taken all of the whole allocations that we could, but the limit
      //   doesn't fall on a convenient boundary. So we'll have to actually
      //   allocate and shuffle at this point.
      // For this object, we will create a new StrLL and copy the difference of
      //   bytes into it. But for the source object, we will just shift all the
      //   bytes forward that fell outside of the limit, and change the size.
      //   The memory will be reclaimed on the next string consolidation.
      const int32_t LEN_DIFFERENCE    = (len_limit - buffer_taken);
      const int32_t LEN_OUT_OF_BOUNDS = (first_overlen->len - LEN_DIFFERENCE);
      concat(first_overlen->str, LEN_DIFFERENCE);
      for (int i = 0; i < LEN_OUT_OF_BOUNDS; i++) {
        *(first_overlen->str + i) = *(first_overlen->str + i + LEN_DIFFERENCE);
      }
      first_overlen->len = LEN_OUT_OF_BOUNDS;
      buffer_taken += LEN_DIFFERENCE;
    }
  }
  #if defined(__BUILD_HAS_PTHREADS)
    pthread_mutex_unlock(&nu->_mutex);
    pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
    //xSemaphoreGiveRecursive(&nu->_mutex);
    //xSemaphoreGiveRecursive(&_mutex);
  #endif
}


/*
* Thank you N. Gortari for the most excellent tip:
*   Intention:       if (obj == nullptr)  // Null-checking is a common thing to do...
*   The risk:        if (obj = nullptr)   // Compiler allows this. Assignment always evals to 'true'.
*   The mitigation:  if (nullptr == obj)  // Equality is commutitive.
*
*     "(nullptr == obj)" and "(obj == nullptr)" mean exaclty the same thing, and are equally valid.
*
* Levarge assignment operator's non-commutivity, so if you derp and do this...
*           if (nullptr = obj)
* ...the mechanics of the language will prevent compilation, and thus, not allow you
*       to overlook it on accident.
*/
void StringBuilder::prependHandoff(StringBuilder* nu) {
  if (nullptr != nu) {
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_lock(&_mutex);
      //pthread_mutex_lock(&nu->_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
    this->root = _promote_collapsed_into_ll();   // Promote the previously-collapsed string.

    // Promote the donor instance's previously-collapsed string so we don't have to worry about it.
    StrLL *current = nu->_promote_collapsed_into_ll();

    if (nullptr != nu->root) {
      // Scan to the end of the donated LL...
      while (nullptr != current->next) {   current = current->next;  }

      current->next = this->root;  // ...and tack our existing list to the end of it.
      this->root = nu->root;       // ...replace our idea of the root.
      nu->root = nullptr;             // Inform the origin instance so it doesn't free what we just took.
    }
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_unlock(&nu->_mutex);
      //pthread_mutex_unlock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
  }
}


/*
* This function must only be called with a malloc()'d buffer, and never one that
*   is on-stack, or in RO memory. By calling this function, the caller is
*   delegating responsibility to free() a (probably large) buffer.
* For safety's sake, the last byte should be a '\0'.
*
* TODO: Merged-allocation will force the re-write or removal of this function.
*   This function in among those that must be carefully considered when
*   before reworking the memory model.
*/
void StringBuilder::concatHandoff(uint8_t* buf, int len) {
  if ((buf) && (len > 0)) {
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_lock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
    StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
    if (nu_element) {
      //nu_element->reap = true;
      nu_element->next = nullptr;
      nu_element->len  = len;
      nu_element->str  = buf;
      this->_stack_str_onto_list(nu_element);
    }
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_unlock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
  }
}


/*
*/
void StringBuilder::prepend(uint8_t* nu, int len) {
  if ((nullptr != nu) && (len > 0)) {
    this->root = _promote_collapsed_into_ll();   // Promote the previously-collapsed string.

    StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
    if (nullptr == nu_element) return;   // O no.
    //nu_element->reap = true;
    nu_element->len  = len;
    nu_element->str  = (uint8_t*) malloc(len+1);
    if (nu_element->str != nullptr) {
      *(nu_element->str + len) = '\0';
      memcpy(nu_element->str, nu, len);
      nu_element->next = this->root;
      this->root = nu_element;
    }
    else {
      // We were able to malloc the slot for the string, but not the buffer for
      // the string itself. We should free() the slot before failing or we will
      // exacerbate an already-present memory crunch.
      free(nu_element);
    }
  }
}


void StringBuilder::concat(uint8_t* nu, int len) {
  if ((nu != nullptr) && (len > 0)) {
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_lock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
    StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
    if (nu_element != nullptr) {
      //nu_element->reap = true;
      nu_element->next = nullptr;
      nu_element->len  = len;
      nu_element->str  = (uint8_t*) malloc(len+1);
      if (nu_element->str != nullptr) {
        *(nu_element->str + len) = '\0';
        memcpy(nu_element->str, nu, len);
        this->_stack_str_onto_list(nu_element);
      }
    }
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_unlock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
  }
}


/**
* Override to make best use of memory for const strings...
*
* Crash warning: Any const char* concat'd to a StringBuilder will not be copied
*   until it is manipulated somehow. So be very careful if you cast to (const char*).
*
* NOTE: Provisionally shunted into concat(char*)
*/
//void StringBuilder::concat(const char *nu) {
//  if (nu != nullptr) {
//    int len = strlen(nu);
//    if (len > 0) {
//      #if defined(__BUILD_HAS_PTHREADS)
//        //pthread_mutex_lock(&_mutex);
//      #elif defined(__BUILD_HAS_FREERTOS)
//      #endif
//      StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
//      if (nu_element != nullptr) {
//        nu_element->reap = false;
//        nu_element->next = nullptr;
//        nu_element->len  = len;
//        nu_element->str  = (uint8_t*) nu;
//        this->_stack_str_onto_list(nu_element);
//      }
//      #if defined(__BUILD_HAS_PTHREADS)
//        //pthread_mutex_unlock(&_mutex);
//      #elif defined(__BUILD_HAS_FREERTOS)
//      #endif
//    }
//  }
//}


void StringBuilder::concat(unsigned char nu) {
  this->concat(&nu, 1);
}
void StringBuilder::concat(char nu) {
  char temp[2] = {nu, 0};
  this->concat(temp);
}
void StringBuilder::concat(int nu) {
  char temp[12] = {0, };
  sprintf(temp, "%d", nu);
  this->concat(temp);
}
void StringBuilder::concat(unsigned int nu) {
  char temp[12] = {0, };
  sprintf(temp, "%u", nu);
  this->concat(temp);
}
void StringBuilder::concat(double nu) {
  char temp[16] = {0, };
  sprintf(temp, "%f", nu);
  this->concat(temp);
}


/**
* Override to cleanly support Our own type.
* This costs a great deal more than concatHandoff(StringBuilder*), but has
*   the advantage of making a deep copy of the argument.
*
* @param nu The StringBuilder to append.
*/
void StringBuilder::concat(StringBuilder* nu) {
  if (nu != nullptr) {
    this->concat(nu->string(), nu->length());
  }
}

/**
* Variadic. No mutex required because all working memory is confined to stack.
*
* @param format is the printf-style formatting string.
* @param ... are the optional variadics.
* @return the byte count written to this StringBuilder, or negative on failure.
*/
int StringBuilder::concatf(const char* format, ...) {
  va_list args;
  int ret = 0;
  va_start(args, format);
  ret = this->concatf(format, args);
  va_end(args);
  return ret;
}


/**
* Variadic. No mutex required because all working memory is confined to stack.
*
* @param format is the printf-style formatting string.
* @param args is a discrete parameter that contains the optional variadics.
* @return the byte count written to this StringBuilder, or negative on failure.
*/
int StringBuilder::concatf(const char* format, va_list args) {
  int len = strlen(format);
  unsigned short f_codes = 0;  // Count how many format codes are in use...
  for (unsigned short i = 0; i < len; i++) {  if (*(format+i) == '%') f_codes++; }
  // Allocate (hopefully) more space than we will need....
  int est_len = len + 512 + (f_codes * 15);   // TODO: Iterate on failure of vsprintf().
  char *temp = (char *) alloca(est_len);
  memset(temp, 0, est_len);
  int ret = 0;
  ret = vsprintf(temp, format, args);
  if (ret > 0) this->concat((char *) temp);
  return ret;
}


#ifdef ARDUINO
/**
* Override to cleanly support Strings.
*
* @param s The String to append.
*/
void StringBuilder::concat(String s) {
  int len = s.length()+1;
  char *out  = (char *) alloca(len);
  memset(out, 0, len);
  s.toCharArray(out, len);
  this->concat((uint8_t*) out, strlen(out));
}
#endif   // ARDUINO



/**
* Given an offset and a length, will throw away any part of the string that falls outside
*   of the given range.
*
* @param offset The index at which to start culling.
* @param length The numbers of characters to retain.
*/
void StringBuilder::cull(int offset, int new_length) {
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  const int CURRENT_LENGTH = this->length();
  if (0 == new_length) {
    // If this is a complicated way to clear the string, do that instead.
    this->clear();
  }
  if (offset >= 0) {                                // If the offset is positive...
    if (CURRENT_LENGTH >= (offset + new_length)) {  // ...and the range exists...
      uint8_t* temp = (uint8_t*) malloc(new_length+1);  // + 1 for null-terminator.
      if (temp != nullptr) {
        this->_collapse_into_buffer();   // Room to optimize here...
        memcpy(temp, (this->str + offset), new_length);
        *(temp + new_length) = '\0';
        this->clear();         // Throw away all else.
        this->str = temp;      // Replace our ref.
        this->col_length = new_length;
      }
    }
  }
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
}


/**
* Given a character count (x), will throw away the first x characters and adjust
*   the object appropriately.
* If the string length is smaller than the parameter, the entire string will be
*   erased.
*
* @param x The numbers of characters to cull.
*/
void StringBuilder::cull(int x) {
  if (x > 0) {
    #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
    if (x >= this->length()) {
      clear();
    }
    else if (x < this->length()) {   // Does the given range exist?
      int remaining_length = this->length()-x;
      unsigned char* temp = (unsigned char*) malloc(remaining_length+1);  // + 1 for null-terminator.
      if (temp != nullptr) {
        *(temp + remaining_length) = '\0';
        this->_collapse_into_buffer();   // Room to optimize here...
        memcpy(temp, (unsigned char*)(this->str + x), remaining_length);
        this->clear();         // Throw away all else.
        this->str = temp;      // Replace our ref.
        this->col_length = remaining_length;
      }
    }
    #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
  }
}


/**
* Trims whitespace from the ends of the string and replaces it.
*/
void StringBuilder::trim() {
  this->_collapse_into_buffer();
  // TODO: How have I not needed this yet? Add it...
  //    ---J. Ian Lindsay   Thu Dec 17 03:22:01 MST 2015
}


/**
* Search the string for the given character.
* Collapses the string before searching.
*
* @param needle The char to search for.
* @return true if the string contains the given character.
*/
bool StringBuilder::contains(char needle) {
  this->_collapse_into_buffer();
  if (this->col_length == 0) {   return false;   }

  for (int i = 0; i < this->col_length; i++) {
    if (needle == *(this->str + i)) return true;
  }
  return false;
}


/**
* TODO: Something about this is broken. Don't use it with confidence.
* Search the string for the given string.
* Collapses the string before searching.
*
* @param needle The string to search for.
* @return true if the string contains the given string.
*/
bool StringBuilder::contains(const char* needle) {
  const int NEEDLE_LEN = strlen(needle);
  this->_collapse_into_buffer();
  if (NEEDLE_LEN > this->col_length) {  return false;   }
  if (NEEDLE_LEN == 0) {                return false;   }

  for (int i = 0; i < (this->col_length - NEEDLE_LEN); i++) {
    int needle_offset = 0;
    while ((*(needle + needle_offset) == *(this->str + i + needle_offset)) && (needle_offset < NEEDLE_LEN)) {
      needle_offset++;
    }
    if (needle_offset == NEEDLE_LEN) {
      //return (*(needle + needle_offset) == *(this->str + i + needle_offset));
      return true;
    }
  }
  return false;
}


/**
* Compares two binary strings on a byte-by-byte basis.
* Returns 1 if the values match. 0 otherwise.
* Collapses the buffer prior to comparing.
* Will compare ONLY the first len bytes, or the length of our present string. Whichever is less.
*
* @param unknown The byte string under test.
* @param len The number of bytes to compare.
* @return 1 if the strings are equal to the comparison limit. 0 otherwise.
*/
int StringBuilder::cmpBinString(uint8_t* unknown, int len) {
  this->_collapse_into_buffer();
  int minimum = (len > this->col_length) ? this->col_length : len;
  for (int i = 0; i < minimum; i++) {
    if (*(unknown+i) != *(this->str+i)) return 0;
  }
  return 1;
}


/**
* If we are going to do something that requires a null-terminated string,
*   make sure that we have one. If we do, this call does nothing.
*   If we don't, we will add it.
*/
void StringBuilder::_null_term_check() {
  if (nullptr != this->str) {
    if (*(this->str + (this->col_length-1)) != '\0') {
      uint8_t* temp = (uint8_t*) malloc(this->col_length+1);
      if (nullptr != temp) {
        *(temp + this->col_length) = '\0';
        memcpy(temp, this->str, this->col_length);
        free(this->str);
        this->str = temp;
      }
    }
  }
}


/**
* Replaces instances of the former argument with the latter.
* Collapses the buffer. Possibly twice.
*
* @param search The string to search for.
* @param replace The string to replace it with.
* @return The number of replacements, or -1 on memory error.
*/
int StringBuilder::replace(const char* needle, const char* replace) {
  int return_value = 0;
  const int NEEDLE_LEN   = strlen(needle);
  const int REPLACE_LEN  = strlen(replace);
  bool dangling_bulk = false;
  this->_collapse_into_buffer();
  if ((this->col_length >= NEEDLE_LEN) && (0 < NEEDLE_LEN)) {
    #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
    _null_term_check();   // This will depend on a null-terminator.

    const int HAYSTACK_LEN = this->col_length;
    uint8_t* tok_start = this->str;
    int haystack_offset = 0;   // Offset in the haystack.
    while (haystack_offset < (HAYSTACK_LEN - (NEEDLE_LEN-1))) {
      int needle_bytes_found = 0;
      // Seek to the end of any potential needle at this offset.
      while ((*(needle + needle_bytes_found) == *(this->str + haystack_offset + needle_bytes_found)) && (needle_bytes_found < NEEDLE_LEN)) {
        needle_bytes_found++;
      }
      if (needle_bytes_found == NEEDLE_LEN) {
        // There is a needle here. Calculate the length of the new string
        //   segment, and insert a list item.
        int len_bulk  = haystack_offset - (tok_start - this->str);
        int len_total = len_bulk + REPLACE_LEN;

        StrLL* nu_element = (StrLL*) malloc(sizeof(StrLL));
        if (nu_element != nullptr) {
          //nu_element->reap = true;
          nu_element->next = nullptr;
          nu_element->len  = len_total;
          nu_element->str  = (uint8_t*) malloc(len_total+1);
          if (nu_element->str != nullptr) {
            *(nu_element->str + len_total) = '\0';
            int local_offset = 0;
            if (0 < len_bulk) {
              memcpy(nu_element->str, tok_start, len_bulk);
              local_offset = len_bulk;
            }
            if (0 < REPLACE_LEN) {
              memcpy(nu_element->str + local_offset, replace, REPLACE_LEN);
            }
          }
          else {
            return -1;
          }
          this->_stack_str_onto_list(nu_element);
        }
        else {
          return -1;
        }
        haystack_offset += NEEDLE_LEN;  // Don't scan the delimiter space again.
        tok_start = this->str + haystack_offset;
        dangling_bulk = (haystack_offset < (HAYSTACK_LEN-1));
        return_value++;
      }
      else {
        haystack_offset++;
      }
    }

    if (0 < return_value) {
      if (dangling_bulk) {
        int len_dangling  = HAYSTACK_LEN - haystack_offset;
        this->concat(tok_start, len_dangling);
      }
      // If we made a replacement, then the collapsed string is obsoleted by the
      //   list. Free to collapsed string, and implode.
      free(this->str);
      this->str = nullptr;
      this->col_length = 0;
    }

    #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
  }
  return return_value;
}


/**
* This function untokenizes the string by a given delimiter.
*
* @param delim The string to use as the delimiter.
* @return The number of tokens that were imploded.
*/
int StringBuilder::implode(const char* delim) {
  int ret = 0;
  if (delim != nullptr) {
    _promote_collapsed_into_ll();
    const int DELIM_LEN     = strlen(delim);
    const int TOKD_STR_LEN  = length();
    const int TOK_COUNT     = count();
    if ((DELIM_LEN > 0) & (TOKD_STR_LEN > 2) & (TOK_COUNT > 1)) {
      // In order for this operation to make sense, we need to have at least
      //   two tokens, and a total string length of at least 2 characters.
      const int TOTAL_STR_LEN = ((TOK_COUNT-1) * DELIM_LEN) + TOKD_STR_LEN;
      int orig_idx = 0;
      this->str = (uint8_t*) malloc(TOTAL_STR_LEN+1);
      this->col_length = TOTAL_STR_LEN;
      if (nullptr != this->str) {
        StrLL* current = this->root;
        for (int ti = 0; ti < (TOK_COUNT-1); ti++) {
          for (int i = 0; i < current->len; i++) {   // Copy the data over.
            *(this->str + orig_idx++) = *(current->str + i);
          }
          for (int i = 0; i < DELIM_LEN; i++) {   // Re-issue the delimiter.
            *(this->str + orig_idx++) = *(delim + i);
          }
          current = current->next;
        }
        for (int i = 0; i < current->len; i++) {   // Copy the last data over.
          *(this->str + orig_idx++) = *(current->str + i);
        }
        this->str[orig_idx++] = 0;    // Null terminate
        _destroy_str_ll(this->root);  // Wipe the linked list.
        ret = TOK_COUNT;
      }
    }
  }
  return ret;
}


/**
* This function subdivides the sum of this String into memory chunks not larger
*   than the parameter.
* The tail chunk may be smaller than the parameter.
*
* @param csize The size of each chunk.
* @return The number of tokens, or -1 on failure.
*/
int StringBuilder::chunk(int csize) {
  int ret = 0;
  if (0 < csize) {
    this->_collapse_into_buffer();
    if (this->col_length != 0) {
      // At this point, we are assured of having a string of non-zero length stored
      //   in a single contiguous buffer. Create a new chain of containers of the
      //   given size, and copy the old data into it.
      StrLL* nu_root = (StrLL*) malloc(sizeof(StrLL));
      if (nu_root) {
        StrLL* current = nu_root;
        int orig_idx   = 0;
        int remaining_len = this->col_length;
        while ((-1 != ret) && (current)) {
          const int LL_BUF_LEN = strict_min((int32_t) csize, (int32_t) remaining_len);
          //current->reap = true;
          current->next = nullptr;
          current->len  = LL_BUF_LEN;
          current->str  = (uint8_t*) malloc(LL_BUF_LEN);
          if (nullptr != current->str) {
            for (int i = 0; i < LL_BUF_LEN; i++) {   // Copy the data over.
              *(current->str + i) = *(this->str + orig_idx++);
            }
            if (nu_root != current) { // Don't do this for the first StrLL.
              _stack_str_onto_list(nu_root, current);
            }
            remaining_len -= LL_BUF_LEN;
            current = (0 >= remaining_len) ? nullptr : (StrLL*) malloc(sizeof(StrLL));
            ret++;
          }
          else {
            if (nu_root != current) {
              free(current);  // Don't stack onto the list. Free immediately.
              current = nullptr;
            }
            ret = -1;
          }
        }
      }
      else {
        ret = -1;
      }

      if (-1 != ret) {
        // The new chain has the data. Wipe the existing linked list.
        clear();
        this->root = nu_root;
      }
      else {
        // Something went wrong along the way. Probably a failure to malloc().
        // Clean up any mess we've made that can't be trusted.
        _destroy_str_ll(nu_root);
      }
    }
  }
  else {
    ret--;
  }
  return ret;
}


/**
* This function tokenizes the sum of this String according to the parameter.
*
* @param delims The delimiter list to be fed to strtok()
* @return The number of tokens.
*/
int StringBuilder::split(const char* delims) {
  int return_value = 0;
  this->_collapse_into_buffer();
  if (this->col_length == 0) {
    return 0;
  }
  _null_term_check();   // This will depend on a null-terminator.
  char *temp_str  = strtok((char *)this->str, delims);
  if (nullptr != temp_str) {
    while (nullptr != temp_str) {
      this->concat(temp_str);
      return_value++;
      temp_str = strtok(nullptr, delims);
    }
    free(this->str);
    this->str = nullptr;
    this->col_length = 0;
  }
  else {
    this->concat("");   // Assure at least one token.
  }
  return return_value;
}


/**
* This method prints ASCII representations of the bytes this instance contains.
*
* @param output The StringBuilder object into which output is written.
*/
void StringBuilder::printDebug(StringBuilder* output) {
  unsigned char* temp = this->string();
  int temp_len  = this->length();

  if ((temp != nullptr) && (temp_len > 0)) {
    for (int i = 0; i < temp_len; i++) {
      output->concatf("%02x ", *(temp + i));
    }
    output->concat("\n");
  }
}


/*******************************************************************************
* Private functions below this block
*******************************************************************************/
/**
* Recursive function to get the length of string material starting from any
*   particular node. Does not consider the collapsed buffer.
*
* @param node The StrLL to treat as the root.
* @return The calculatedlength.
*/
int StringBuilder::_total_str_len(StrLL* node) {
  int len  = 0;
  if (node != nullptr) {
    len  = this->_total_str_len(node->next);
    if (node->str != nullptr)  len  = len + node->len;
  }
  return len;
}

/**
* Recursive function to add a new string onto the end of the list.
*
* @param current The StrLL to treat as the root.
* @param nu A newly-formed StrLL to be added to the list.
* @return The StrLL reference that precedes the parameter nu in the list.
*/
StrLL* StringBuilder::_stack_str_onto_list(StrLL* current, StrLL* nu) {
  if (current != nullptr) {
    if (current->next == nullptr) current->next = nu;
    else this->_stack_str_onto_list(current->next, nu);
  }
  return current;
}

/**
* Non-recursive override to make additions less cumbersome.
*
* @param nu A newly-formed StrLL to be added to the list.
* @return The StrLL reference that precedes the parameter nu in the list.
*/
StrLL* StringBuilder::_stack_str_onto_list(StrLL* nu) {
  StrLL* return_value = nullptr;
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  if (this->root == nullptr) {
    this->root  = nu;
    return_value = this->root;
  }
  else {
    return_value = this->_stack_str_onto_list(this->root, nu);
  }
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  return return_value;
}


/**
* Clean up after ourselves. Frees the memory taken by the LL node, and also that
*   taken by the string itself (if it wasn't passed to the class as a const).
*
* @param r_node The linked list node to be cleaned up.
*/
void StringBuilder::_destroy_str_ll(StrLL* r_node) {
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  if (r_node != nullptr) {
    if (r_node->next != nullptr) {
      _destroy_str_ll(r_node->next);
      r_node->next  = nullptr;
    }
    //if (r_node->reap) free(r_node->str);
    if (r_node->str) free(r_node->str);
    free(r_node);
    if (r_node == this->root) this->root = nullptr;
  }
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
}


/**
* Calling this fxn should take the collapsed string (if any) and prepend it to
*   the list, taking any measures necessary to convince the class that there is
*   no longer a collapsed string present.
* Always returns a pointer to the root of the LL. Changed or otherwise.
*
* @return An StrLL built from str, or NULL if str was empty.
*/
StrLL* StringBuilder::_promote_collapsed_into_ll() {
  if ((nullptr != str) && (col_length > 0)) {
    StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
    if (nullptr != nu_element) {   // This is going to grief us later...
      //nu_element->reap = true;
      nu_element->next = this->root;
      this->root = nu_element;
      nu_element->str = this->str;
      nu_element->len = this->col_length;
      this->str = nullptr;
      this->col_length = 0;
    }
  }
  return this->root;
}


/**
* Traverse the list and keep appending strings to the buffer.
* Will prepend the str buffer if it is not nullptr.
* Updates the length.
*
* @return 0 on success, or negative on failure.
*/
int8_t StringBuilder::_collapse_into_buffer() {
  int8_t ret = -1;
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  StrLL *current = _promote_collapsed_into_ll();   // Promote the previously-collapsed string.
  if (current != nullptr) {
    this->col_length = this->_total_str_len(this->root);
    if (this->col_length > 0) {
      this->str = (uint8_t*) malloc(this->col_length + 1);
      if (this->str != nullptr) {
        ret = 0;
        *(this->str + this->col_length) = '\0';
        int tmp_len = 0;
        while (current != nullptr) {
          if (current->str != nullptr) {
            memcpy((void*)(this->str + tmp_len), (void *)(current->str), current->len);
            tmp_len = tmp_len + current->len;
          }
          current = current->next;
        }
      }
    }
    this->_destroy_str_ll(this->root);
  }
  else {
    ret = 0;
  }
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  return ret;
}


/**
* Return the RAM use of this string.
* By passing true to deep, the return value will also factor in concealed heap
*   overhead and the StringBuilder itself.
* Return value accounts for padding due to alignment constraints.
*
* @param deep will also factor in heap overhead, and the StringBuilder itself.
* @return 0 on success, or negative on failure.
*/
int StringBuilder::memoryCost(bool deep) {
  // TODO: 4 for OVERHEAD_PER_MALLOC is an assumption based on a specific build
  //   of newlib. Find a way to discover it from the build.
  const uint32_t MALLOCS_PER_FRAG    = 2;  // TODO: This will go to unity once merged allocation is done.
  const uint32_t OVERHEAD_PER_CLASS  = (deep ? sizeof(StringBuilder) : 0);
  const uint32_t OVERHEAD_PER_MALLOC = (deep ? (4) : 0);
  const uint32_t OVERHEAD_PER_FRAG   = sizeof(StrLL) + (MALLOCS_PER_FRAG * OVERHEAD_PER_MALLOC);
  int32_t ret = length();
  if (deep & (0 < col_length)) {
    ret += OVERHEAD_PER_MALLOC;
  }
  ret += (count() * OVERHEAD_PER_FRAG);
  return ret;
}
