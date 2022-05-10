#include "../CppPotpourri.h"
#include "../Vector3.h"
#include "TripleAxisOrientation.h"
#include "TripleAxisPipe.h"

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
  bool compass_updated = false;
  switch (s) {
    case SpatialSense::ACC:        // A gravity vector.
      {
        if (nullptr != error) {
          _ERR_ACC.set(error);
        }
        Vector3<float> plane_xz(0.0, 1.0, 0.0);  // Sagittal plane.
        Vector3<float> plane_yz(1.0, 0.0, 0.0);  // Coronal plane.
        Vector3<float> proj_up_xz   = _up.projected(&plane_xz);    // Project the up vector onto sagittal plane.
        Vector3<float> proj_up_yz   = _up.projected(&plane_yz);    // Project the up vector onto coronal plane.
        Vector3<float> proj_vect_xz = data->projected(&plane_xz);  // Project the vector onto sagittal plane.
        Vector3<float> proj_vect_yz = data->projected(&plane_yz);  // Project the vector onto coronal plane.
        _gravity.set(
          Vector3<float>::angle(proj_up_xz, proj_vect_xz),
          Vector3<float>::angle(proj_up_yz, proj_vect_yz),
          0   // Unless we have a magnetometer or bearing, we can't track yaw.
        );
        _data_period = wrap_accounted_delta(tmp_millis, _last_update);
        _last_update = tmp_millis;
        _update_count++;
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
      return -1;
  }
  return 0;
}


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
* Functions to output things to the console
*******************************************************************************/


/*******************************************************************************
* Core class functions
*******************************************************************************/
