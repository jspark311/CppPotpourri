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

#define TAP_INDENT_STRING "    "


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
* TripleAxisPipeWithEfferent
*******************************************************************************/

void TripleAxisPipeWithEfferent::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  if (verbosity > 0) {
    StringBuilder indent;
    for (uint8_t i = 0; i < stage; i++) {    indent.concat(TAP_INDENT_STRING);    }
    indent.string();
    _print_pipe(output, &indent, verbosity);
    indent.clear();
    if (nullptr != _NXT) {
      _NXT->printPipe(output, (stage+1), verbosity);
    }
  }
}


/*******************************************************************************
* TripleAxisSenseFilter
*******************************************************************************/

/*
* Behavior: If sense parameter matches the local class sense, refreshes
*   this instance's state and calls callback, if defined. Marks the data
*   as fresh if the callback is either absent, or returns nonzero.
*
* @return -2 on push failure, -1 on broken pipe, or 0 on success.
*/
FAST_FUNC int8_t TripleAxisSenseFilter::pushVector(const SpatialSense s, Vector3f* data, Vector3f* error, uint32_t seq_num) {
  int8_t ret = -1;   // Defaults to a broken pipe error.
  if (_SENSE == s) {
    ret = _filtered_push(data, error, seq_num);
    if (_fwd_matched_afferents & (nullptr != _NXT)) {
      ret = _NXT->pushVector(s, data, error, seq_num);
    }
  }
  else if (_fwd_mismatches & (nullptr != _NXT)) {
    // We relay vectors of SpatialSense's we aren't configured for.
    ret = _NXT->pushVector(s, data, error, seq_num);
  }
  return ret;
}


/*
* Middle implementation to print the filter and forward behavior.
*/
void TripleAxisSenseFilter::_print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity) {
  output->concat(indent);
  output->concatf("| SpatialSense:   %s (forwards ", TripleAxisPipe::spatialSenseStr(_SENSE));
  uint8_t aff_sel = (_fwd_matched_afferents ? 1 : 0);
  aff_sel += (_fwd_mismatches ? 2 : 0);
  switch (aff_sel) {
    case 0:  output->concat("no");           break;
    case 1:  output->concat("matching");     break;
    case 2:  output->concat("mismatched");   break;
    case 3:  output->concat("all");          break;
  }
  output->concat(" afferents)\n");
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
}


void TripleAxisFork::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  StringBuilder indent;
  for (uint8_t i = 0; i < stage; i++) {    indent.concat(TAP_INDENT_STRING);    }
  indent.string();
  output->concatHandoff(&indent);
  output->concat("+-< 3AxisPipe: Fork >-------------\n");
  if (nullptr != _LEFT) {
    _LEFT->printPipe(output, (stage+1), verbosity);
  }
  if (nullptr != _RIGHT) {
    _RIGHT->printPipe(output, (stage+1), verbosity);
  }
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


void TripleAxisRemapper::_print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity) {
  output->concat(indent);
  output->concat("+-< 3AxisPipe: Remapper >-------------\n");
}


bool TripleAxisRemapper::mapAfferent(
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
* TripleAxisTerminalCallback
*******************************************************************************/
FAST_FUNC int8_t TripleAxisTerminalCallback::pushVector(const SpatialSense s, Vector3f* data, Vector3f* error, uint32_t seq_num) {
  return ((nullptr == _CALLBACK) ? -2 : _CALLBACK(s, data, error, seq_num));
}

void TripleAxisTerminalCallback::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  StringBuilder indent;
  for (uint8_t i = 0; i < stage; i++) {    indent.concat(TAP_INDENT_STRING);    }
  indent.string();
  output->concatHandoff(&indent);
  output->concat("+-< 3AxisPipe: TerminalCallback >-------------\n");
}


/*******************************************************************************
* TripleAxisStorage
*******************************************************************************/

