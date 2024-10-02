/*
File:   TripleAxisPipe.cpp
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

#include "TripleAxisPipe.h"
#include "../../AbstractPlatform.h"


/*******************************************************************************
* Static class utility members
*******************************************************************************/

const char* TripleAxisPipe::spatialSenseStr(SpatialSense s) {
  switch (s) {
    case SpatialSense::UNITLESS:   return "UNITLESS";
    case SpatialSense::ACC:        return "ACC";
    case SpatialSense::GYR:        return "GYR";
    case SpatialSense::MAG:        return "MAG";
    case SpatialSense::EULER_ANG:  return "EULER_ANG";
    case SpatialSense::BEARING:    return "BEARING";
    default:
      break;
  }
  return "UNDEFINED";
}



/*******************************************************************************
* TripleAxisFork
*******************************************************************************/

/*
* Behavior: Pushes left first. Then right, regardless of failure on left.
*
* @return 0 on success if either side of the fork reports success, -1 on failure.
*/
FAST_FUNC int8_t TripleAxisFork::pushVector(const SpatialSense s, Vector3f* data, Vector3f* error, uint32_t seq_num) {
  const bool LEFT_SUCCESS  = ((nullptr != _LEFT) && (0 == _LEFT->pushVector(s, data, error, seq_num)));
  const bool RIGHT_SUCCESS = ((nullptr != _RIGHT) && (0 == _RIGHT->pushVector(s, data, error, seq_num)));
  return ((LEFT_SUCCESS | RIGHT_SUCCESS) ? 0 : -1);
};


void TripleAxisFork::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  StringBuilder indent;
  for (uint8_t i = 0; i < stage; i++) {    indent.concat("    ");    }
  indent.concat("+-< 3AxisPipe: Fork >-------------------\n");

  if ((verbosity > 0) && (nullptr != _LEFT)) {
    _LEFT->printPipe(&indent, stage+1, verbosity);
  }
  if ((verbosity > 0) && (nullptr != _RIGHT)) {
    _RIGHT->printPipe(&indent, stage+1, verbosity);
  }
  indent.string();
  output->concatHandoff(&indent);
}



/*******************************************************************************
* TripleAxisRemapper
*******************************************************************************/

/*
* Behavior: Refreshes this instance's state and calls callback, if defined.
* Marks the data as fresh if the callback is either absent, or returns nonzero.
*
* @return -2 on null NXT, or return code from downstream pushVector() fxn.
*/
FAST_FUNC int8_t TripleAxisRemapper::pushVector(const SpatialSense s, Vector3f* data, Vector3f* error, uint32_t seq_num) {
  int8_t ret = -2;
  if (nullptr != _NXT) {
    Vector3f tmp_err(0.0f, 0.0f, 0.0f);
    Vector3f tmp_dat(
      (data->x * (_invert_0 ? -1.0f : 1.0f)),
      (data->y * (_invert_1 ? -1.0f : 1.0f)),
      (data->z * (_invert_2 ? -1.0f : 1.0f))
    );

    if (nullptr != error) {
      tmp_err.set(
        (error->x * (_invert_0 ? -1.0f : 1.0f)),
        (error->y * (_invert_1 ? -1.0f : 1.0f)),
        (error->z * (_invert_2 ? -1.0f : 1.0f))
      );
    }

    Vector3f stacked_err(0.0f, 0.0f, 0.0f);
    Vector3f stacked_dat(0.0f, 0.0f, 0.0f);

    ret++;
    switch (_ef_0) {
      case AxisID::X:  stacked_dat.x = tmp_dat.x;  stacked_err.x = tmp_err.x;  break;
      case AxisID::Y:  stacked_dat.y = tmp_dat.x;  stacked_err.y = tmp_err.x;  break;
      case AxisID::Z:  stacked_dat.z = tmp_dat.x;  stacked_err.z = tmp_err.x;  break;
      default:         break;
    }
    switch (_ef_1) {
      case AxisID::X:  stacked_dat.x = tmp_dat.y;  stacked_err.x = tmp_err.y;  break;
      case AxisID::Y:  stacked_dat.y = tmp_dat.y;  stacked_err.y = tmp_err.y;  break;
      case AxisID::Z:  stacked_dat.z = tmp_dat.y;  stacked_err.z = tmp_err.y;  break;
      default:         break;
    }
    switch (_ef_2) {
      case AxisID::X:  stacked_dat.x = tmp_dat.z;  stacked_err.x = tmp_err.z;  break;
      case AxisID::Y:  stacked_dat.y = tmp_dat.z;  stacked_err.y = tmp_err.z;  break;
      case AxisID::Z:  stacked_dat.z = tmp_dat.z;  stacked_err.z = tmp_err.z;  break;
      default:         break;
    }
    if (0 == _NXT->pushVector(s, &stacked_dat, &stacked_err, seq_num)) {
      ret = 0;
    }
  }
  return ret;
}


