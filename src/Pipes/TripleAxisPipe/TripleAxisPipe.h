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
#include "../../Vector3.h"
#include "../../Quaternion.h"
#include "../../StringBuilder.h"
#include "../../TimeSeries/TimeSeries.h"
#include "../../TimeSeries/SensorFilter.h"
#include "../../FlagContainer.h"

#ifndef __TRIPLE_AXIS_PIPE_H__
#define __TRIPLE_AXIS_PIPE_H__

/**
* @page tripleaxispipe
* @section overview Overview
*
* TripleAxisPipe (or 3AP, for short) is a vector processing pipeline with
*   semantic tagging and optional error propagation. Its purpose is to
*   facilitate use of common sensors for practical problems in 3-space.
*
* Many of the mid-level problems involving spatial modeling from sensors are
*   general cases of filtering and transform, and are applicatble to vectors
*   from any real-world source. Such pieces should be parameterized and
*   collected under this interface.
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

enum class AxisID : uint8_t {  NONE = 0, X = 1,  Y = 2,  Z = 3  };


/* Shorthand for a pointer to a callback for value updates.  */
typedef int8_t (*TripleAxisTerminalCB)(SpatialSense, Vector3f* dat, Vector3f* err, uint32_t seq_num);


/*
* Pure virtual class to define a uniform interface between sensors that produce
*   and process 3-axis data. This interface is for afferent vectors.
*/
class TripleAxisPipe {
  public:
    /**
    * NOTE: This API only supports symetrical error bars. Might need to extend it.
    *
    * @param SpatialSense to indicate the type of data. (NOTE: May be eventually replaced by SIUnit enum).
    * @param data is a pointer to the vector being pushed.
    * @param error is an optional pointer to an error figure associated with the vector.
    * @return 1 on success with asynchronous processing.
    *         0 on success with synchronous processing.
    *        -1 on non-terminal rejection of vector (try again later).
    *        -2 on terminal rejection of vector.
    */
    // TODO: Make alternate calling conventions to support double[3]. Might need flag support.
    virtual int8_t pushVector(const SpatialSense, Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0) =0;
    virtual void   printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity) =0;

    static const char* spatialSenseStr(const SpatialSense);
};


/*
* Pure virtual that implements an efferent side, in addition to the afferent.
* Implements the base class printPipe().
*/
class TripleAxisPipeWithEfferent : public TripleAxisPipe {
  public:
    inline void next3AP(TripleAxisPipe* n) {  _NXT  = n;    };
    inline TripleAxisPipe* next3AP() {        return _NXT;  };

    void printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity);

  protected:
    TripleAxisPipe* _NXT;
    TripleAxisPipeWithEfferent(TripleAxisPipe* next) : _NXT(next) {};

    virtual void _print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity) =0;
};


/*
* Virtual that implements SpatialSense filtering, in addition to the efferent and
*   afferent sides. It also comes with logic to dictate forwarding policy for
*   the incoming vector stream.
* Although intended for extension, this class can be instantiated to act as a
*   whitelist or blacklist for a specific SpatialSense.
*/
class TripleAxisSenseFilter : public TripleAxisPipeWithEfferent {
  public:
    TripleAxisSenseFilter(const SpatialSense s, TripleAxisPipe* next) : TripleAxisPipeWithEfferent(next), _SENSE(s) {};

    int8_t pushVector(const SpatialSense, Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0);

    inline const SpatialSense spatialSense() {        return _SENSE;                  };
    inline void forwardMismatchedAfferents(bool x) {  _fwd_mismatches = x;            };
    inline void forwardMatchedAfferents(bool x) {     _fwd_matched_afferents = x;     };
    inline bool forwardMismatchedAfferents() {        return _fwd_mismatches;         };
    inline bool forwardMatchedAfferents() {           return _fwd_matched_afferents;  };


  protected:
    const SpatialSense _SENSE;
    bool _fwd_mismatches        = false;   // Should the class forward SpatialSense mismatches?
    bool _fwd_matched_afferents = false;   // Should the class forward SpatialSense matches that it receives?

    // These functions should be overridden for implementing non-trivial behaviors
    //   of the child class.
    virtual int8_t _filtered_push(Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0) {  return 0;  };
    virtual void _print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity);
};




