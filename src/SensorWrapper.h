/*
File:   SensorWrapper.h
Author: J. Ian Lindsay
Date:   2013.11.28

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


This class contains machinery for abstracting and collecting sensor data,
  usually as preparation for sharing over an MLink, but it may also be used in a
  connectionless application for easy management of various data in the program.

The simplest use of this header is to use the SensorCallBack function
*/


#ifndef __SENSOR_WRAPPER_INCLUDE_H
#define __SENSOR_WRAPPER_INCLUDE_H

#include "StringBuilder.h"
#include "CppPotpourri.h"
#include "EnumeratedTypeCodes.h"
#include "C3PValue/C3PValue.h"
#include "AbstractPlatform.h"

class C3PSensor;
class SensorDatum;

/*
* Flags for the sensor itself. These may not be fully-leveraged by drivers
*   extending C3PSensor, but all drivers must supply workable status for
*   these flags.
*/
#define C3P_SENSOR_FLAG_DEVICE_PRESENT   0x01  // Part was found.
#define C3P_SENSOR_FLAG_PINS_CONFIGURED  0x02  // Low-level pin setup is complete.
#define C3P_SENSOR_FLAG_INITIALIZED      0x04  // Registers are initialized.
#define C3P_SENSOR_FLAG_ENABLED          0x08  // Device is measuring.
#define C3P_SENSOR_FLAG_CALIBRATED       0x10

/* Sensors can automatically report their values. */
enum class SensorReporting : uint8_t {
  OFF        = 0,
  NEW_VALUE  = 1,
  EVERY_READ = 2,
  THRESHOLD  = 3,
  RESERVED_0 = 4,
  RESERVED_1 = 5,
  RESERVED_2 = 6,
  RESERVED_3 = 7
};


/*
* These are possible reasons for a callback to happen.
* TODO: Rigidly define contract here.
*/
enum class SensorCallbackCode : int8_t {
  UNSPECIFIED = 0,  // This should never happen.
  SAY_HELLO,        // A SensorDatum might be explicitly announced.
  DATA_UPDATED,     // SensorDatum might announce that it was freshly-updated with a new reading.
  CONF_CHANGE,      // Notice of a change in sensor state.
  SAY_GOODBYE,      // A SensorDatum might withdraw itself from use.
  HARDWARE_FAULT    // Callback is used to relay news of a fault.
};


/*
* We have the option of directing a SensorDatum to autoreport under certain
*   conditions. When such a datum becomes dirty, the SensorDatum class can call
*   an optional provided callback.
* NOTE: If your application uses SensorManager, that class will monopolize the
*   use of this function. Any application-provided callbacks should be set in
*   the SensorManager class in such cases.
*/
typedef int8_t (*SensorCallBack) (const SensorCallbackCode, C3PSensor*, SensorDatum*);


/*
* This class describes a single type of data produced by a sensor. A single
*   sensor may define several of these, which will be returned to the
*   application on request, and will annotate any data callbacks.
* Every instance of C3PSensor should defined at least one of these.
* These objects are to be owned by the sensor that instantiates them. They can
*   be declared as compositional elements of a sensor driver, or new them from
*   the heap. But in either case, it should be done once, and expected that the
*   reference to this class might be held elsewhere. See notes on the
*   callback contract.
*/
class SensorDatum {
  public:
    const SIUnit* const units;  // Real-world units that this datum measures.
    const char* const   desc;   // A brief description of the datum for humans.
    C3PValue value;             // Conceals true type.
    C3PValue error;             // Conceals true type.
    C3PValue threshold_high;    // Conceals true type.
    C3PValue threshold_low;     // Conceals true type.

    SensorDatum(const TCode TYPE, const SIUnit* const UNITS, const char* const DESC);
    ~SensorDatum();

    void        printDebug(StringBuilder*);
    int8_t      serialize(StringBuilder*, TCode);
    int8_t      deserialize(StringBuilder*, TCode);

