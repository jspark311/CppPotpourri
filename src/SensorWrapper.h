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


This class is a generic interface to a sensor. That sensor might measure many things.

*/


#ifndef __SENSOR_WRAPPER_INCLUDE_H
#define __SENSOR_WRAPPER_INCLUDE_H

#include <inttypes.h>
#include <string.h>

#include "StringBuilder.h"
#include "cbor-cpp/cbor.h"
#include "CppPotpourri.h"
#include "EnumeratedTypeCodes.h"

class SensorWrapper;

/*
* We have the option of directing a datum to autoreport under certain conditions.
* When such a datum becomes dirty, the SensorWrapper class can call a provided
*   callback with the entire sensor wrapper as an argument.
*/
typedef void (*SensorCallBack) (SensorWrapper*);

/* Flags for an individual piece of data from a sensor. */
#define SENSE_DATUM_FLAG_HARDWARE      0x8000  // The proximate basis of the data is hardware.
#define SENSE_DATUM_FLAG_IS_PROXIED    0x4000  // This datum belongs to another sensor.
#define SENSE_DATUM_FLAG_REPORT_MASK   0x3800  // Reporting behavior bits.
#define SENSE_DATUM_FLAG_DIRTY         0x0004  // This data was updated since it was last read.
#define SENSE_DATUM_FLAG_MEM_ALLOC     0x0002  // Any memory needed for this datum is allocated.
#define SENSE_DATUM_FLAG_IS_ACTIVE     0x0001  // Do we care to collect data?

#define SENSE_DATUM_FLAG_PRELOAD_MASK  0xC000  // What flags are blown away by preload.


/* Flags for the sensor itself. */
#define MANUVR_SENSOR_FLAG_DEVICE_PRESENT   0x0001  // Part was found.
#define MANUVR_SENSOR_FLAG_PINS_CONFIGURED  0x0002  // Low-level pin setup is complete.
#define MANUVR_SENSOR_FLAG_INITIALIZED      0x0004  // Registers are initialized.
#define MANUVR_SENSOR_FLAG_ENABLED          0x0008  // Device is measuring.
#define MANUVR_SENSOR_FLAG_CALIBRATED       0x1000


/* Sensors can automatically report their values. */
enum class SensorParameter : uint8_t {
  UNDEFINED          = 0,
  AUTOGAIN           = 1,   // 0 or 1
  SAMPLE_PERIOD      = 7    // milliseconds
};


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


/* These are possible error codes. */
enum class SensorError : int8_t {
  DATA_UPDATED       =    1,   // There was no error, and fresh data.
  NO_ERROR           =    0,   // There was no error.
  ABSENT             =   -1,   // We failed to talk to the sensor.
  OUT_OF_MEMORY      =   -2,   // Couldn't allocate memory for some sensor-related task.
  WRONG_IDENTITY     =   -3,   // Some sensors come with ID markers in them. If we get one wrong, this happens.
  NOT_LOCAL          =   -4,   // If we try to read or change parameters of a sensor that isn't attached to us.
  INVALID_PARAM_ID   =   -5,   // If we try to read or change a sensor parameter that isn't supported.
  INVALID_PARAM      =   -6,   // If we try to set a sensor parameter to something invalid for an existing register.
  NOT_CALIBRATED     =   -7,   // If we try to read a sensor that is uncalibrated. Not all sensors require this.
  INVALID_DATUM      =   -8,   // If we try to do an operation on a datum that doesn't exist for this sensor.
  UNHANDLED_TYPE     =   -9,   // Issued when we ask for a string conversion that we don't support.
  NULL_POINTER       =  -10,   // What happens when we try to do I/O on a null pointer.
  BUS_ERROR          =  -11,   // If there was some generic bus error that we otherwise can't pinpoint.
  BUS_ABSENT         =  -12,   // If the requested bus is not accessible.
  REG_NOT_WRITABLE   =  -13,   // If we try to write to a read-only register.
  REG_NOT_DEFINED    =  -14,   // If we try to do I/O on a non-existent register.
  DATA_EXHAUSTED     =  -15,   // If we try to poll sensor data faster than the sensor can produce it.
  BAD_TYPE_CONVERT   =  -16,   // If we ask the class to convert types in a way that isn't possible.
  MISSING_CONF       =  -17,   // If we ask the sensor class to perform an operation on parameters that it doesn't have.
  NOT_INITIALIZED    =  -18,   // Tried to do something that requires an initialization that hasn't happened.
  UNIMPLEMENTED      =  -19,   // The requested action was valid, but unimplemented.
  UNDEFINED_ERR      = -128,   //
};


/* This struct allows us to not replicate const data in precious memory. */
typedef struct sense_datum_def_t {
  const char* const desc;     // A brief description of the datum for humans.
  const TCode       type_id;  // The type of the data member.
  const SIUnit      units;    // Real-world units that this datum measures.
  const uint16_t    flgs;     // Flags to preload into the datum.
} DatumDef;


