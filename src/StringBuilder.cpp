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
StringBuilder::StringBuilder() : _root(nullptr) {
  #if defined(__BUILD_HAS_PTHREADS)
    #if defined (PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP)
    _mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
    #else
    _mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
    #endif
  #endif

  // TODO: Remove that^ and replace with thisv.
  #if defined(__BUILD_HAS_CONCURRENT_STRINGBUILDER)
  // TODO: Semaphore check-and-lock.
  #endif  // __BUILD_HAS_CONCURRENT_STRINGBUILDER
}

StringBuilder::StringBuilder(char* initial) : StringBuilder() {
  concat(initial);
}

StringBuilder::StringBuilder(uint8_t* initial, int len) : StringBuilder() {
  concat(initial, len);
}


StringBuilder::StringBuilder(const char* initial) : StringBuilder() {
  concat(initial);
}


/**
* Destructor.
*/
StringBuilder::~StringBuilder() {
  if (_root != nullptr) {
    _destroy_str_ll(_root);
    _root = nullptr;
  }
  #if defined(__BUILD_HAS_PTHREADS)
    pthread_mutex_destroy(&_mutex);
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
  int return_value = 0;
  if (_root != nullptr) {
    return_value += _total_str_len(_root);
  }
  return return_value;
}

/**
* Public fxn to quickly decide if a String is empty. This is significantly
*   faster than testing the actual length, since the allocations don't need to
*   be traversed.
* A string is considered empty if either...
*   a) The total length is zero.
*   b) The total length is one, but the only byte is a null-terminator and checking is not strict.
*
* @param strict will consider the presence of a null-terminator to be "not-empty".
* @return True if this StringBuilder is empty (or zero-length).
*/
bool StringBuilder::isEmpty(const bool strict) {
  switch ((nullptr == _root) ? 0 : _root->len) {
    case 0:   return true;  // No length means no string, strict or not.
    case 1:   return ((0 == *(_root->str)) & strict);
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
  unsigned short return_value = 0;
  StrLL* current = _root;
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
uint8_t* StringBuilder::string() {
  // If there is nothing in this object, return a zero-length string.
  // NOTE: We would previously allocate an empty string to satisfy the caller,
  //   but this causes undue heap thrash, and serves no real purpose.
  // Besides... If the caller is going to take the risk of direct value
  //   manipulation of the memory at the location returned by this function,
  //   then that code needs to at least have enough sense to check a length
  //   first. So the fib we are telling by casting in the manner below should
  //   not expose the user of this class to any illegal memory accesses
  //   (writing to read-only mem) that their bounds-checking wouldn't already
  //   prevent.            --- J. Ian Lindsay Sat 24 Sep 2022 01:03:08 AM MDT
  uint8_t* ret = (uint8_t*) "";
  if (nullptr != _root) {
    _collapse();
    ret = _root->str;
  }
  return ret;
}


/**
*
* @return The byte at the given offset, or 0 on negative or excessive param.
*/
uint8_t StringBuilder::byteAt(const int OFFSET) {
  uint8_t ret = 0;
  int offset = OFFSET;
  StrLL* ll_with_byte = _get_ll_containing_offset(_root, &offset);
  if (nullptr != ll_with_byte) {
    ret = *(ll_with_byte->str + offset);
  }
  return ret;
}


/**
* Wipes the StringBuilder, free'ing memory as appropriate.
*/
void StringBuilder::clear() {
  #if defined(__BUILD_HAS_CONCURRENT_STRINGBUILDER)
    // TODO: Lock with semaphore.
  #endif
  if (_root != nullptr) _destroy_str_ll(_root);
  _root = nullptr;
  #if defined(__BUILD_HAS_CONCURRENT_STRINGBUILDER)
    // TODO: Unlock with semaphore.
  #endif
}


/**
* Convert all printable characters to uppercase. Use only on ASCII strings.
*/
void StringBuilder::toUpper() {
  StrLL *current = _root;
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
  StrLL *current = _root;
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
  StrLL* current = _root;
  int i = 0;
  while ((i != pos) & (nullptr != current)){
    current = current->next;
    i++;
  }
  return ((nullptr != current) ? (char*)current->str : (char*) "");
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
uint8_t* StringBuilder::position(int pos, int *pos_len) {
  StrLL* current = _root;
  int i = 0;
  while ((i != pos) && (nullptr != current)){
    current = current->next;
    i++;
  }
  *pos_len = (nullptr != current) ? current->len : 0;
  return ((nullptr != current) ? current->str : (uint8_t*)"");
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
  StrLL* current = _root;
  StrLL* prior   = nullptr;
  unsigned int i = 0;
  while ((i != pos) && (current != nullptr)){
    prior = current;
    current = current->next;
    i++;
  }
  if (current != nullptr) {
    if (prior == nullptr) {
      _root = current->next;
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
void StringBuilder::concatHandoff(StringBuilder* donar) {
  #if defined(__BUILD_HAS_PTHREADS)
    // TODO: Both this instance, as well as the argument instance must be locked.
    pthread_mutex_lock(&_mutex);
    pthread_mutex_lock(&nu->_mutex);
  #endif
  if ((nullptr != donar) && (!donar->isEmpty(true))) {
    _stack_str_onto_list(donar->_root);
    donar->_root = nullptr;  // Inform the donar instance...
  }
  #if defined(__BUILD_HAS_PTHREADS)
    // TODO: Both this instance, as well as the argument instance must be unlocked.
    pthread_mutex_unlock(&nu->_mutex);
    pthread_mutex_unlock(&_mutex);
  #endif
}


/**
* Like concatHandoff(StringBuilder*), but bounded. Will only transfer the given
*   number of bytes, taking full chunks if possible.
*/
void StringBuilder::concatHandoffLimit(StringBuilder* donar, unsigned int len_limit) {
  #if defined(__BUILD_HAS_PTHREADS)
    // TODO: Both this instance, as well as the argument instance must be locked.
    pthread_mutex_lock(&_mutex);
    pthread_mutex_lock(&donar->_mutex);
  #endif
  if ((0 < len_limit) && (nullptr != donar) && (!donar->isEmpty(true))) {
    StrLL* old_root = donar->_root;  // We'll take this for the moment...
    donar->_root = nullptr;          // Inform the donar instance...
    int offset = (len_limit - 1);    // Get the StrLL containing the last byte to be transfered.
    StrLL* ll_with_byte = _get_ll_containing_offset(old_root, &offset);
    if ((nullptr != ll_with_byte) & (0 < offset)) {
      // We're going to leave some data with the donor. How much?
      if ((ll_with_byte->len - 1) == offset) {
        // The requested length fit neatly on a fragment boundary.
        donar->_root = ll_with_byte->next;    // Inform the donar instance...
        ll_with_byte->next = nullptr;         // ...snip...
        _stack_str_onto_list(old_root);       // ...and stitch.
      }
      else {
        // The limit fell within the available length, but not on a convenient
        //   boundary. So we must actually allocate and shuffle at this point.
        // For this object (left-hand side of the split), we will create a new
        //   StrLL. But for the source object (right-hand side), we will just
        //   shift all the bytes forward that fell outside of the limit, and
        //   change the size. The memory will be reclaimed on the next string
        //   consolidation.
        const int32_t LEN_RIGHT    = ((ll_with_byte->len - 1) - offset);
        const int32_t LEN_LEFT     = (ll_with_byte->len - LEN_RIGHT);
        StrLL* split_left = _create_str_ll(LEN_LEFT, ll_with_byte);
        if (nullptr != split_left) {
          for (int i = 0; i < LEN_RIGHT; i++) {
            *(ll_with_byte->str + i) = *(ll_with_byte->str + i + LEN_LEFT);
          }
          ll_with_byte->len = LEN_RIGHT;
          donar->_root = ll_with_byte;    // Inform the donar instance...

          // Seek through the input and find the point where we need to snip.
          StrLL* current = old_root;
          while ((nullptr != current) && (ll_with_byte != current) && (ll_with_byte != current->next)) {
            current = current->next;
          }
          // To the extent that we can simply move memory references around, do
          //   so. All leading StrLL that fits within the limit is taken directly.
          if (current != old_root) {
            current->next = split_left;       // Snip the list where we left it.
            _stack_str_onto_list(old_root);   // Attach the fragment list before
          }
          else {
            _stack_str_onto_list(split_left);  // No leading whole fragments to take.
          }
        }
      }
    }
    else {
      // We're taking the whole source.
      _stack_str_onto_list(old_root);
    }
  }
  #if defined(__BUILD_HAS_PTHREADS)
    // TODO: Both this instance, as well as the argument instance must be unlocked.
    pthread_mutex_unlock(&donar->_mutex);
    pthread_mutex_unlock(&_mutex);
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
void StringBuilder::prependHandoff(StringBuilder* donar) {
  if (nullptr != donar) {
    #if defined(__BUILD_HAS_CONCURRENT_STRINGBUILDER)
      // TODO: Lock with semaphore.
    #endif
    StrLL* new_root = donar->_root;
    StrLL* current  = new_root;
    StrLL* old_root = _root;

    if (nullptr != new_root) {
      // Scan to the end of the donated LL and tack our existing list to the end of it.
      while (nullptr != current->next) {   current = current->next;  }
      current->next = old_root;
      // Replace our own root and formally take it from the origin instance.
      _root = new_root;
      donar->_root = nullptr;
    }
    #if defined(__BUILD_HAS_CONCURRENT_STRINGBUILDER)
      // TODO: Unlock with semaphore.
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
    StrLL* nu_element = _create_str_ll(4);   // Allocate a stub.
    if (nu_element) {
      nu_element->str = buf;  // By doing this, we will cause destroy to do a
      nu_element->len = len;  //   separate free() on this member.
      _stack_str_onto_list(nu_element);
    }
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_unlock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
  }
}


/*
*/
void StringBuilder::prepend(uint8_t* buf, int len) {
  if ((nullptr != buf) && (len > 0)) {
    StrLL* nu_element = _create_str_ll(len, buf, _root);
    if (nullptr != nu_element) {
      _root = nu_element;
    }
  }
}


void StringBuilder::concat(uint8_t* buf, int len) {
  if ((buf != nullptr) && (len > 0)) {
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_lock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
    StrLL* nu_element = _create_str_ll(len, buf);

    if (nu_element != nullptr) {
      _stack_str_onto_list(nu_element);
    }
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_unlock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
  }
}


void StringBuilder::concat(uint8_t nu) {
  concat(&nu, 1);
}
void StringBuilder::concat(char nu) {
  char temp[2] = {nu, 0};
  concat(temp);
}
void StringBuilder::concat(int nu) {
  char temp[12] = {0, };
  sprintf(temp, "%d", nu);
  concat(temp);
}
void StringBuilder::concat(unsigned int nu) {
  char temp[12] = {0, };
  sprintf(temp, "%u", nu);
  concat(temp);
}
void StringBuilder::concat(double nu) {
  char temp[16] = {0, };
  sprintf(temp, "%f", nu);
  concat(temp);
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
    concat(nu->string(), nu->length());
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
  ret = concatf(format, args);
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
  const int est_len = len + 512 + (f_codes * 15);   // TODO: Iterate on failure of vsprintf().
  char temp[est_len] = {0, };
  int ret = 0;
  ret = vsprintf(temp, format, args);
  if (ret > 0) concat((char *) temp);
  return ret;
}


#ifdef ARDUINO
/**
* Override to cleanly support Strings.
*
* @param s The String to append.
*/
void StringBuilder::concat(String s) {
  const int INPUT_LEN  = s.length()+1;
  char  out[INPUT_LEN] = {0, };
  s.toCharArray(out, len);
  concat((uint8_t*) out, strlen(out));
}
#endif   // ARDUINO



/**
* Given an offset and a length, will throw away any part of the string that
*   falls outside of the given range.
*
* @param offset The index at which to start culling.
* @param length The numbers of characters to retain.
*/
void StringBuilder::cull(int offset, int new_length) {
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  const int CURRENT_LENGTH = length();
  if (0 == new_length) {    // If this is a complicated way to clear
    clear();                //   the string, do that instead.
  }
  else if (offset >= 0) {                           // If the offset is positive...
    if (CURRENT_LENGTH >= (offset + new_length)) {  // ...and the range exists...
      StrLL* culled_str_ll = _create_str_ll(new_length);
      if (nullptr != culled_str_ll) {
        // TODO: Prepend the collapsed string into the list, and eat fragments.
        //   there is no need whatsoever for this fxn to ever call malloc(), let
        //   alone incure almost triple the memory load (worst-case).
        _collapse();   // TODO: Room to optimize here...
        memcpy(culled_str_ll->str, (_root->str + offset), new_length);
        clear();                 // Throw away all else.
        _root = culled_str_ll;   // Re-assign root.
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
    const int CURRENT_LENGTH = length();
    if (0 < CURRENT_LENGTH) {
      if (x >= CURRENT_LENGTH) {    // If this is a complicated way to clear
        clear();                    //   the string, do that instead.
      }
      else {                        // The given range exists.
        const int REMAINING_LENGTH = (CURRENT_LENGTH - x);
        StrLL* culled_str_ll = _create_str_ll(REMAINING_LENGTH);
        if (nullptr != culled_str_ll) {
          // TODO: Prepend the collapsed string into the list, and eat fragments.
          //   there is no need whatsoever for this fxn to ever call malloc(), let
          //   alone incure almost triple the memory load (worst-case).
          _collapse();   // TODO: Room to optimize here...
          memcpy(culled_str_ll->str, (uint8_t*)(_root->str + x), REMAINING_LENGTH);
          clear();                 // Throw away all else.
          _root = culled_str_ll;   // Re-assign root.
        }
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
  _collapse();
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
  if (nullptr != _root) {
    _collapse();   // TODO: Lazy.... Needlessly mutates string.
    for (int i = 0; i < _root->len; i++) {
      if (needle == *(_root->str + i)) return true;
    }
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
  if (nullptr != _root) {
    const int NEEDLE_LEN = strlen(needle);
    _collapse();   // TODO: Lazy.... Needlessly mutates string.
    if (NEEDLE_LEN > _root->len) {  return false;   }
    if (NEEDLE_LEN == 0) {          return false;   }

    for (int i = 0; i < (_root->len - NEEDLE_LEN); i++) {
      int needle_offset = 0;
      while ((*(needle + needle_offset) == *(_root->str + i + needle_offset)) && (needle_offset < NEEDLE_LEN)) {
        needle_offset++;
      }
      if (needle_offset == NEEDLE_LEN) {
        //return (*(needle + needle_offset) == *(_root->str + i + needle_offset));
        return true;
      }
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
  if (nullptr != _root) {
    _collapse();   // TODO: Lazy.... Needlessly mutates string.
    const int MINIMUM_LEN = ((len > _root->len) ? _root->len : len);
    for (int i = 0; i < MINIMUM_LEN; i++) {
      if (*(unknown + i) != *(_root->str + i)) return 0;
    }
  }
  return 1;
}


/**
* Replaces instances of the former argument with the latter.
* Collapses the buffer. Possibly twice.
*
* @param search The string to search for. Must be non-null and non-zero length.
* @param replace The string to replace it with.
* @return The number of replacements, or -1 on memory error.
*/
int StringBuilder::replace(const char* needle, const char* replace) {
  if (nullptr == needle) {  return 0;  }
  int ret = 0;
  const int NEEDLE_LEN   = strlen(needle);
  const int REPLACE_LEN  = ((nullptr == replace) ? 0 : strlen(replace));
  const int HAYSTACK_LEN = length();
  const int LENGTH_DIFFERENTIAL = (REPLACE_LEN - NEEDLE_LEN);

  if ((0 < HAYSTACK_LEN) && (HAYSTACK_LEN >= NEEDLE_LEN) && (0 < NEEDLE_LEN)) {
    #if defined(__BUILD_HAS_CONCURRENT_STRINGBUILDER)
      // TODO: Lock with semaphore.
    #endif  // __BUILD_HAS_CONCURRENT_STRINGBUILDER

    // Iterate through fragments, and replace them only if necessary due to
    //   length differential and needle location, but be wary of failing to
    //   compare across frag boundaries.
    StringBuilder working_sb;     // Writes happen to a working object.
    // Priming of iterator variables that must cross scopes.
    int remaining_search_len = HAYSTACK_LEN;
    StrLL* src_ll         = _root;  // Iterator over the source list.
    int search_hit_idx    = 0;      // Offset within the search string that match is comparing.
    int intra_frag_idx    = 0;      // Offset within the current source fragment.
    StrLL* src_cpy_ll     = src_ll; // If the source needs to be copied for mutation, start here.
    int src_cpy_idx       = 0;      // If the source needs to be copied for mutation, start here.
    int src_cpy_ll_idx    = 0;      // If the source needs to be copied for mutation, start here within the LL.
    int src_cpy_total_idx = 0;      // If the source needs to be copied for mutation, start here within the full length.
    bool allocation_failure = false;
    // Haystack search loop. If the source string were collapsed, this would
    //   be as simple as searching for a substring within a string.
    // But since we don't want to spike the heap or mutate the source mem
    //   needlessly, roll through its fragment list and compare as we go.
    // One byte per-loop is compared against the delimiter.
    while (!allocation_failure & (remaining_search_len-- > 0) & (nullptr != src_ll)) {
      if (*(src_ll->str + intra_frag_idx++) == *(needle + search_hit_idx++)) {
        // Bytes match.
        if (NEEDLE_LEN == search_hit_idx) {  // If it is the conclusion of a match...
          // Create a copy of the string up-to the first byte of the needle...
          const int NEXT_BYTE_OFFSET  = (HAYSTACK_LEN - remaining_search_len);
          const int THIS_BYTE_OFFSET  = (NEXT_BYTE_OFFSET - 1);
          const int NEW_FRAG_LENGTH   = ((NEXT_BYTE_OFFSET - src_cpy_total_idx) - NEEDLE_LEN);
          // There might not be anything to copy, in the case of multiple
          //   replacements with no intervening bytes.
          if (0 < NEW_FRAG_LENGTH) {
            StrLL* chunk_ll = _create_str_ll(NEW_FRAG_LENGTH, src_cpy_ll, src_cpy_ll_idx);
            allocation_failure = (nullptr == chunk_ll);    // Test for failure.
            if (!allocation_failure) {
              // Stack onto the working copy's root. Not ours.
              working_sb._stack_str_onto_list(chunk_ll);
            }
          }

          // If there is a replacement string, add it into the working copy.
          if (0 < REPLACE_LEN) {
            // TODO: There are safety and minor speed-gains to be had by not
            //   doing it this way. implode(replace) when finished?
            working_sb.concat(replace);
          }
          ret++;   // We made a replacement.

          // Do our offset reshuffle. Check against the src_ll boundaries so
          //   that the next comparison happens on valid premises.
          // It won't matter if there was an allocation failure above. It will
          //   just be unused preparation.
          // We want the next copy to start at the byte after this one (which
          //   was the terminal byte of an instance of the search term).
          src_cpy_total_idx = NEXT_BYTE_OFFSET;
          src_cpy_ll_idx    = NEXT_BYTE_OFFSET;
          // NOTE: We are setting src_cpy_ll_idx to the global index, because we
          //   are about to search from root, and therefore, the parameter should
          //   be the absolute (full-string) offset. But upon return,
          src_cpy_ll = _get_ll_containing_offset(_root, &src_cpy_ll_idx);
          search_hit_idx = 0;  // Reset the search
        }
      }
      else {                       // Bytes disagree.
        if (1 < search_hit_idx) {  // If we were into a search, back out of our
          remaining_search_len++;  //   postfixed iterator actions so the
          intra_frag_idx--;        //   current byte gets compared against the
        }                          //   first needle byte.
        search_hit_idx = 0;        // Reset the search.
      }

      // Check against the src_ll to ensure proper gather on boundaries.
      if (intra_frag_idx >= src_ll->len) {
        intra_frag_idx = 0;
        src_ll = src_ll->next;
      }
    }

    if (!allocation_failure) {
      if (0 < ret) {
        // If replacement actually happened, add any tail that probably
        //   followed the final replacement, and mutate the source.
        // search_hit_idx must be considered to avoid pulling in string that was
        //   replaced into nothing at the end of the haystack.
        const int DANGLING_LENGTH = (HAYSTACK_LEN - src_cpy_total_idx);
        if (0 < DANGLING_LENGTH) {
          StrLL* terminal_ll = _create_str_ll(DANGLING_LENGTH, src_cpy_ll, src_cpy_ll_idx);
          allocation_failure = (nullptr == terminal_ll);    // Test for failure.
          if (!allocation_failure) {
            working_sb._stack_str_onto_list(terminal_ll);
          }
        }
        if (!allocation_failure) {
          _destroy_str_ll(_root);  // Wipe the linked list.
          _root = working_sb._root;
          working_sb._root = nullptr;
        }
      }
    }

    if (allocation_failure) {
      ret = -1;
    }

    #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
  }
  return ret;
}


/**
* This function untokenizes the string by a given delimiter.
*
* @param delim The string to use as the delimiter.
* @return The number of tokens that were imploded, or -1 on failure.
*/
int StringBuilder::implode(const char* delim) {
  int ret = 0;
  if (delim != nullptr) {
    const int DELIM_LEN     = strlen(delim);
    const int TOKD_STR_LEN  = length();
    const int TOK_COUNT     = count();
    if ((DELIM_LEN > 0) & (TOKD_STR_LEN > 2) & (TOK_COUNT > 1)) {
      // In order for this operation to make sense, we need to have at least
      //   two tokens, and a total string length of at least 2 characters.
      const int TOTAL_STR_LEN = ((TOK_COUNT-1) * DELIM_LEN) + TOKD_STR_LEN;

      StrLL* chunk_ll = _create_str_ll(TOTAL_STR_LEN);
      if (nullptr != chunk_ll) {
        StrLL* src_ll = _root;       // Iterator over the source list.
        // Priming of iterator variables.
        int total_offset = 0;
        while (nullptr != src_ll) {
          // Copy the fragment's content...
          memcpy((chunk_ll->str + total_offset), src_ll->str, src_ll->len);
          total_offset += src_ll->len;
          src_ll = src_ll->next;
          ret++;
          if (nullptr != src_ll) {
            // and if there is another fragment, copy in the delimiter...
            memcpy((chunk_ll->str + total_offset), delim, DELIM_LEN);
            total_offset += DELIM_LEN;
          }
        }
        _destroy_str_ll(_root);  // Wipe the linked list.
        _root = chunk_ll;
      }
      else {
        ret = -1;
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
* @param CSIZE The size of each chunk.
* @return The number of tokens, or -1 on failure.
*/
int StringBuilder::chunk(const int CSIZE) {
  int ret = -1;
  if (0 < CSIZE) {
    const int TOTAL_STR_LEN = length();
    if (TOTAL_STR_LEN <= CSIZE) {
      // If the requested chunk size holds the whole string in one chunk, we
      //   only need to collapse.
      _collapse();
      ret = 1;
    }
    else {
      // It looks like we'll need to toss the fragments. Create a new instance
      //   on-stack to make the _root isolation easier...
      StrLL* chunk_ll = _create_str_ll(CSIZE);
      bool allocation_failure = (nullptr == chunk_ll);
      if (!allocation_failure) {
        // We allocated. Start building the new object and prepare to
        //   run the chunk loop.
        ret = 0;
        StringBuilder working_sb;        // Writes happen to a working object.
        StrLL* src_ll     = _root;       // Iterator over the source list.
        // Priming of iterator variables that must cross scopes.
        int remaining_len     = TOTAL_STR_LEN;
        int current_chunk_len = CSIZE;
        int intra_frag_idx  = 0;
        int intra_chunk_idx = 0;

        // Chunk loop.
        // Copy the content into the chunk. If the source string were
        //   collapsed, this would be as simple as...
        //   memcpy(chunk_ll->str, _root->str, CSIZE);
        // But since we don't want to spike the heap or mutate the source mem
        //   needlessly, roll through its fragment list and copy as we go.
        while (!allocation_failure & (remaining_len > 0)) {
          while ((intra_chunk_idx < current_chunk_len) & (nullptr != src_ll)) {
            // This is the chunk copy loop. One byte per-loop is written to the
            //   new chunk, Copy the byte we are primed for...
            *(chunk_ll->str + intra_chunk_idx++) = *(src_ll->str + intra_frag_idx++);

            // Check against the src_ll to ensure proper gather on boundaries.
            if (intra_frag_idx >= src_ll->len) {
              intra_frag_idx = 0;
              src_ll = src_ll->next;
            }
          }
          intra_chunk_idx = 0;  // Reset iterator couter.
          ret++;   // Increment the chunk count.

          // Stack onto the working copy's root. Not ours.
          working_sb._stack_str_onto_list(chunk_ll);

          // Do length correction, and decide if we need to loop again.
          remaining_len -= current_chunk_len;
          current_chunk_len = strict_min(remaining_len, CSIZE);
          if (current_chunk_len > 0) {
            // There is another chunk. Loop will run again unless we fail to
            //   allocate a fresh StrLL of the required size.
            chunk_ll = _create_str_ll(current_chunk_len);
            allocation_failure = (nullptr == chunk_ll);    // Test for failure.
          }
        }

        // If all of that happened cleanly, we can clear ourselves, replace our
        //   own root by taking the root from the working copy, and return our
        //   count. Otherwise, return a failure. Memory will manage itself, and
        //   no mutation of this object has occurred.
        if (!allocation_failure) {
          _destroy_str_ll(_root);      // Free the source memory.
          _root = working_sb._root;    // Replace the reference.
          working_sb._root = nullptr;  // Take the memory from the working copy.
        }
        else {
          ret = -1;
        }
      }
    }
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
  if ((nullptr != _root) && (0 < _root->len)) {
    _collapse();
    char* temp_str = strtok((char*) _root->str, delims);
    if (nullptr != temp_str) {
      StrLL* old_root = _root;
      _root = nullptr;
      while (nullptr != temp_str) {
        concat(temp_str);
        return_value++;
        temp_str = strtok(nullptr, delims);
      }
      _destroy_str_ll(old_root);    // Free the source memory.
    }
    //else {
    //  concat("");   // Assure at least one token.
    //}
  }
  return return_value;
}


/**
* This method prints ASCII representations of the bytes this instance contains.
*
* @param output The StringBuilder object into which output is written.
*/
void StringBuilder::printDebug(StringBuilder* output) {
  uint8_t* temp = string();
  int temp_len  = length();

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
  int len = ((node->next != nullptr) ? _total_str_len(node->next) : 0);
  return (len + node->len);
}


/**
* Traverse the fragments and return the one that contains the data at the given
*   offset. If the function returns a not-null pointer, offset will have been
*   updated to contain the offset within that StrLL that corresponds to the
*   offset originally requested. Thus, specific bytes can be asked for.
* NOTE: Recursion in use.
*
* @param str_ll is the StrLL in which to begin searching.
* @param offset is the pass-by-reference parameter mutated by the recursion.
* @return A pointer to the StrLL containing the offset, or nullptr on failure.
*/
StrLL* StringBuilder::_get_ll_containing_offset(StrLL* str_ll, int* offset) {
  StrLL* ret = nullptr;
  const int STACKED_OFFSET = *offset;
  if (STACKED_OFFSET < str_ll->len) {    // If we weren't asked for an offset
    ret = str_ll;                        //   past the end our bounds, we're
  }                                      //   it. Otherwise, return nullptr.
  else if (nullptr != str_ll->next) {    // Can we go deeper?
    *offset = (STACKED_OFFSET - str_ll->len);
    ret = _get_ll_containing_offset(str_ll->next, offset);
  }
  else {           // We could simply fall through, but this adds some safety by
    *offset = -1;  //   setting the mutated parameter to a single state that
  }                //   indicates failure (it would otherwise be undefined).
  return ret;
}


/**
* Traverse the fragments at src and copy them into dest.
* No allocation is done here, and so outright failure isn't possible.
*
* NOTE: Does not preserve structure.
* NOTE: Presumes an existing allocation of adequate length was made for dest.
*
* @param src The StrLL to treat as the root.
* @param dest The StrLL to copy into.
* @param COPY_LEN The number of bytes to copy.
* @param INITIAL_LL_OFFSET The initial offset where copying begins.
* @return The number of bytes copied.
*/
int StringBuilder::_deepcopy_ll_bytes(StrLL* src, StrLL* dest, const int COPY_LEN, int const INITIAL_LL_OFFSET) {
  int ret = 0;
  if ((nullptr != src) & (nullptr != dest) & (0 < COPY_LEN)) {
    if (src->len > INITIAL_LL_OFFSET) {  // We _could_ skip ahead, but fail instead.
      if (dest->len >= COPY_LEN) {  // We _could_ fill as much as possible, but fail instead.
        StrLL* current = src;
        int intra_frag_idx = INITIAL_LL_OFFSET;
        while ((ret < COPY_LEN) & (nullptr != current)) {
          // This is the chunk copy loop. One byte per-loop is written to the
          //   new chunk, Copy the byte we are primed for...
          *(dest->str + ret++) = *(current->str + intra_frag_idx++);

          // Check against the src_ll to ensure proper gather on boundaries.
          if (intra_frag_idx >= current->len) {
            intra_frag_idx = 0;
            current = current->next;
          }
        }
      }
    }
  }
  return ret;
}



/**
* Recursive function to add a new string onto the end of the list.
*
* @param current The StrLL to treat as the root.
* @param nu A newly-formed StrLL to be added to the list.
* @return The StrLL reference that precedes the parameter nu in the list.
*/
StrLL* StringBuilder::_stack_str_onto_list(StrLL* current, StrLL* nu) {
  if (nullptr != current) {
    if (nullptr == current->next) current->next = nu;
    else _stack_str_onto_list(current->next, nu);
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
  if (nullptr == _root) {
    _root  = nu;
    return_value = _root;
  }
  else {
    return_value = _stack_str_onto_list(_root, nu);
  }
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  return return_value;
}



/**
* Choke-point for the creation of a new string fragment.
*
* @param content_len The length of the data to allocate for, and is required.
* @param content_buf Is optional, and contains the content to be copied in.
* @param content_len The length of the data to copy.
* @return The StrLL reference that precedes the parameter nu in the list.
*/
StrLL* StringBuilder::_create_str_ll(int content_len, uint8_t* content_buf, StrLL* nxt_ll) {
  StrLL* ret = nullptr;
  if (content_len > 0) {
    // Over-allocate by content_len to give space for the content in the same allocation.
    // Over-allocate by one byte to ensure we have a null-terminator.
    const int TOTAL_MALLOC_SIZE = (sizeof(StrLL) + content_len + 1);
    ret = (StrLL*) malloc(TOTAL_MALLOC_SIZE);
    if (nullptr != ret) {
      // The str pointer should point to the first byte after the StrLL it is
      //   member to.
      ret->len  = content_len;  // Do not report our silent addition of null.
      ret->next = nxt_ll;
      ret->str  = (((uint8_t*) ret) + sizeof(StrLL));   // Derive content ptr.
      if (nullptr != content_buf) {
        memcpy(ret->str, content_buf, content_len);     // Copy content, if provided.
      }
      else {
        memset(ret->str, 0, content_len);               // Zero content, if not.
      }
      *(ret->str + content_len) = 0;                    // Assign guard-rail.
    }
  }
  return ret;
}


/**
* Choke-point for the creation of a new string fragment.
*
* @param content_len The length of the data to allocate for, and is required.
* @param content_buf Is optional, and contains the content to be copied in.
* @param content_len The length of the data to copy.
* @return The StrLL reference that precedes the parameter nu in the list.
*/
StrLL* StringBuilder::_create_str_ll(int CONTENT_LEN, StrLL* content_ll, int initial_ll_offset) {
  StrLL* ret = nullptr;
  if (nullptr != content_ll) {
    if (0 < CONTENT_LEN) {
      const int SRC_LL_LENGTH = _total_str_len(content_ll);
      if (CONTENT_LEN <= (SRC_LL_LENGTH - initial_ll_offset)) {
        ret = _create_str_ll(CONTENT_LEN);
        if (nullptr != ret) {
          _deepcopy_ll_bytes(content_ll, ret, CONTENT_LEN, initial_ll_offset);
        }
      }
    }
  }
  return ret;
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
    const bool WAS_MERGED_MALLOC = (r_node->str == (((uint8_t*) r_node) + sizeof(StrLL)));
    if (!WAS_MERGED_MALLOC) {
      free(r_node->str);
    }
    free(r_node);
    if (r_node == _root) _root = nullptr;
  }
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
}


/**
* Traverse the fragments and append them each to a flattened buffer.
* Will not mutate the original string on failure.
* Will return success if called on an emptry string.
*
* @return 0 on success, or negative on failure.
*/
int8_t StringBuilder::_collapse() {
  int8_t ret = 0;
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif

  StrLL* current = _root;
  if (nullptr != current) {
    if (_fragged()) {
      // If the string is frag'd, we know that (length > 0), and that we
      //   have work to do.
      // Spike the heap usage briefly to create our new allocation...
      const int TOTAL_STR_LEN = _total_str_len(_root);
      StrLL* collapsesd_str_ll = _create_str_ll(TOTAL_STR_LEN);
      if (nullptr != collapsesd_str_ll) {
        // If that allocation worked, we aren't going to fail.
        // Copy the fragments into the flat buffer...
        int offset = 0;
        while (current != nullptr) {
          memcpy((collapsesd_str_ll->str + offset), current->str, current->len);
          offset += current->len;
          current = current->next;
        }

        // Destroy the original memory and replace the reference.
        _destroy_str_ll(_root);
        _root = collapsesd_str_ll;
      }
      else {
        ret = -1;  // If allocation failed, indicate failure.
      }
    }
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
  // TODO: sizeof(intptr_t) for OVERHEAD_PER_MALLOC is an assumption based on a
  //   specific build of newlib. Find a way to discover it from the build.
  const uint32_t OVERHEAD_PER_CLASS  = (deep ? sizeof(StringBuilder) : 0);
  const uint32_t OVERHEAD_PER_MALLOC = (deep ? sizeof(intptr_t) : 0);
  const uint32_t OVERHEAD_PER_FRAG   = (sizeof(StrLL) + OVERHEAD_PER_MALLOC);
  int32_t ret = OVERHEAD_PER_CLASS;
  StrLL* current = _root;
  while (nullptr != current) {
    ret += (current->len + OVERHEAD_PER_FRAG);
    const bool WAS_MERGED_MALLOC = (current->str == (((uint8_t*) current) + sizeof(StrLL)));
    if (!WAS_MERGED_MALLOC) {
      ret += 4;  // The heap handoff fxn will stub out 4 extra bytes.
      ret += OVERHEAD_PER_MALLOC;  // Plus the extra malloc that had to be done.
    }
    else {
      ret += 1;   // Merged-allocation overdoes it by 1 byte.
    }
    current = current->next;
  }
  return ret;
}


/**
* Is this string fragmented?
* NOTE: This is the basis of Lemma #3.
*
* @return false for contiguous or empty strings. True for fragmented strings.
*/
bool StringBuilder::_fragged() {
  // NOTE: The commented line below is simple, and will work. But we take a
  //   shortcut for efficiency's sake. We don't care about the count. We only
  //   care if there are at least two fragments, and we don't seek to the end
  //   of the list. Similar relationship to length() versus isEmpty().
  // NOTE: Short-circuit operator is required for safety.
  //return (1 < count());
  return ((nullptr != _root) && (nullptr != _root->next));
}
