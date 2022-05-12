/*
File:   TripleAxisPipe.h
Author: J. Ian Lindsay
Date:   2020.05.14

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

#include <inttypes.h>
#include <stdint.h>
#include "../Vector3.h"
#include "../StringBuilder.h"
#include "../SensorFilter.h"
#include "../FlagContainer.h"

#ifndef __TRIPLE_AXIS_PIPE_H__
#define __TRIPLE_AXIS_PIPE_H__

/**
* @page tripleaxispipe
* @section overview Overview
*
*
*/


/*******************************************************************************
* Types and interface
*******************************************************************************/
/**
* @page tripleaxispipe
*
* Different 3-axis sensors this interface supports.
* Interfaces passing this enum must either pass their vectors in SI units, or
*   pass the UNITLESS enum.
* TODO: Tie into SIUnit enum. It is presently an assumed value.
* NOTE: The enum values have been chosen to correspond to shift-sizes in various
*   flag and index implementations. They must be contiguous and begin at 0.
*/
enum class SpatialSense : uint8_t {
  UNITLESS     = 0,  // Unitless scalar
  ACC          = 1,  // Accelerometer. Data/error is given in m/s^2.
  GYR          = 2,  // Gyroscope. Data/error is given in rad/s.
  MAG          = 3,  // Magnetometer. Data/error is given in Teslas.
  EULER_ANG    = 4,  // Orientation (Roll, Pitch, Yaw). Data/error is given in radians.
  BEARING      = 5,  // Orientation on Earth (Mag-North, Mag-Dip, True-North). Data/error is given in radians.
  // TODO: Consider adding statistical and arithmetic intermediary results? EG, STDEV/RMS/MEAN/Etc...
  ENUM_SIZE    = 12  // Top of enum
};


/* Shorthand for a pointer to a callback for value updates.  */
typedef int8_t (*TripleAxisTerminalCB)(SpatialSense, Vector3f* dat, Vector3f* err, uint32_t seq_num);


/*
* Pure virtual class to define a uniform interface between sensors that produce
*   and process 3-axis data.
* TODO: Need to define semantics of return value.
*/
class TripleAxisPipe {
  public:
    /*
    * NOTE: This API only supports symetrical error bars. Might need to extend it.
    *
    * @return 0 on success. Less-than zero otherwise.
    */
    virtual int8_t pushVector(SpatialSense, Vector3f* data, Vector3f* error = nullptr) =0;
    // TODO: Make alternate calling conventions to support double[3]. Might need flag support.
    virtual void   printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity) =0;

    static const char* spatialSenseStr(SpatialSense);
};


/*
* Some TAPs will want flag space and some inline accessors to support common
*   behaviors.
*/
#define TRIPAX_FLAG_RELAY_MASK              0x00000FFF  // Relay map. See notes.
#define TRIPAX_FLAG_ASYNC_BREAK_UNTIL_POLL  0x40000000  // Don't send efferent data synchronously.

class TripleAxisPipeWithFlags : public TripleAxisPipe {
  public:
    /* TripleAxisPipe interface */
    //virtual int8_t pushVector(SpatialSense, Vector3f* data, Vector3f* error = nullptr) =0;
    //virtual void   printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity) =0;

    inline bool relaySense(const SpatialSense E) {
      return _flags.value((1 << (uint8_t) E) & TRIPAX_FLAG_RELAY_MASK);
    };
    inline void relaySense(const SpatialSense E, bool relay) {
      _flags.set((1 << (uint8_t) E) & TRIPAX_FLAG_RELAY_MASK, relay);
    };


  protected:
    FlagContainer32 _flags;
    TripleAxisPipeWithFlags(uint32_t f = 0) : _flags(f) {};

    //int8_t _base_vector_relay();
};



/*******************************************************************************
* Instantiable utility classes
*******************************************************************************/

/**
* An instantiable TripleAxisPipe that forks a single afferent into two
*   efferents. Retains no state apart from refs to its two efferent pathways,
*   denoted "left" and "right".
*/
class TripleAxisFork : public TripleAxisPipe {
  public:
    TripleAxisFork() {};
    TripleAxisFork(TripleAxisPipe* l, TripleAxisPipe* r) : _LEFT(l), _RIGHT(r) {};
    ~TripleAxisFork() {};

    inline void setLeft(TripleAxisPipe* l) {   _LEFT  = l;  };
    inline void setRight(TripleAxisPipe* r) {  _RIGHT = r;  };

    /**
    * Behavior: Pushes left first. Then right, regardless of failure on left.
    *
    * @return 0 on sucess on both sides of the fork, -1 on one failure, or -2 on two failures.
    */
    int8_t pushVector(SpatialSense, Vector3f* data, Vector3f* error = nullptr);
    void   printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity);


  private:
    TripleAxisPipe* _LEFT  = nullptr;
    TripleAxisPipe* _RIGHT = nullptr;
};