/*
* Atomic accessor with freshness management and return indication.
*
* @return 0 on success with stale data.
* @return 1 on success with fresh data.
*/
int8_t TripleAxisStorage::getDataWithErr(Vector3f* d, Vector3f* e, uint32_t* c) {
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


/**
* Behavior: Refreshes this instance's state. Marks the data as fresh.
* Does not push an efferent, since the base implementation does no transform on
*   the afferent, and the superclass will already forwardOnMatch() if desired.
*
* NOTE: This is the most-basal implementations of _filtered_push(), and the only
*   reason it is not pure virtual is to allow direct-instantiation of this
*   class. _filtered_push() does not actually push an efferant. All it does is
*   store the given vector/error/seq and updates class state to reflect new data.
* This is easy enough to do with repeat code in a child class, but it is also
*   acceptable to "call super" in this case.
*
* @return 0 always.
*/
FAST_FUNC int8_t TripleAxisStorage::_filtered_push(Vector3f* data, Vector3f* error, uint32_t seq_num) {
  _DATA.set(data);
  if (nullptr != error) {
    _ERR.set(error);
    _has_error = true;
  }
  _last_update = seq_num;
  _update_count++;
  _fresh_data = true;
  return 0;
}


/*
* Resets the class to zero.
*/
void TripleAxisStorage::reset() {
  _has_error    = false;
  _fresh_data   = false;
  _last_update  = 0;
  _update_count = 0;
  _DATA.set(0.0, 0.0, 0.0);
  _ERR.set(0.0, 0.0, 0.0);
}


void TripleAxisStorage::_print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity) {
  output->concat(indent);
  output->concatf("+-< 3AxisPipe: Storage >---------------\n");
  TripleAxisSenseFilter::_print_pipe(output, indent, verbosity);
  output->concat(indent);
  output->concatf("| Seq number:     %u\n", _update_count);
  output->concat(indent);
  output->concatf("| Last update:    %u\n", _last_update);
  output->concat(indent);
  output->concatf("| Value %s:    (%.3f, %.3f, %.3f)\n", (_fresh_data?"FRESH":"STALE"), (double) _DATA.x, (double) _DATA.y, (double) _DATA.z);
  if (_has_error) {
    output->concat(indent);
    output->concatf("| Error:          (%.3f, %.3f, %.3f)\n", (double) _ERR.x, (double) _ERR.y, (double) _ERR.z);
  }
}



/*******************************************************************************
* TripleAxisIntegrator
*******************************************************************************/
/*
* @return -2 on push failure, -1 on broken pipe, or 0 on success.
*/
FAST_FUNC int8_t TripleAxisIntegrator::_filtered_push(Vector3f* data, Vector3f* error, uint32_t seq_num) {
  int8_t ret = 0;
  Vector3f  local_dat = (_DATA + (*data));
  Vector3f* local_err = error;
  if (nullptr != error) {
    _ERR  += (*error);
    local_err = &_ERR;
  }

  // Store the result in the Storage class.
  TripleAxisStorage::_filtered_push(&local_dat, local_err, seq_num);

  if (nullptr != _NXT) {
    ret = _NXT->pushVector(_SENSE, &_DATA, &_ERR, _update_count);
  }
  return ret;
}


void TripleAxisIntegrator::_print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity) {
  output->concat(indent);
  output->concat("+-< 3AxisPipe: Integrator >-----------\n");
  TripleAxisStorage::_print_pipe(output, indent, verbosity);
}



/*******************************************************************************
* TripleAxisDifferentiator
*******************************************************************************/
/*
* @return -2 on push failure, -1 on broken pipe, or 0 on success.
*/
FAST_FUNC int8_t TripleAxisDifferentiator::_filtered_push(Vector3f* data, Vector3f* error, uint32_t seq_num) {
  int8_t ret = 0;

  if (_primed) {
    Vector3f  local_dat = (_DATA + (*data));
    Vector3f* local_err = error;
    if (nullptr != error) {
      // Error is doubled because: two datapoints produced the efferent.
      // TODO: Open your engineering textbooks and review error propagation through
      //   differentiation. 2x the input sounds logical, but there is a splinter in
      //   my brain that it is not correct. Verify.
      _ERR  = ((*error) * 2);
      local_err = &_ERR;
    }
    local_dat = ((*data) - _prior_afferant);
    // Store the result in the Storage class.
    TripleAxisStorage::_filtered_push(&local_dat, local_err, seq_num);

    if (nullptr != _NXT) {
      ret = _NXT->pushVector(_SENSE, &_DATA, &_ERR, _update_count);
    }
  }
  else {
    _primed = true;
  }
  _prior_afferant.set(data);
  return ret;
}