/*******************************************************************************
* Instantiable utility classes
*******************************************************************************/
/**
* Forks a single afferent into two efferents. Retains no state apart from refs
*   to its two efferent pathways, denoted "left" and "right".
*/
class TripleAxisFork : public TripleAxisPipe {
  public:
    TripleAxisFork(TripleAxisPipe* l = nullptr, TripleAxisPipe* r = nullptr) : _LEFT(l), _RIGHT(r) {};
    ~TripleAxisFork() {};

    int8_t pushVector(const SpatialSense, Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0);
    void printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity);

    inline void setLeft(TripleAxisPipe* l) {   _LEFT  = l;  };
    inline void setRight(TripleAxisPipe* r) {  _RIGHT = r;  };


  private:
    TripleAxisPipe* _LEFT;
    TripleAxisPipe* _RIGHT;
};



/**
* This is a simple change-notice sink intended to be used at
*   the end of a pipeline.
*/
class TripleAxisTerminalCallback : public TripleAxisPipe {
  public:
    TripleAxisTerminalCallback(const TripleAxisTerminalCB cb) : _CALLBACK(cb) {};
    ~TripleAxisTerminalCallback() {};

    /**
    * Behavior: If sense parameter matches the local class sense, refreshes
    *   this instance's state and calls callback, if defined. Marks the data
    *   as fresh if the callback is either absent, or returns nonzero.
    *
    * @return 0 on success, or -1 on sense mis-match.
    */
    int8_t pushVector(const SpatialSense, Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0);
    void   printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity);


  private:
    const TripleAxisTerminalCB _CALLBACK;
};



/**
* Performs a coordinate transform on afferents to convert it into a different
*   convention before forwarding it onward. Does not mutate the provided data.
*
* NOTE: This class should probably be the first stage after the source in any
*   pipeline where all of the following conditions are true of the afferent data:
*   1) Native axis arrangement is important to know (IE, a compass).
*   2) Native axis arrangement does not match the desired arrangement.
*   3) Native axis arrangement differs from any other 3-axis data with which it might be muxed.
*/
class TripleAxisRemapper : public TripleAxisPipeWithEfferent {
  public:
    TripleAxisRemapper(TripleAxisPipe* nxt = nullptr) :
      TripleAxisPipeWithEfferent(nxt),
      _ef_0(AxisID::NONE), _ef_1(AxisID::NONE), _ef_2(AxisID::NONE),
      _invert_0(false), _invert_1(false), _invert_2(false) {};
    ~TripleAxisRemapper() {};

    /**
    * Behavior: Refreshes this instance's state and calls callback, if defined.
    * Marks the data as fresh if the callback is either absent, or returns nonzero.
    *
    * @return -2 on null NXT, or return code from downstream pushVector() fxn.
    */
    int8_t pushVector(const SpatialSense, Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0);

    bool mapAfferent(
      AxisID AXIS_EF_0, AxisID AXIS_EF_1, AxisID AXIS_EF_2,
      bool INV_X = false, bool INV_Y = false, bool INV_Z = false
    );

  protected:
    AxisID _ef_0;
    AxisID _ef_1;
    AxisID _ef_2;
    bool   _invert_0;
    bool   _invert_1;
    bool   _invert_2;

    void _print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity);
};


/**
* Profiling tool to help locate expensive parts of the pipeline.
*/
class TripleAxisProfiler : public TripleAxisPipeWithEfferent {
  public:
    TripleAxisProfiler(TripleAxisPipe* nxt = nullptr) : TripleAxisPipeWithEfferent(nxt) {};
    ~TripleAxisProfiler() {};

    int8_t pushVector(const SpatialSense, Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0);

    void reset();


  protected:
    void _print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity);
};



/*
* Performs scaling on the vector stream.
* If the scaling vector is the zero vector, the incoming vector will be
*   normalized.
* If the scaling vector is (1, 1, 1), the incoming vector will be unchanged.
* If the scaling vector components are all equal, the incoming vector will be
*   scaled isotropically (uniformly). Otherwise, scaling will be anisotropic.
*/
class TripleAxisScaling : public TripleAxisPipeWithEfferent {
  public:
    TripleAxisScaling(TripleAxisPipe* nxt = nullptr) : TripleAxisPipeWithEfferent(nxt) {};
    ~TripleAxisScaling() {};