void TripleAxisRemapper::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  StringBuilder indent;
  for (uint8_t i = 0; i < stage; i++) {    indent.concat("    ");    }
  output->concat(&indent);
  output->concat("+-< 3AxisPipe: Remapper >-------------\n");
  if ((verbosity > 0) && (nullptr != _NXT)) {
    _NXT->printPipe(output, stage+1, verbosity);
  }
}


bool TripleAxisRemapper::mapAffernt(
  AxisID AXIS_EF_0, AxisID AXIS_EF_1, AxisID AXIS_EF_2,
  bool INV_X, bool INV_Y, bool INV_Z
)
{
  _ef_0     = AXIS_EF_0;
  _ef_1     = AXIS_EF_1;
  _ef_2     = AXIS_EF_2;
  _invert_0 = INV_X;
  _invert_1 = INV_Y;
  _invert_2 = INV_Z;
  return true;
}



/*******************************************************************************
* TripleAxisTerminus
*******************************************************************************/

/*
* Atomic accessor with freshness management and return indication.
*
* @return 0 on success with stale data.
* @return 1 on success with fresh data.
*/
int8_t TripleAxisTerminus::getDataWithErr(Vector3f* d, Vector3f* e, uint32_t* c) {
  int8_t ret = _fresh_data ? 1 : 0;
  _fresh_data = false;
  d->set(&_DATA);
  if (nullptr != c) {
    *c = _update_count;
  }
  if ((nullptr != e) && (_has_error)) {
    e->set(&_ERR);
  }
  return ret;
}



/*
* Behavior: If sense parameter matches the local class sense, refreshes
*   this instance's state and calls callback, if defined. Marks the data
*   as fresh if the callback is either absent, or returns nonzero.
*
* @return 0 on refresh, or -1 on sense mis-match.
*/
FAST_FUNC int8_t TripleAxisTerminus::pushVector(const SpatialSense s, Vector3f* data, Vector3f* error, uint32_t seq_num) {
  int8_t ret = -1;
  if (_SENSE == s) {
    ret = 0;
    _last_update = millis();
    _update_count++;
    _DATA.set(data);
    if (nullptr != error) {
      _ERR.set(error);
      _has_error = true;
    }
    if (nullptr != _CALLBACK) {
      _fresh_data = (0 != _CALLBACK(_SENSE, &_DATA, (_has_error ? &_ERR : nullptr), seq_num));
    }
    else {
      _fresh_data = true;
    }
  }
  return ret;
}


/*
* Resets the class to zero.
*/
void TripleAxisTerminus::reset() {
  _has_error    = false;
  _fresh_data   = false;
  _last_update  = 0;
  _update_count = 0;
  _DATA.set(0.0, 0.0, 0.0);
  _ERR.set(0.0, 0.0, 0.0);
}


void TripleAxisTerminus::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  StringBuilder indent;
  for (uint8_t i = 0; i < stage; i++) {    indent.concat("    ");    }
  indent.string();
  output->concat(&indent);
  output->concat("+-< 3AxisPipe: Terminus >---------------\n");
  output->concat(&indent);
  output->concatf("| Has callback:   %c\n", (nullptr != _CALLBACK)?'y':'n');
  output->concat(&indent);
  output->concatf("| Seq number:     %u\n", _update_count);
  output->concat(&indent);
  output->concatf("| SpatialSense:   %s\n", TripleAxisPipe::spatialSenseStr(_SENSE));
  output->concat(&indent);
  output->concatf("| Value %s:    (%.3f, %.3f, %.3f)\n", (_fresh_data?"FRESH":"STALE"), (double) _DATA.x, (double) _DATA.y, (double) _DATA.z);
  output->concat(&indent);
  if (_has_error) {
    output->concatf("| Error:          (%.3f, %.3f, %.3f)\n", (double) _ERR.x, (double) _ERR.y, (double) _ERR.z);
  }
  output->concatf("| Last update:    %u\n", _last_update);
}


