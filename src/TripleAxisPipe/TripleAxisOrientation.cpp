#include "../CppPotpourri.h"
#include "../Vector3.h"
#include "../AbstractPlatform.h"
#include "TripleAxisPipe.h"


#define TRIPAX_ORIENT_FLAG_PENDING_ZERO         0x00000001  // Pending re-zero from upstream.
#define TRIPAX_ORIENT_FLAG_VALUE_DIRTY          0x00000002  // Unread data is available.


/*******************************************************************************
* Overrides from TripleAxisPipe
*******************************************************************************/

/*
* Behavior: If sense parameter represents usable data, refreshes
*   this instance's state and calls callback (if defined) when the compass
*   changes. Marks the data as fresh if the callback is either absent, or
*   returns nonzero.
*
* @return 0 on acceptance, or -1 on sense mis-match.
*/
int8_t TripleAxisOrientation::pushVector(SpatialSense s, Vector3f* data, Vector3f* error) {
  uint32_t tmp_millis = millis();
  int8_t ret = 0;
  switch (s) {
    case SpatialSense::ACC:        // A gravity vector.
      {
        if (nullptr != error) {
          _ERR_ACC.set(error);
        }
        if (_flags.value(TRIPAX_ORIENT_FLAG_PENDING_ZERO)) {
          _up.set(data);
          _flags.clear(TRIPAX_ORIENT_FLAG_PENDING_ZERO);
        }
        Vector3f plane_xz(0.0, 1.0, 0.0);  // Sagittal plane.
        Vector3f plane_yz(1.0, 0.0, 0.0);  // Coronal plane.
        Vector3f proj_up_xz   = _up - _up.projected(plane_xz);      // Project the up vector onto sagittal plane.
        Vector3f proj_up_yz   = _up - _up.projected(plane_yz);      // Project the up vector onto coronal plane.
        Vector3f proj_vect_xz = *data - data->projected(plane_xz);  // Project the vector onto sagittal plane.
        Vector3f proj_vect_yz = *data - data->projected(plane_yz);  // Project the vector onto coronal plane.
        //proj_up_xz.normalize(PI);
        //proj_up_yz.normalize(PI);
        //proj_vect_xz.normalize(PI);
        //proj_vect_yz.normalize(PI);

        _gravity.set(
          //Vector3<float>::angle(proj_up_yz, proj_vect_yz),  // Roll
          //Vector3<float>::angle(proj_up_xz, proj_vect_xz),  // Pitch
          //atanf((proj_up_yz % proj_vect_yz).length() / (proj_up_yz * proj_vect_yz)),  // Roll
          //atanf((proj_up_xz % proj_vect_xz).length() / (proj_up_xz * proj_vect_xz)),  // Pitch
          atan2(proj_vect_yz.y, proj_vect_yz.z) - atan2(proj_up_yz.y, proj_up_yz.z),  // Roll
          atan2(proj_vect_xz.x, proj_vect_xz.z) - atan2(proj_up_xz.x, proj_up_xz.z),  // Pitch
          0   // Unless we have a magnetometer or bearing, we can't track yaw.
        );
        _data_period = wrap_accounted_delta(tmp_millis, _last_update);
        _last_update = tmp_millis;
        _update_count++;
        if (nullptr != _NXT) {
          // If the vector was pushed downstream, we consider it noted. If it
          //   was rejected, we leave the class marked dirty.
          //_filter_dirty = (0 > _NXT->pushVector(SpatialSense::EULER_ANG, &_gravity, &_ERR));
          ret = _NXT->pushVector(SpatialSense::EULER_ANG, &_gravity, &_ERR_ACC);
        }
        if (!_flags.value(TRIPAX_ORIENT_FLAG_VALUE_DIRTY)) {
          _flags.set(TRIPAX_ORIENT_FLAG_VALUE_DIRTY, (0 > ret));
        }
      }
      break;

    case SpatialSense::MAG:        // A field vector.
      if (nullptr != error) {
        _ERR_MAG.set(error);
      }
      break;

    case SpatialSense::EULER_ANG:  // An orientation.
      break;

    case SpatialSense::GYR:        // Ignored by this class.
    case SpatialSense::UNITLESS:   // wat.
    default:
      ret = -1;
  }
  return ret;
}


/*******************************************************************************
* Functions to output things to the console
*******************************************************************************/

void TripleAxisOrientation::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  StringBuilder indent;
  for (uint8_t i = 0; i < stage; i++) {    indent.concat("    ");    }
  output->concatf("%s+-< 3AxisPipe: Orientation >----------------\n", (char*) indent.string());
  output->concatf("%s| Seq number:     %u\n", (char*) indent.string(), _update_count);
  output->concatf("%s| Last update:    %u\n", (char*) indent.string(), _last_update);
  if (_data_period) {
    output->concatf("%s| Data rate:      %.2f vectors/sec\n", indent.string(), (1000.0 / _data_period));
  }
  output->concatf("%s| Up:    (%.4f, %.4f, %.4f)\n", (char*) indent.string(), _up.x, _up.y, _up.z);
  output->concatf("%s| Pitch: %.4f\n", (char*) indent.string(), _gravity.x);
  output->concatf("%s| Roll:  %.4f\n", (char*) indent.string(), _gravity.y);
}



/*******************************************************************************
* Core class functions
*******************************************************************************/

void TripleAxisOrientation::markLevel() {  _flags.set(TRIPAX_ORIENT_FLAG_PENDING_ZERO);   };
bool TripleAxisOrientation::dirty() {      _flags.value(TRIPAX_ORIENT_FLAG_VALUE_DIRTY);  };
Vector3f* TripleAxisOrientation::value() {
  _flags.clear(TRIPAX_ORIENT_FLAG_VALUE_DIRTY);
  return &_gravity;
};
