#ifndef __3_AXIS_COMPASS_H__
#define __3_AXIS_COMPASS_H__

#include "../../CppPotpourri.h"
#include "../../StringBuilder.h"
#include "../../SensorFilter.h"
#include "../../Vector3.h"
#include "TripleAxisPipe.h"


enum class HeadingType : uint8_t {
  MAGNETIC_NORTH  = 0,   // Direction of magnetic North without corrections.
  MAGNETIC_DIP    = 1,   // Angle of dip.
  TRUE_NORTH      = 2,   // Direction of geographic North.
  WAYPOINT        = 3    // Direction towards the given lat/lon.
};

enum class CompassErr : int8_t {
  NO_MEM         = -4,  // Not enough memory.
  PARAM_MISSING  = -3,  // A parameter was not supplied or isn't available.
  PARAM_RANGE    = -2,  // A parameter was out of range.
  UNSPECIFIED    = -1,  // Something failed. Not sure what.
  NONE           = 0,   // No errors.
  UNLIKELY       = 1,   // No errors, but the field is too strong to be Earth's.
  STALE          = 2    // No errors, but data is stale since last check.
};


/*
* Class flags.
*/
#define COMPASS_FLAG_IN_CALIBRATION   0x00000001  // Compass is taking data for calibration.
#define COMPASS_FLAG_CALIBRATED       0x00000002  // Compass is calibrated.
#define COMPASS_FLAG_TILT_COMPENSATE  0x00000004  // Compass controls for tilt.
#define COMPASS_FLAG_GIVEN_ACC_ERR    0x00000008  // We have error bars for ACC.
#define COMPASS_FLAG_GIVEN_MAG_ERR    0x00000010  // We have error bars for MAG.
#define COMPASS_FLAG_COMPASS_FRESH    0x00000020  // Compass is freshly-updated.
#define COMPASS_FLAG_QUANT_4          0x00000100  // Quantize the compass into 4 cardinal directions.
#define COMPASS_FLAG_QUANT_8          0x00000200  // Quantize the compass into 8 cardinal directions.
#define COMPASS_FLAG_QUANT_16         0x00000300  // Quantize the compass into 16 cardinal directions.

#define COMPASS_FLAG_HAVE_ERR_MASK    (COMPASS_FLAG_GIVEN_ACC_ERR | COMPASS_FLAG_GIVEN_MAG_ERR)
#define COMPASS_FLAG_QUANTIZER_MASK   0x00000300  // Quantization flag mask


/*
* An instantiable TripleAxisPipe that implements a sink for magnetometer data.
*/
class TripleAxisCompass : public TripleAxisPipe {
  public:
    TripleAxisCompass() : _CALLBACK(nullptr) {};
    TripleAxisCompass(const TripleAxisTerminalCB cb) : _CALLBACK(cb) {};
    ~TripleAxisCompass() {};

    /**
    * Behavior: If sense parameter matches the local class sense, refreshes
    *   this instance's state and calls callback, if defined. Marks the data
    *   as fresh if the callback is either absent, or returns nonzero.
    *
    * @return 0 on success, or -1 on sense mis-match.
    */
    int8_t pushVector(const SpatialSense s, Vector3f* data, Vector3f* error = nullptr);
    void   printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity);

    CompassErr calibrate();

    void printField(StringBuilder*);
    void printBearing(HeadingType, StringBuilder*);

    CompassErr getBearing(HeadingType, float*);
    CompassErr getBearingToWaypoint(float*, double, double);

    CompassErr setOptions(uint32_t, bool);

    inline Vector3f* getError() {   return &_ERR_COMPASS;   };

    inline void setDelination(float v) {        _declination = v;             };
    inline void setLatLong(double lat, double lon) {
      _latitude  = lat;
      _longitude = lon;
    };
    inline Vector3f* getFieldVector() {   return &_field;    };

    inline bool isCalibrated() {   return _compass_flag(COMPASS_FLAG_CALIBRATED);     };
    inline bool dataReady() {      return _compass_flag(COMPASS_FLAG_COMPASS_FRESH);  };

    static const char* errorString(CompassErr);



  private:
    const TripleAxisTerminalCB _CALLBACK;
    float    _declination  = 0.0;
    double   _latitude     = 0.0;
    double   _longitude    = 0.0;
    uint32_t _update_count = 0;
    uint32_t _last_update  = 0;  // millis() when the field was last updated.
    uint32_t _flags        = 0;
    uint32_t _data_period  = 0;  // How many ms between vector updates?

    // Static orientation of the sensor axes with respect to those of the unit.
    //   Expressed as a normalized deviation from "dead-ahead" (1.0, 0, 0).
    // This vector is generally fixed by hardware and imparted as configuration
    //   prior to init.
    Vector3f _hw_deviation;

    // Generated during calibration. These are corrective vectors applied to
    //   _field to account for distortions induced by the sensor's environment.
    Vector3f _offset_vector;    // Hard iron correction
    Vector3f _scaling_vector;   // Soft iron correction

    Vector3f _gravity;          // Where is "up"?
    Vector3f _ERR_ACC;          // Last recorded error from the IMU.
    Vector3f _field;            // "Magnetic North", corrected for offset and scaling.
    Vector3f _ERR_MAG;          // Last recorded error from the MAG
    Vector3f _tc_field;         // This is the tilt-compensated field vector.
    Vector3f _declined_field;   // Tilt-compensated field corrected for declination.
    Vector3f _bearings;         // x: MagNorth   y: MagDip   z: TrueNorth
    Vector3f _ERR_COMPASS;      // x: MagNorth   y: MagDip   z: TrueNorth


    inline bool _has_error() {
      return (COMPASS_FLAG_HAVE_ERR_MASK == (_flags & COMPASS_FLAG_HAVE_ERR_MASK));
    };

    /* Flag manipulation inlines */
    inline uint32_t _compass_flags() {                return _flags;           };
    inline bool _compass_flag(uint32_t _flag) {       return (_flags & _flag); };
    inline void _compass_flip_flag(uint32_t _flag) {  _flags ^= _flag;         };
    inline void _compass_clear_flag(uint32_t _flag) { _flags &= ~_flag;        };
    inline void _compass_set_flag(uint32_t _flag) {   _flags |= _flag;         };
    inline void _compass_set_flag(uint32_t _flag, bool nu) {
      if (nu) _flags |= _flag;
      else    _flags &= ~_flag;
    };

    int8_t _set_field(float, float, float);

    CompassErr _apply_static_offset();
    CompassErr _apply_magnetic_declination();
    CompassErr _apply_tilt_compensation();
};

#endif   // __3_AXIS_COMPASS_H__