void TripleAxisDifferentiator::_print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity) {
  output->concat(indent);
  output->concat("+-< 3AxisPipe: Differentiator >-----------\n");
  TripleAxisStorage::_print_pipe(output, indent, verbosity);
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
FAST_FUNC int8_t TripleAxisTimeSeries::_filtered_push(Vector3f* data, Vector3f* error, uint32_t seq_num) {
  int8_t ret = -1;
  if (!initialized()) {
    init();  // Init memory on first-use. See class notes.
  }
  ret = -2;
  if (initialized()) {
    switch (feedSeries(data)) {
      case 1:    // Value accepted into filter. New result ready.
        if (nullptr != error) {
          // We will retain the afferent error figure. Adjusting this figure for
          //   efferent push() will be done at that time. Doing it this way will
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
  return ret;
}


void TripleAxisTimeSeries::_print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity) {
  bool was_dirty = dirty();
  output->concat(indent);
  output->concat("+-< 3AxisPipe: TimeSeries >-----------\n");
  TripleAxisSenseFilter::_print_pipe(output, indent, verbosity);
  if (verbosity > 5) {
    _print_series(output);
  }
  Vector3f val = value();
  output->concat(indent);
  output->concatf("| Value %s:    (%.3f, %.3f, %.3f)\n", (was_dirty?"FRESH":"STALE"), (double) val.x, (double) val.y, (double) val.z);
  if (_has_error) {
    output->concat(indent);
    output->concatf("| Error:          (%.3f, %.3f, %.3f)\n", (double) _ERR.x, (double) _ERR.y, (double) _ERR.z);
  }
}


/*******************************************************************************
* TripleAxisScaling
*******************************************************************************/

/*
* Scaling changes the error vector in a linear manner.
*
* @return -2 on push failure, -1 on broken pipe, or 0 on success.
*/
FAST_FUNC int8_t TripleAxisScaling::pushVector(const SpatialSense s, Vector3f* data, Vector3f* error, uint32_t seq_num) {
  int8_t ret = -1;
  if (nullptr != _NXT) {
    Vector3<float> result_err;
    Vector3<float> result = *(data);
    const float LEN_INPUT = result.length();
    const bool HAVE_ERROR = ((nullptr != error) && (LEN_INPUT != 0.0f));
    if (_scaling_vector.isZero()) {
      if (HAVE_ERROR) {
        // When normalized, the error must be scaled by the length difference
        //   following normalization.
        const float LEN_SCALE = (1.0f/LEN_INPUT);
        result_err = *(error) * LEN_SCALE;
      }
      result.normalize(LEN_INPUT);
    }
    else {
      result.x = result.x * _scaling_vector.x;
      result.y = result.y * _scaling_vector.y;
      result.z = result.z * _scaling_vector.z;
      if (HAVE_ERROR) {
        result_err.x = result_err.x * _scaling_vector.x;
        result_err.y = result_err.y * _scaling_vector.y;
        result_err.z = result_err.z * _scaling_vector.z;
      }
    }
    ret = _NXT->pushVector(s, &result, ((nullptr == error) ? nullptr : &result_err), seq_num);
  }
  return ret;
}


void TripleAxisScaling::_print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity) {
  output->concat(indent);
  output->concat("+-< 3AxisPipe: Scaling >-----------\n");
  output->concat(indent);
  output->concatf("| Scaling:  (%.3f, %.3f, %.3f)\n", (double) _scaling_vector.x, (double) _scaling_vector.y, (double) _scaling_vector.z);
}



/*******************************************************************************
* TripleAxisOffset
*******************************************************************************/
/*
* Offset doesn't change the error vector.
*
* @return -2 on push failure, -1 on broken pipe, or 0 on success.
*/
FAST_FUNC int8_t TripleAxisOffset::pushVector(const SpatialSense s, Vector3f* data, Vector3f* error, uint32_t seq_num) {
  int8_t ret = -1;
  if (nullptr != _NXT) {
    Vector3<float> result = *(data) + _offset_vector;
    ret = _NXT->pushVector(s, &result, error, seq_num);
  }
  return ret;
}


void TripleAxisOffset::_print_pipe(StringBuilder* output, StringBuilder* indent, uint8_t verbosity) {
  output->concat(indent);
  output->concat("+-< 3AxisPipe: Offset >-----------\n");
  output->concat(indent);
  output->concatf("| Offset:   (%.3f, %.3f, %.3f)\n", (double) _offset_vector.x, (double) _offset_vector.y, (double) _offset_vector.z);
}
