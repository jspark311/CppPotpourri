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
* @return 0 on sucess on both sides of the fork, -1 on one failure, or -2 on two failures.
*/
FAST_FUNC int8_t TripleAxisFork::pushVector(SpatialSense s, Vector3f* data, Vector3f* error) {
  int8_t ret = -2;
  if ((nullptr != _LEFT) && (0 == _LEFT->pushVector(s, data, error))) {
    ret++;
  }
  if ((nullptr != _RIGHT) && (0 == _RIGHT->pushVector(s, data, error))) {
    ret++;
  }
  return ret;
};


void TripleAxisFork::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  StringBuilder indent;
  for (uint8_t i = 0; i < stage; i++) {    indent.concat("    ");    }
  output->concatf("%s+-< 3AxisPipe: Fork >-------------------\n", (char*) indent.string());
  indent.clear();
  if ((verbosity > 0) && (nullptr != _LEFT)) {
    _LEFT->printPipe(output, stage+1, verbosity);
  }
  if ((verbosity > 0) && (nullptr != _RIGHT)) {
    _RIGHT->printPipe(output, stage+1, verbosity);
  }
}



/*******************************************************************************
* TripleAxisConvention
*******************************************************************************/

/*
* Behavior: Refreshes this instance's state and calls callback, if defined.
* Marks the data as fresh if the callback is either absent, or returns nonzero.
*
* @return -2 on null NXT, or return code from downstream pushVector() fxn.
*/
FAST_FUNC int8_t TripleAxisConvention::pushVector(SpatialSense s, Vector3f* data, Vector3f* error) {
  Vector3f stacked_dat;
  Vector3f stacked_err;
  float inversion = 1.0;
  int8_t ret = -2;
  if (nullptr != _NXT) {
    ret++;
    switch (_SRC_FMT) {
      case GnomonType::RH_NEG_X:
      case GnomonType::LH_POS_X:
        inversion = -1.0;
      case GnomonType::RH_POS_X:
      case GnomonType::LH_NEG_X:
      case GnomonType::UNDEFINED:  // Undefinition results in no transform.
        stacked_dat(inversion * data->x, data->y, data->z);
        if (nullptr != error) {
          stacked_err(inversion * error->x, error->y, error->z);
        }
        break;
      case GnomonType::RH_NEG_Y:
      case GnomonType::LH_POS_Y:
        inversion = -1.0;
      case GnomonType::RH_POS_Y:
      case GnomonType::LH_NEG_Y:
        stacked_dat(inversion * data->y, data->z, data->x);
        if (nullptr != error) {
          stacked_err(inversion * error->y, error->z, error->x);
        }
        break;
      case GnomonType::RH_NEG_Z:
      case GnomonType::LH_POS_Z:
        inversion = -1.0;
      case GnomonType::RH_POS_Z:
      case GnomonType::LH_NEG_Z:
        stacked_dat(inversion * data->z, data->x, data->y);
        if (nullptr != error) {
          stacked_err(inversion * error->z, error->x, error->y);
        }
        break;
    }
    if (0 == _NXT->pushVector(s, &stacked_dat, (nullptr != error) ? &stacked_err : nullptr)) {
      ret = 0;
    }
  }
  return ret;
}