    // Obligate implementation of TripleAxisPipe.
    int8_t pushVector(const SpatialSense s, Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0);

    inline void     scaling(Vector3f sv) {  _scaling_vector.set(sv);       };
    inline void     scaling(float s) {      _scaling_vector.set(s, s, s);  };  // Shorthand for uniform scaling.
    inline Vector3f scaling() {             return _scaling_vector;        };


  protected:
    Vector3f  _scaling_vector;

    void _print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity);
};



/*
* Performs an offset transform on the vector stream.
* If the offset vector is the zero vector, the incoming vector will be
*   unchanged.
*/
class TripleAxisOffset : public TripleAxisPipeWithEfferent {
  public:
    TripleAxisOffset(TripleAxisPipe* nxt = nullptr) : TripleAxisPipeWithEfferent(nxt) {};
    ~TripleAxisOffset() {};

    // Obligate implementation of TripleAxisPipe.
    int8_t pushVector(const SpatialSense s, Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0);

    inline void     offsetVector(Vector3f ov) {    _offset_vector.set(ov);   };
    inline Vector3f offsetVector() {               return _offset_vector;    };


  protected:
    Vector3f _offset_vector;

    void _print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity);
};


/**
* Functions as a storage stage by making a single copy of the afferant that
*   matches the given SpatialSense. This happens to also be the minimum
*   requirement for breaking the pipeline apart into asynchronous chunks.
*/
class TripleAxisStorage : public TripleAxisSenseFilter {
  public:
    TripleAxisStorage(const SpatialSense s, TripleAxisPipe* nxt = nullptr) :
      TripleAxisSenseFilter(s, nxt) {};
    ~TripleAxisStorage() {};

    inline void     setData(Vector3f x) {  _DATA.set(x);          };
    inline Vector3f getData() {            return _DATA;          };
    inline Vector3f getError() {           return _ERR;           };
    inline uint32_t lastUpdate() {         return _last_update;   };
    inline uint32_t updateCount() {        return _update_count;  };
    inline bool     haveError() {          return _has_error;     };
    inline bool     dataFresh() {          return _fresh_data;    };
    //inline void     asyncBreak(bool x) {   _async_break = x;      };
    //inline bool     asyncBreak() {         return _async_break;   };

    /**
    * Atomic accessor with freshness management and return.
    *
    * @return 0 on success with stale data.
    * @return 1 on success with fresh data.
    */
    int8_t getDataWithErr(Vector3f* d, Vector3f* e, uint32_t* seq_num);

    /** Resets the class to zero. */
    void reset();


  protected:
  // TODO: Move bools below into an integer field for alignment authority.
    bool     _has_error    = false;   // TODO: Move bools below into an integer field for alignment authority.
    bool     _fresh_data   = false;   // TODO: Move bools below into an integer field for alignment authority.
    //bool     _async_break  = false;   //
    uint32_t _last_update  = 0;
    uint32_t _update_count = 0;
    Vector3f _DATA;
    Vector3f _ERR;

    // Implementation of TripleAxisSenseFilter.
    virtual int8_t _filtered_push(Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0);
    virtual void   _print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity);
};



/*
* For incoming vectors of a matching SpatialSense, performs a running addition
*   and corresponding error adjustment followed by an efferent push.
*/
class TripleAxisIntegrator : public TripleAxisStorage {
  public:
    TripleAxisIntegrator(const SpatialSense s, TripleAxisPipe* nxt = nullptr) :
      TripleAxisStorage(s, nxt) {};
    ~TripleAxisIntegrator() {};

    //inline bool     overflow() {       return _has_overflow;     };


  protected:
    //bool     _has_overflow   = false;

    // Implementation of TripleAxisSenseFilter.
    virtual int8_t _filtered_push(Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0);
    virtual void   _print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity);
};


/*
* For incoming vectors of a matching SpatialSense, computes the instantaneous
*   rate of change and corresponding error adjustment followed by an efferent
*   push.
*/
class TripleAxisDifferentiator : public TripleAxisStorage {
  public:
    TripleAxisDifferentiator(const SpatialSense s, TripleAxisPipe* nxt = nullptr) :
      TripleAxisStorage(s, nxt) {};
    ~TripleAxisDifferentiator() {};