/*******************************************************************************
* TripleAxisTimeSeries
*******************************************************************************/

/*
* Atomic accessor with freshness management and return indication.
*
* @return 0 on success with stale data.
* @return 1 on success with fresh data.
*/
int8_t TripleAxisTimeSeries::getDataWithErr(Vector3f* d, Vector3f* e) {
  int8_t ret = dirty() ? 1 : 0;
  d->set(value());
  if (nullptr != e) {
    if (_has_error) {
      // TODO: This class might do extensive massaging of data, and it will certainly
      //   impact not only the value of the error, but also what the value _means_.
      //   Carefully review this for accurate processing once things are working.
      // NOTE: Any incoming error figure will be multiplied by the standard error
      //   cofactor associated with the depth of the window.
      // TODO: Assumes error is constant between samples. If it isn't,
      //   the true error figure of the data will converge to the value
      //   reported to the filter. If the error figure changes from
      //   upstream, it might be best to clear the filter, rather than
      //   allow epistemologically unsound data to pass.
      // NOTE: Assumes error represents an even distribution with no
      //   correlation between samples. I believe that this would make
      //   the reported error figure more conservative than may be
      //   warranted from a given source.
      e->set(_ERR / _error_cofactor);
    }
    else {
      e->zero();
    }
  }
  return ret;
}


/*
* Behavior: If sense parameter matches the local class sense, refreshes
*   this instance's state and calls callback, if defined. Marks the data
*   as fresh if the callback is either absent, or returns nonzero.
*
* @return -2 on push failure, -1 on broken pipe, or 0 on success.
*/
FAST_FUNC int8_t TripleAxisTimeSeries::pushVector(const SpatialSense s, Vector3f* data, Vector3f* error, uint32_t seq_num) {
  int8_t ret = -1;
  if (_SENSE == s) {
    if (!initialized()) {
      init();  // Init memory on first-use. See class notes.
    }
    ret = -2;
    if (initialized()) {
      switch (feedSeries(data)) {
        case 1:    // Value accepted into filter. New result ready.
          if (nullptr != error) {
            // We will retain the afferant error figure. Adjusting this figure for
            //   efferant push() will be done at that time. Doing it this way will
            //   avoid destroying information about past incoming error figures,
            //   which is important for detecting changes.
            if (_has_error) {
              // TODO: Which we aren't yet doing.
            }
            else {
              _ERR.set(error);
              _error_cofactor = sqrt(windowSize());
              _has_error = true;
            }
          }
          if (_fwd_from_backside && (nullptr != _NXT)) {
            Vector3f tmp_err;
            Vector3f tmp_dat = value();
            if (_has_error) {
              tmp_err = (_ERR / _error_cofactor);
            }
            ret = _NXT->pushVector(_SENSE, &tmp_dat, &tmp_err, seq_num);
          }
          // NOTE: No break;
        case 0:    // Value accepted into filter. No new result.
          ret = 0;
          break;
        default:  // Anything else is an error.
          break;
      }
    }
    if (_fwd_matched_afferants & (nullptr != _NXT)) {
      ret = _NXT->pushVector(s, data, error, seq_num);
    }
  }
  else if (_fwd_mismatches & (nullptr != _NXT)) {
    // We relay vectors of SpatialSense's we aren't configured for.
    ret = _NXT->pushVector(s, data, error, seq_num);
  }

  return ret;
}


void TripleAxisTimeSeries::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  bool was_dirty = dirty();
  StringBuilder indent;
  for (uint8_t i = 0; i < stage; i++) {    indent.concat("    ");    }
  indent.string();
  output->concat(&indent);
  output->concat("+-< 3AxisPipe: SingleFilter >-----------\n");
  output->concat(&indent);
  output->concatf("| SpatialSense:   %s\n", TripleAxisPipe::spatialSenseStr(_SENSE));
  if (verbosity > 5) {
    _print_series(output);
  }
  output->concat(&indent);
  Vector3f val = value();
  output->concatf("| Value %s:    (%.3f, %.3f, %.3f)\n", (was_dirty?"FRESH":"STALE"), (double) val.x, (double) val.y, (double) val.z);
  if (_has_error) {
    output->concat(&indent);
    output->concatf("| Error:          (%.3f, %.3f, %.3f)\n", (double) _ERR.x, (double) _ERR.y, (double) _ERR.z);
  }
  if ((verbosity > 0) && (nullptr != _NXT)) {
    _NXT->printPipe(output, stage+1, verbosity);
  }
}