void TripleAxisConvention::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  StringBuilder indent;
  for (uint8_t i = 0; i < stage; i++) {    indent.concat("    ");    }
  output->concatf("%s+-< 3AxisPipe: Convention >-------------\n", (char*) indent.string());
  indent.clear();
  if ((verbosity > 0) && (nullptr != _NXT)) {
    _NXT->printPipe(output, stage+1, verbosity);
  }
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
FAST_FUNC int8_t TripleAxisTerminus::pushVector(SpatialSense s, Vector3f* data, Vector3f* error) {
  int8_t ret = -1;
  if (_SENSE == s) {
    _last_update = millis();
    _update_count++;
    _DATA.set(data);
    if (nullptr != error) {
      _ERR.set(error);
      _has_error = true;
    }
    if (nullptr != _CALLBACK) {
      _fresh_data = (0 != _CALLBACK(_SENSE, &_DATA, (_has_error ? &_ERR : nullptr), _update_count));
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
  output->concatf("%s+-< 3AxisPipe: Terminus >---------------\n", (char*) indent.string());
  output->concatf("%s| Has callback:   %c\n", (char*) indent.string(), (nullptr != _CALLBACK)?'y':'n');
  output->concatf("%s| Seq number:     %u\n", (char*) indent.string(), _update_count);
  output->concatf("%s| SpatialSense:   %s\n", (char*) indent.string(), TripleAxisPipe::spatialSenseStr(_SENSE));
  output->concatf("%s| Value %s:    (%.3f, %.3f, %.3f)\n", (char*) indent.string(), (_fresh_data?"FRESH":"STALE"), (double) _DATA.x, (double) _DATA.y, (double) _DATA.z);
  if (_has_error) {
    output->concatf("%s| Error:          (%.3f, %.3f, %.3f)\n", (char*) indent.string(), (double) _ERR.x, (double) _ERR.y, (double) _ERR.z);
  }
  output->concatf("%s| Last update:    %u\n", (char*) indent.string(), _last_update);
}


/*******************************************************************************
* TripleAxisSingleFilter
*******************************************************************************/


/*
* Atomic accessor with freshness management and return indication.
*
* @return 0 on success with stale data.
* @return 1 on success with fresh data.
*/
int8_t TripleAxisSingleFilter::getDataWithErr(Vector3f* d, Vector3f* e) {
  int8_t ret = dirty() ? 1 : 0;
  d->set(value());
  if (nullptr != e) {
    // TODO: This class might do extensive massaging of data, and it will certainly
    //   impact not only the value of the error, but also what the value _means_.
    //   Carefully review this for accurate processing once things are working.
    e->set(&_ERR);
  }
  return ret;
}



/*
* Behavior: If sense parameter matches the local class sense, refreshes
*   this instance's state and calls callback, if defined. Marks the data
*   as fresh if the callback is either absent, or returns nonzero.
*
* @return 0 on refresh, or -1 on sense mis-match, -3 on uninitialized.
*/
FAST_FUNC int8_t TripleAxisSingleFilter::pushVector(SpatialSense s, Vector3f* data, Vector3f* error) {
  int8_t ret = -3;

  if (!initialized()) {
    init();  // Init memory on first-use. See class notes.
  }
  if (initialized()) {
    ret = -1;
    if (_SENSE == s) {
      Vector3f tmp_err(error);
      ret = 0;
      switch (feedFilter(data)) {
        case 0:    // Value accepted into filter. No new result.
          break;
        case 1:    // Value accepted into filter. New result ready.
          if (nullptr != error) {
            _has_error = true;
            switch (strategy()) {
              case FilteringStrategy::RAW:
                _ERR.set(&tmp_err);
                break;
              case FilteringStrategy::MOVING_AVG:
              case FilteringStrategy::HARMONIC_MEAN:    // TODO: Still epistemologically correct?
              case FilteringStrategy::GEOMETRIC_MEAN:   // TODO: Still epistemologically correct?
                // Arithmetic mean applies the inverse square of the variance to the given error figure.
                // TODO: Assumes error is constant between samples. If it isn't,
                //   the true error figure of the data will converge to the value
                //   reported to the filter. If the error figure changes from
                //   upstream, it might be best to clear the filter, rather than
                //   allow epistemologically unsound data to pass.
                // NOTE: Assumes error represents an even distribution with no
                //   correlation between samples. I believe that this would make
                //   the reported error figure more conservative than may be
                //   warranted from a given source.
                tmp_err /= (float) (windowSize() * windowSize());
                _ERR.set(&tmp_err);
                break;
              default:    // TODO: How to account for the other cases? Do something obviously wrong for now.
                _ERR.set(error);   // Wrong.
                break;
            }
          }
          if (nullptr != _NXT) {
            // If the vector was pushed downstream, we consider it noted. If it
            //   was rejected, we leave the class marked dirty.
            _filter_dirty = (0 > _NXT->pushVector(s, value(), &_ERR));
          }
          break;
        default:   // Catch-all error case. Should never happen with the earlier init().
          ret = -4;
          break;
      }
    }
    else {
      // We blindly relay vectors of SpatialSense's we aren't configured for.
      if (0 == _NXT->pushVector(s, data, error)) {
        ret = -5;
      }
    }
  }
  return ret;
}


void TripleAxisSingleFilter::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  bool was_dirty = dirty();
  StringBuilder indent;
  for (uint8_t i = 0; i < stage; i++) {    indent.concat("    ");    }
  output->concatf("%s+-< 3AxisPipe: SingleFilter >-----------\n", (char*) indent.string());
  output->concatf("%s| SpatialSense:   %s\n", (char*) indent.string(), TripleAxisPipe::spatialSenseStr(_SENSE));
  if (verbosity > 5) {
    SensorFilter3<float>::printFilter(output);
  }
  output->concatf("%s| Value %s:    (%.3f, %.3f, %.3f)\n", (char*) indent.string(), (was_dirty?"FRESH":"STALE"), (double) value()->x, (double) value()->y, (double) value()->z);
  if (_has_error) {
    output->concatf("%s| Error:          (%.3f, %.3f, %.3f)\n", (char*) indent.string(), (double) _ERR.x, (double) _ERR.y, (double) _ERR.z);
  }
  if ((verbosity > 0) && (nullptr != _NXT)) {
    _NXT->printPipe(output, stage+1, verbosity);
  }
}