/**
* An instantiable TripleAxisPipe that performs a coordinate transform on
*   afferent data to convert it into the conventions of aircraft principal axes
*   before forwarding it onward. Does not mutate the provided data.
*
* NOTE: This class should probably be the first stage after the source in any
*   pipeline where all of the following conditions are true of the afferent data:
*   1) Native axis arrangement is important to know (IE, a compass).
*   2) Native axis arrangement does not match the desired arrangement.
*   3) Native axis arrangement differs from any other 3-axis data with which it might be muxed.
*/
class TripleAxisConvention : public TripleAxisPipe {
  public:
    TripleAxisConvention() {};
    TripleAxisConvention(TripleAxisPipe* nxt, GnomonType ag, GnomonType eg) : _NXT(nxt), _SRC_FMT(ag), _NXT_FMT(eg) {};
    ~TripleAxisConvention() {};

    inline void setNext(TripleAxisPipe* n) {          _NXT  = n;        };
    inline GnomonType afferentGnomon() {              return _SRC_FMT;  };
    inline void       afferentGnomon(GnomonType n) {  _SRC_FMT  = n;    };
    inline GnomonType efferentGnomon() {              return _NXT_FMT;  };
    inline void       efferentGnomon(GnomonType n) {  _NXT_FMT  = n;    };

    /**
    * Behavior: Refreshes this instance's state and calls callback, if defined.
    * Marks the data as fresh if the callback is either absent, or returns nonzero.
    *
    * @return -2 on null NXT, or return code from downstream pushVector() fxn.
    */
    int8_t pushVector(SpatialSense, Vector3f* data, Vector3f* error = nullptr);
    void   printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity);


  private:
    TripleAxisPipe* _NXT = nullptr;
    GnomonType      _SRC_FMT = GnomonType::RH_POS_Z;  // Defaults to no operation.
    GnomonType      _NXT_FMT = GnomonType::RH_POS_Z;  // Defaults to no operation.
};



/**
* Latin: Single end
*
* An instantiable TripleAxisPipe that functions as a storage and change-notice
*   sink at the end of a pipeline, and which handles the single SpatialSense
*   type that the class was constructed with.
*/
class TripleAxisTerminus : public TripleAxisPipe {
  public:
    TripleAxisTerminus(const SpatialSense s, const TripleAxisTerminalCB cb) : _CALLBACK(cb), _SENSE(s) {};
    TripleAxisTerminus(const SpatialSense s) : TripleAxisTerminus(s, nullptr) {};   // For instances that will be polled.
    ~TripleAxisTerminus() {};

    inline const SpatialSense sense() {    return _SENSE;           };
    inline Vector3f*    getData() {        return &_DATA;           };
    inline Vector3f*    getError() {       return &_ERR;            };
    inline uint32_t     lastUpdate() {     return _last_update;     };
    inline uint32_t     updateCount() {    return _update_count;    };
    inline bool         haveError() {      return _has_error;       };
    inline bool         dataFresh() {      return _fresh_data;      };

    /**
    * Atomic accessor with freshness management and return.
    *
    * @return 0 on success with stale data.
    * @return 1 on success with fresh data.
    */
    int8_t getDataWithErr(Vector3f* d, Vector3f* e, uint32_t* seq_num);

    /**
    * Behavior: If sense parameter matches the local class sense, refreshes
    *   this instance's state and calls callback, if defined. Marks the data
    *   as fresh if the callback is either absent, or returns nonzero.
    *
    * @return 0 on success, or -1 on sense mis-match.
    */
    int8_t pushVector(SpatialSense, Vector3f* data, Vector3f* error = nullptr);
    void   printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity);

    /**
    * Resets the class to zero.
    */
    void reset();


  private:
    const TripleAxisTerminalCB _CALLBACK;
    const SpatialSense         _SENSE;
    bool     _has_error    = false;   // TODO: Move bools below into an integer field for alignment authority.
    bool     _fresh_data   = false;   // TODO: Move bools below into an integer field for alignment authority.
    uint16_t _PADDING      = 0;
    uint32_t _last_update  = 0;
    uint32_t _update_count = 0;
    Vector3f _DATA;
    Vector3f _ERR;
};


/**
* TODO:
* Latin: All ends
* An instantiable TripleAxisPipe that functions as a storage and change-notice
*   sink at the end of a pipeline, and which handles all SpatialSense types it
*   receives.
*
* NOTE: Requires dynamic memory, but might be a lower-overhead choice versus
*   three independent copies of TripleAxisTerminus to represent a 9-DoF sensor.
*/
//class TripleAxisOmnitermini : public TripleAxisPipe {
//};


