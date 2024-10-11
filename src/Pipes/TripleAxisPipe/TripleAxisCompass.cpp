#include "../../CppPotpourri.h"
#include "../../Vector3.h"
#include "../../AbstractPlatform.h"
#include "TripleAxisCompass.h"
#include "TripleAxisPipe.h"

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

static const char* BEARING_STRINGS[4] = {
  "MAG_NORTH", "MAG_DIP", "TRUE_NORTH", "WAYPOINT"
};


const char* TripleAxisCompass::errorString(CompassErr x) {
  switch (x) {
    case CompassErr::NO_MEM:         return "Out of memory";
    case CompassErr::PARAM_MISSING:  return "Paramter unavailable";
    case CompassErr::PARAM_RANGE:    return "Paramter out of range";
    case CompassErr::UNSPECIFIED:    return "Unspecified";
    case CompassErr::NONE:           return "None";
    case CompassErr::STALE:          return "Stale result";
    case CompassErr::UNLIKELY:       return "Field is too big to be Earth";
  }
  return "UNKNOWN";
}


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
FAST_FUNC int8_t TripleAxisCompass::pushVector(const SpatialSense s, Vector3f* data, Vector3f* error, uint32_t seq_num) {
  uint32_t tmp_millis;
  bool compass_updated = false;
  switch (s) {
    case SpatialSense::ACC:        // A gravity vector.
      _gravity.set(data);
      _gravity.normalize();
      if (nullptr != error) {
        _ERR_ACC.set(error);
        _compass_set_flag(COMPASS_FLAG_GIVEN_ACC_ERR);
      }
      _compass_set_flag(COMPASS_FLAG_TILT_COMPENSATE);
      break;

    case SpatialSense::MAG:        // A field vector.
      _field.set(data);
      if (nullptr != error) {
        _ERR_MAG.set(error);
        _compass_set_flag(COMPASS_FLAG_GIVEN_MAG_ERR);
      }
      _apply_static_offset();
      if (_compass_flag(COMPASS_FLAG_TILT_COMPENSATE)) {
        _apply_tilt_compensation();
        _ERR_COMPASS.set(&_ERR_MAG);   // TODO: Obvious wrongness.
      }
      else {
        _ERR_COMPASS.set(&_ERR_MAG);   // TODO: Obvious wrongness.
      }
      _apply_magnetic_declination();
      tmp_millis = millis();
      _data_period = millis_since(_last_update);
      _last_update = tmp_millis;
      // TODO: Recalculate error bars.
      _update_count++;
      compass_updated = true;
      break;

    case SpatialSense::EULER_ANG:  // An orientation.
      // TODO: Calculate gravity vector from orientation, or use orientation directly?
      //_orientation.set(data);
      //_orientation.normalize();
      break;

    case SpatialSense::GYR:        // Ignored by this class.
    case SpatialSense::UNITLESS:   // wat.
    default:
      return -1;
  }

  if (compass_updated) {
    if (nullptr != _CALLBACK) {
      if ((0 != _CALLBACK(SpatialSense::BEARING, &_bearings, (_has_error() ? &_ERR_COMPASS : nullptr), _update_count))) {
        _compass_set_flag(COMPASS_FLAG_COMPASS_FRESH, true);
      }
    }
    else {
      _compass_set_flag(COMPASS_FLAG_COMPASS_FRESH, true);
    }
  }
  return 0;
}