/*
* This class helps us present and manage the unique data that comes out of each sensor.
* Every instance of SensorWrapper should have at least one of these defined for it.
*/
class SensorDatum {
  public:
    const DatumDef* def;                  // A brief description of the datum for humans.

    /* Declaration of obligatory overrides. */
    virtual void        printDebug(StringBuilder*)        =0;
    virtual SensorError printValue(StringBuilder*)        =0;
    virtual SensorError serializeCBOR(StringBuilder*)     =0;
    virtual SensorError serializeRaw(uint8_t*, uint32_t)  =0;

    /* Inline accessors */
    inline uint32_t lastUpdate() { return _last_update;  };
    inline bool isProxied() {    return (_flags & SENSE_DATUM_FLAG_IS_PROXIED);  };
    inline bool mem_ready() {    return (_flags & SENSE_DATUM_FLAG_MEM_ALLOC);   };
    inline bool hardware() {     return (_flags & SENSE_DATUM_FLAG_HARDWARE);    };
    inline bool dirty() {        return (_flags & SENSE_DATUM_FLAG_DIRTY);       };
    inline SensorReporting autoreport() {
      return ((SensorReporting) (_flags & SENSE_DATUM_FLAG_REPORT_MASK));
    };


  protected:
    SensorDatum(const DatumDef* DEF) : def(DEF) {};
    //~SensorDatum();

    inline void dirty(bool x) {
      _flags = (x) ? (_flags | SENSE_DATUM_FLAG_DIRTY) : (_flags & ~SENSE_DATUM_FLAG_DIRTY);
      if (x) _last_update = millis();
    };


  private:
    uint16_t   _flags        = 0;
    uint32_t   _last_update  = 0;
};



class SensorWrapper {
  public:
    /* Declaration of obligatory overrides. */
    virtual SensorError init()                        =0;   // Initialize the sensor.
    virtual SensorError poll()                        =0;   // Polls the class.
    virtual SensorError reset()                       =0;   // Reset the sensor.
    virtual SensorError enable(bool)                  =0;   // Should the sensor be enabled?
    virtual void        printDebug(StringBuilder*)    =0;
    virtual SensorError serialize(cbor::encoder*)     =0;
    virtual SensorError serialize(uint8_t*, uint32_t) =0;

    /* Inline accessors */
    inline bool pinsConfigured() {  return _sensor_flag(MANUVR_SENSOR_FLAG_PINS_CONFIGURED); };
    inline bool devFound() {        return _sensor_flag(MANUVR_SENSOR_FLAG_DEVICE_PRESENT);  };
    inline bool isInitialized() {   return _sensor_flag(MANUVR_SENSOR_FLAG_INITIALIZED);     };
    inline bool isEnabled() {       return _sensor_flag(MANUVR_SENSOR_FLAG_ENABLED);         };
    inline bool isCalibrated() {    return _sensor_flag(MANUVR_SENSOR_FLAG_CALIBRATED);      };
    inline const char* sensorName() {   return _name;   };

    /* Static functions */
    static const char* errorString(SensorError);
    static void printSensorBasicInfo(const SensorWrapper*, StringBuilder*);


  protected:
    //SensorCallBack ar_callback = nullptr; // The pointer to the callback function used for autoreporting.

    SensorWrapper(const char*);
    SensorWrapper(const char*, const char*);
    ~SensorWrapper();

    /* Inline accessors */
    inline void pinsConfigured(bool x) {  _sensor_set_flag(MANUVR_SENSOR_FLAG_PINS_CONFIGURED, x); };
    inline void devFound(bool x) {        _sensor_set_flag(MANUVR_SENSOR_FLAG_DEVICE_PRESENT, x);  };
    inline void isInitialized(bool x) {   _sensor_set_flag(MANUVR_SENSOR_FLAG_INITIALIZED, x);     };
    inline void isEnabled(bool x) {       _sensor_set_flag(MANUVR_SENSOR_FLAG_ENABLED, x);         };
    inline void isCalibrated(bool x) {    _sensor_set_flag(MANUVR_SENSOR_FLAG_CALIBRATED, x);      };

    /* Flag manipulation inlines */
    inline uint16_t _sensor_flags() {                return _flags;           };
    inline bool _sensor_flag(uint16_t _flag) {       return (_flags & _flag); };
    inline void _sensor_clear_flag(uint16_t _flag) { _flags &= ~_flag;        };
    inline void _sensor_set_flag(uint16_t _flag) {   _flags |= _flag;         };
    inline void _sensor_set_flag(uint16_t _flag, bool nu) {
      if (nu) _flags |= _flag;
      else    _flags &= ~_flag;
    };


  private:
    const char*    _name;     // This is the name of the sensor.
    uint16_t       _flags   = 0;       // Holds elementary state and capability info.
};

#endif   // __SENSOR_WRAPPER_INCLUDE_H