/**
* An instantiable TripleAxisPipe that filters data from a single sense.
* By default, this class will ignore (and decline to relay) all non-matching
*   senses. But this behavior can be changed to ignore-with-relay.
* By default, this class absorbs matching afferent data and re-emits the
*   filter's output as its efferent with the same SpatialSense. But this
*   behavior can be changed to relay the afferent data, and enrich the pipeline
*   with a novel SpatialSense, as an orientation filter would.
* The filtering functionality is provided by SensorFilter3.
*/
class TripleAxisSingleFilter : public TripleAxisPipe, public SensorFilter3<float> {
  public:
    TripleAxisSingleFilter(
      const SpatialSense s,    // Only feed these to the filter.
      TripleAxisPipe* nxt,     // The efferent connection.
      FilteringStrategy strat = FilteringStrategy::RAW, // Optional
      int ws = 0,  // Optional
      int param1 = 0   // Optional
    ) : SensorFilter3<float>(ws, strat), _SENSE(s), _NXT(nxt) {};

    ~TripleAxisSingleFilter() {};

    inline void setNext(TripleAxisPipe* n) {        _NXT  = n;      };
    inline Vector3f*    getData() {        return value();          };
    inline Vector3f*    getError() {       return &_ERR;            };
    inline bool         haveError() {      return _has_error;       };
    inline bool         dataFresh() {      return dirty();          };

    /**
    * Atomic accessor with freshness management and return.
    *
    * @return 0 on success with stale data.
    * @return 1 on success with fresh data.
    */
    int8_t getDataWithErr(Vector3f* d, Vector3f* e);

    /**
    * Behavior: If afferent data matches the SpatialSense the class was constructed with...
    *   1) Adds the afferent vector to the filter's input without relaying it.
    *   2) Sends the filter's output (if any is ready) to the efferent connection.
    *
    * @return -2 on push failure, -4 never, -3 on memory error, -1 on null NXT, or return code from downstream pushVector() fxn.
    */
    int8_t pushVector(SpatialSense, Vector3f* data, Vector3f* error = nullptr);
    void   printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity);


  private:
    const SpatialSense _SENSE;
    bool _has_error    = false;   // TODO: Move bools below into an integer field for alignment authority.
    TripleAxisPipe* _NXT = nullptr;
    Vector3f _ERR;
};


/**
* TODO:
* An instantiable TripleAxisPipe that filters data from many senses.
* By default, this class will ignore (and decline to relay) all non-matching
*   senses. But this behavior can be changed to ignore-with-relay.
* By default, this class absorbs matching afferent data and re-emits the
*   filter's output as its efferent with the same SpatialSense. But this
*   behavior can be changed to relay the afferent data, and enrich the pipeline
*   with a novel SpatialSense, as an orientation filter would.
* The filtering functionality is provided by SensorFilter3.
*/
//class TripleAxisMultiFilter : public TripleAxisPipe {
//};


/**
* An instantiable TripleAxisPipe takes MAG/ACC/GYR, and produces EULER_ANG as
*   its efferent.
* By default, this class relays matching afferent data and inhibits non-match
*   relay. But this behavior can be changed.
* TODO: Internally, this class should use a quat to eliminate gimble lock and
*   reduce branching. But efferent data should be converted to Euler angles for
*   insertion into the pipeline.
*/
class TripleAxisOrientation : public TripleAxisPipe {
  public:
    TripleAxisOrientation() : _NXT(nullptr) {};
    TripleAxisOrientation(TripleAxisPipe* nxt) : _NXT(nxt) {};
    ~TripleAxisOrientation() {};

    int8_t pushVector(SpatialSense s, Vector3f* data, Vector3f* error = nullptr);
    void   printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity);

    // Accessors for calibrating this 3AP node.
    inline Vector3f* getUp() {         return &_up;         };
    inline void setUp(Vector3f* v) {  _up.set(v);           };
    void markLevel();
    bool dirty();
    Vector3f* value();
    inline float pitch() {        return _gravity.x;        };
    inline float roll() {         return _gravity.y;        };


  private:
    TripleAxisPipe* _NXT = nullptr;
    uint32_t _update_count = 0;
    uint32_t _last_update  = 0;  // millis() when the field was last updated.
    uint32_t _data_period  = 0;  // How many ms between vector updates?
    FlagContainer32 _flags;

    Vector3f _up;               // Which direction is "up" when the unit is level?
    Vector3f _gravity;          // Which direction is "up" at this moment?
    Vector3f _ERR_ACC;          // Last recorded error from the IMU.
    Vector3f _ERR_MAG;          // Last recorded error from the MAG.
    Vector3f _ERR_GYRO;         // Last recorded error from the GYRO.

    /* Flag manipulation inlines */
};

#endif // __TRIPLE_AXIS_PIPE_H__
