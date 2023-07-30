/*
File:   C3PRefCounter.h
Author: J. Ian Lindsay
Date:   2023.07.29

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


This class is intended to be a compositional element that implements
  reference-counting. This might be employed for garbage collectors,
  wake-locking, or generally any purpose where a class should be notified
  when nothing is depending on it.

There are two patterns available here.
*/

#ifndef __C3P_REFERENCE_COUNTER_H__
#define __C3P_REFERENCE_COUNTER_H__



/*
* This is the proper class to extend if the reference-counting is to be kept
*   entirely internal within an extending class.
*/
class RefCountable {
  public:
    /**
    * Releases a reference.
    *
    * @return true if the call resulted in a state change.
    */
    bool referenceRelease();

    /**
    * Take a reference.
    *
    * @return true if the call resulted in a state change.
    */
    bool referenceAcquire();

    inline bool     referencesOutstanding() {   return (0 < _refs);  };
    inline uint32_t referenceCount() {          return _refs;        };


  protected:
    RefCountable() : _refs(0) {};    // Constructor.
    ~RefCountable() {};              // Trivial destructor.

    // An extending class should implement this method to be notified of
    //   changes to the reference count.
    void _ref_count_callback(uint32_t outstanding_references) =0;


  private:
    uint32_t  _refs;
};


#endif   // __C3P_REFERENCE_COUNTER_H__