void TripleAxisCompass::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  StringBuilder indent;
  for (uint8_t i = 0; i < stage; i++) {    indent.concat("    ");    }
  output->concatf("%s+-< 3AxisPipe: Compass >----------------\n", (char*) indent.string());
  output->concatf("%s| Has callback:   %c\n", (char*) indent.string(), (nullptr != _CALLBACK)?'y':'n');
  output->concatf("%s| Seq number:     %u\n", (char*) indent.string(), _update_count);
  output->concatf("%s| Field %s:    (%.4f, %.4f, %.4f)  Magnitude: %.4f\n", (char*) indent.string(), (dataReady()?"FRESH":"STALE"), (double) _field.x, (double) _field.y, (double) _field.z, (double) _field.length());
  output->concatf("%s| Calibrated:     %c\n", (char*) indent.string(), isCalibrated() ? 'y':'n');
  output->concatf("%s| Tilt CTRL:      %c\n", (char*) indent.string(), _compass_flag(COMPASS_FLAG_TILT_COMPENSATE) ? 'y':'n');
  if (_compass_flag(COMPASS_FLAG_TILT_COMPENSATE)) {
    output->concatf("%s| Gravity:        (%.4f, %.4f, %.4f)\n", (char*) indent.string(), (double) _gravity.x, (double) _gravity.y, (double) _gravity.z);
    output->concatf("%s| Tilt-comp mag:  (%.4f, %.4f, %.4f)  Magnitude: %.4f\n", (char*) indent.string(), (double) _tc_field.x, (double) _tc_field.y, (double) _tc_field.z, (double) _tc_field.length());
  }
  if (_has_error()) {
    output->concatf("%s| Error:          (%.4f, %.4f, %.4f)\n", (char*) indent.string(), (double) _ERR_COMPASS.x, (double) _ERR_COMPASS.y, (double) _ERR_COMPASS.z);
  }
  output->concatf("%s| Last update:    %u\n", (char*) indent.string(), _last_update);
  if (_data_period) {
    output->concatf("%s| Data rate:      %.2f vectors/sec\n", indent.string(), (double) (1000.0 / _data_period));
  }

  printBearing(HeadingType::MAGNETIC_NORTH, output);
  printBearing(HeadingType::TRUE_NORTH, output);
}



/*******************************************************************************
* Filtering...
*******************************************************************************/

CompassErr TripleAxisCompass::getBearing(HeadingType ht, float* bearing) {
  switch (ht) {
    case HeadingType::MAGNETIC_NORTH: *bearing = _bearings.x;   break;
    case HeadingType::MAGNETIC_DIP:   *bearing = _bearings.y;   break;
    case HeadingType::TRUE_NORTH:     *bearing = _bearings.z;   break;
    case HeadingType::WAYPOINT:
      return CompassErr::PARAM_RANGE;
    default:
      return CompassErr::UNSPECIFIED;
  }
  return CompassErr::NONE;
}


CompassErr TripleAxisCompass::getBearingToWaypoint(float* bearing, double lat, double lon) {
  return CompassErr::UNSPECIFIED;
}


CompassErr TripleAxisCompass::setOptions(uint32_t mask, bool en) {
  _flags = en ? (_flags | mask) : (_flags & ~mask);
  return CompassErr::NONE;
}


/*******************************************************************************
* Functions to output things to the console
*******************************************************************************/

void TripleAxisCompass::printField(StringBuilder* output) {
  output->concatf("\tField (uT):  (%.4f, %.4f, %.4f)\tMagnitude: %.4f\n", (double) _field.x, (double) _field.y, (double) _field.z, (double) _field.length());
}


void TripleAxisCompass::printBearing(HeadingType ht, StringBuilder* output) {
  float f = 0.0;
  getBearing(ht, &f);
  output->concatf("\t%s:      %.3f\n", BEARING_STRINGS[((uint8_t) ht) & 0x03], (double) f);
}



/*******************************************************************************
* Core class functions
*******************************************************************************/

CompassErr TripleAxisCompass::_apply_static_offset() {
  _field -= _offset_vector;
  //_field.normalize();
  return CompassErr::NONE;
}


CompassErr TripleAxisCompass::_apply_tilt_compensation() {
  float phi   = atan2(_gravity.y, _gravity.z);
  float theta = atan2(_gravity.x, (_gravity.y * sin(phi) + (_gravity.z * cos(phi))));
  _tc_field.x = (_field.x * cos(theta)) + (_field.y * sin(theta) * sin(phi)) + (_field.z * sin(theta) * cos(phi));
  _tc_field.y = (_field.y * cos(phi)) - (_field.z * sin(phi));
  _tc_field.z = (_field.x * sin(theta)) + (_field.y * cos(theta) * sin(phi)) + (_field.z * cos(theta) * cos(phi));
  _bearings.x = atan2(-1 * _tc_field.y, _tc_field.x) * (180 / PI);
  return CompassErr::NONE;
}


CompassErr TripleAxisCompass::_apply_magnetic_declination() {
  float heading = _bearings.x + _declination;
  // Correct for cyclical wrap, if needed.
  if (heading < 0) {          heading += 2*PI;  }
  else if (heading > 2*PI) {  heading -= 2*PI;  }
  _bearings.z = heading;
  return CompassErr::NONE;
}
