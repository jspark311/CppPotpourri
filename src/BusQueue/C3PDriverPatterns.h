/*
File:   C3PDriverPatterns.h
Author: J. Ian Lindsay
Date:   2024.02.10

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


These classes and types are optional composition elements for hardware drivers.
*/

#ifndef __HARDWARE_DEVICE_PATTERNS_H__
#define __HARDWARE_DEVICE_PATTERNS_H__

#include "../Meta/Rationalizer.h"
#include "../CppPotpourri.h"
#include "../FlagContainer.h"
#include "../C3PLogger.h"



/*******************************************************************************
* A pair of classes that track concurrency-sensitive counts for specific uses.
*******************************************************************************/

class IRQStateTracker {
  public:
    IRQStateTracker() : _irqs_noted(0), _irqs_serviced(0) {};
    ~IRQStateTracker() {};

  private:
    uint32_t _irqs_noted;       // How many IRQs hit the pin?
    uint32_t _irqs_serviced;    // How many IRQs have been processed?
};



class BusOpTracker {
  public:
    BusOpTracker() : _io_dispatched(0), _io_called_back(0) {};
    ~BusOpTracker() {};

  private:
    uint32_t _io_dispatched;    // How many I/O operations were sent out?
    uint32_t _io_called_back;   // How many I/O operations came back?
};


/*******************************************************************************
* Abstractions of offboard registers
*******************************************************************************/
/*
* A single device register
* TODO: This is only here as a scribble target for planning. It will not
*   likely be used unless it is converted entirely to const.
*/
class C3PRegShadow {
  public:
    C3PRegShadow() {};
    ~C3PRegShadow() {};

  private:
};


/*
* A collection of device registers. This is the class that a driver would
*   directly compose as a private member. It handles such things as are common
*   to drivers that implement shadow registers:
*
* 1. Handles pooled memory allocation and concerns surrounding the endianness of
*      their content, if they are multi-byte.
* 2. Distinguishes between indicies and addresses.
* 3. Utility functions for determining address continuity and writability,
*      double-buffering, and dirty detection.
* 4. Provides a uniform console API for manipulation of content.
* 5. Provides optional double-buffering and dirty detection.
*
* It is not required that all device registers be handled by this class, if it
*   is used. Many devices have "that special register" that is quirky for
*   whatever reason. Such registers should behandled specifically by the driver,
*   and omitted from the list of registers passed into this class constructor.
*
********************************************************************************
* Constraints:
* 1. The length of any given register must be at least 1-byte, and cannot
*      exceed 4-bytes.
* 2. Addresses and indicies for registers must both be single-byte.
* 3. Addresses and indicies for registers must have a 1-to-1 relationship.
* 4. Constraints (1), (2), and (3) imply a maximum register shadow volume of
*      1024-bytes (2048-bytes for double-buffered register sets).
* 5. The entire register set is either double-buffered, or not.
* 6. This class has no means of doing safety checks on values. Specifically,
*      registers with bits that are reserved must be handled by the client
*      class. An entire register is considered to be writable/readable or not.
* 7. Registers that have differential meaning if written versus read are
*      supported, but only if double-buffering is used. Otherwise, such
*      registers will effectively ignore their SPLIT_RW fields, and the
*      most-recent I/O operation will define the value of the shadow.
*/
class C3PRegShadows {
  public:
    C3PRegShadows() {};
    ~C3PRegShadows() {};

    uint32_t readShadow(uint8_t idx);
    uint32_t readShadow(uint8_t idx,  const uint32_t);
    uint32_t writeShadow(uint8_t idx);
    uint32_t writeShadow(uint8_t idx, const uint32_t);
    uint32_t mergedShadow(uint8_t idx);


  private:
    bool     _double_buffered;  // Is double-buffering in use?
    uint8_t* _shadows;          // Pointer to shadow register memory pool.

    /*
    * The _metadata member stores a packed bitfield expressing handling rules
    *   for content. The size of the bitfield for each register is the sum of
    *   these sizes.
    * Bit    Purpose
    * --------------------------------------------------------------------------
    * 0-1:   Content length, in bytes. 0 indicates 1-byte.
    * 2:     Endian bit (big if set)
    * 3:     Readable
    * 4:     Writable
    * 5:     SPLIT_RW (if set, read and write shadows are considered distinct)
    * 6:     Render as decimal (only impacts console output).
    */
    uint8_t* _metadata;
};



/*******************************************************************************
* Interfaces for common driver features.
*******************************************************************************/

/* An interface for PWM drivers to implement. */
class C3PInterfacePWM {
  public:
    virtual int8_t   frequency(uint32_t HZ) =0;
    virtual uint32_t frequency()            =0;
    virtual int8_t   dutyRatio(const float) =0;
    virtual float    dutyRatio()            =0;
};


#endif __HARDWARE_DEVICE_PATTERNS_H__