  protected:
    Vector3f _prior_afferant;
    bool     _primed = false;

    // Implementation of TripleAxisSenseFilter.
    virtual int8_t _filtered_push(Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0);
    virtual void   _print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity);
};



/**
* An instantiable TripleAxisPipe that accumulates data from a single sense.
* By default, this class will ignore (and decline to relay) all non-matching
*   senses. But this behavior can be changed to ignore-with-relay.
* The buffering functionality is provided by TimeSeries3.
*
* Behavior: If afferent data matches the SpatialSense the class was constructed with...
*   1) Adds the afferent vector to the filter's input without relaying it.
*   2) Sends the filter's output (if any is ready) to the efferent connection.
*/
class TripleAxisTimeSeries : public TripleAxisSenseFilter, public TimeSeries3<float> {
  public:
    TripleAxisTimeSeries(
      const SpatialSense s,    // Only feed these to the filter.
      TripleAxisPipe* nxt = nullptr,  // The efferent connection.
      int window_size = 0      // Optional
    ) : TripleAxisSenseFilter(s, nxt), TimeSeries3<float>(window_size) {};

    ~TripleAxisTimeSeries() {};

    inline Vector3f     getData() {        return value();          };
    inline Vector3f     getError() {       return _ERR;             };
    inline bool         haveError() {      return _has_error;       };
    inline bool         dataFresh() {      return dirty();          };

    inline void forwardWhenFull(bool x) {             _fwd_from_backside = x;       };
    inline bool forwardWhenFull() {                 return _fwd_from_backside;      };

    /**
    * Atomic accessor with freshness management and return.
    *
    * @return 0 on success with stale data.
    * @return 1 on success with fresh data.
    */
    int8_t getDataWithErr(Vector3f* d, Vector3f* e);


  protected:
    // TODO: Move bools below into an integer field for alignment authority.
    //bool _ready_for_intake  = false;   // Is this object ready to accept vectors?
    bool _has_error             = false;   // Does this object have an error figure?
    bool _fwd_from_backside     = false;   // Should the class forward SpatialSense matches as they fall out of memory?
    Vector3f _ERR;
    double   _error_cofactor = 1.0D;

    int8_t _filtered_push(Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0);
    void   _print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity);
};



/**
* Accepts MAG/ACC/GYR, and produces EULER_ANG as an efferent.
*
* TODO: Internally, this class should use a quat to eliminate gimble lock and
*   reduce branching. But efferent data should be converted to Euler angles for
*   insertion into the pipeline.
*/
class TripleAxisOrientation : public TripleAxisPipeWithEfferent {
  public:
    TripleAxisOrientation(TripleAxisPipe* nxt = nullptr) : TripleAxisPipeWithEfferent(nxt) {};
    ~TripleAxisOrientation() {};

    int8_t pushVector(const SpatialSense s, Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0);
    void   printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity);

    // Accessors for calibrating this 3AP node.
    inline Vector3f* getUp() {         return &_up;         };
    inline void setUp(Vector3f* v) {  _up.set(v);           };
    void markLevel();
    bool dirty();
    Vector3f* value();
    inline float pitch() {        return _gravity.x;        };
    inline float roll() {         return _gravity.y;        };


  protected:
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



class TripleAxisMadgwick : public TripleAxisPipeWithEfferent {
  public:
    TripleAxisMadgwick(TripleAxisPipe* nxt = nullptr) : TripleAxisPipeWithEfferent(nxt) {};
    ~TripleAxisMadgwick() {};

    void reset();

    int8_t pushVector(const SpatialSense s, Vector3f* data, Vector3f* error = nullptr, uint32_t seq_num = 0);


  protected:
    uint32_t _update_count = 0;
    uint32_t _last_update  = 0;  // millis() when the field was last updated.
    uint32_t _data_period  = 0;  // How many ms between vector updates?
    float    _mag_discard_threshold = 0.8f;  // In Gauss.

    void   _print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity);
};

#endif // __TRIPLE_AXIS_PIPE_H__