    /* Inline accessors */
    inline bool            dirty() {           return _dirty;          };
    inline bool            isProxied() {       return _is_proxied;     };
    inline bool            mem_ready() {       return _mem_ready;      };
    inline bool            hardware() {        return _is_hardware;    };
    inline uint32_t        lastUpdate() {      return _last_update;    };
    inline SensorReporting autoreport() {      return _reporting;      };
    inline C3PSensor*      owner() {           return _owner;          };


  private:
    friend C3PSensor;                                     // C3PSensor can directly manipulate this class.
    C3PSensor*      _owner       = nullptr;               // The sensor that owns this datum.
    unsigned int    _last_update = 0;                     // System time of last new data.
    bool            _is_hardware = false;                 // True if this object directly represents hardware.
    bool            _is_proxied  = false;                 // True if this data source is local to firmware.
    bool            _dirty       = false;                 // True if the value is fresh.
    bool            _mem_ready   = false;                 // True if the memory holding the data is initialized.
    SensorReporting _reporting   = SensorReporting::OFF;  // Autoreporting behavior.

    void        dirty(bool x);
};


/*
* C3PSensor is a pure virtual wrapper class that should be implemented by
*   any sensor that wants to use the SensorDatum abstraction.
*/
class C3PSensor {
  public:
    const char* const name;      // This is the name of the sensor.

    /* Declaration of obligatory overrides. */
    virtual uint32_t     datumCount()       =0;
    virtual SensorDatum* getDatum(uint32_t) =0;

    /* Inline accessors */
    inline bool pinsConfigured() {      return _sensor_flag(C3P_SENSOR_FLAG_PINS_CONFIGURED); };
    inline bool devFound() {            return _sensor_flag(C3P_SENSOR_FLAG_DEVICE_PRESENT);  };
    inline bool sensor_initialized() {  return _sensor_flag(C3P_SENSOR_FLAG_INITIALIZED);     };
    inline bool sensor_enabled() {      return _sensor_flag(C3P_SENSOR_FLAG_ENABLED);         };
    inline bool sensor_calibrated() {   return _sensor_flag(C3P_SENSOR_FLAG_CALIBRATED);      };

    inline void setCallback(SensorCallBack x) {    _sensor_cb = x;   };

    static const char* const callbackCodeStr(const SensorCallbackCode);


  protected:
    C3PSensor(const char* const N) : name(N), _sensor_cb(nullptr), _flags(0) {};
    ~C3PSensor();

    inline void _sensor_pinsConfigured(bool x) {  _sensor_set_flag(C3P_SENSOR_FLAG_PINS_CONFIGURED, x); };
    inline void _sensor_devFound(bool x) {        _sensor_set_flag(C3P_SENSOR_FLAG_DEVICE_PRESENT, x);  };
    inline void _sensor_initialized(bool x) {     _sensor_set_flag(C3P_SENSOR_FLAG_INITIALIZED, x);     };
    inline void _sensor_enabled(bool x) {         _sensor_set_flag(C3P_SENSOR_FLAG_ENABLED, x);         };
    inline void _sensor_calibrated(bool x) {      _sensor_set_flag(C3P_SENSOR_FLAG_CALIBRATED, x);      };

    inline int8_t _datum_callback(SensorDatum* datum) {
      return _sensor_cb_general(SensorCallbackCode::DATA_UPDATED, datum);
    };

    void _print_c3p_sensor(StringBuilder*);


  private:
    SensorCallBack _sensor_cb;  // The optional callback function used for autoreporting.
    uint8_t        _flags;      // Holds elementary binary state for sensor.

    // Callback usage...
    inline int8_t _sensor_cb_general(const SensorCallbackCode CODE, SensorDatum* datum) {
      return ((nullptr != _sensor_cb) ? _sensor_cb(CODE, this, datum) : -1);
    };

    /* Flag manipulation inlines */
    inline bool _sensor_flag(uint8_t _flag) {       return (_flags & _flag); };
    inline void _sensor_set_flag(uint8_t _flag, bool nu) {
      if (nu) _flags |= _flag;
      else    _flags &= ~_flag;
    };
};

#endif   // __SENSOR_WRAPPER_INCLUDE_H
