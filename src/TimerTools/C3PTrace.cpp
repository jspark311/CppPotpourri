/*
File:   C3PTrace.cpp
Author: J. Ian Lindsay
Date:   2016.03.11

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

#include "../CppPotpourri.h"
#include "../StringBuilder.h"
#include "TimerTools.h"
#include "C3PTrace.h"
#include "../cbor-cpp/cbor.h"



#if defined(CONFIG_C3P_TRACE_ENABLED)
// Trace is a singleton capability.
static C3PTrace global_profiler(CONFIG_C3P_TRACE_MAX_POINTS);
C3PTrace* C3PTrace::tracerTool = &global_profiler;


/*******************************************************************************
* C3PTrace
*******************************************************************************/

C3PTrace::C3PTrace(const uint32_t MAX_POINTS) :
  _recording_began(0), _recording_ended(0), _trace_count(0), _mode_oneshot(false),
  _trace_points(MAX_POINTS) {}

C3PTrace::~C3PTrace() {
  while (_trace_paths.size() > 0) {  delete _trace_paths.dequeue();  }
}


/*
* This function is the ultimate intake for trace.
*/
ISR_FUNC void C3PTrace::leave_trace(const uint32_t TRACE_WORD) {
  const unsigned int NOW = micros();
  if (recording()) {
    const uint32_t SPATIAL_WORD = (C3P_TRACE_WORD_SPATIAL_MASK & TRACE_WORD);
    const TraceAction ACTION = (const TraceAction) (TRACE_WORD & C3P_TRACE_WORD_SPATIAL_MASK);
    if (0 == _trace_points.vacancy()) {
      _trace_points.get();
    }
    TracePoint tmp_point(NOW, SPATIAL_WORD);
    _trace_points.insert(tmp_point);
    switch (ACTION) {
      case TraceAction::PATH_START:
      case TraceAction::PATH_STOP:
        {
          const uint8_t  PATH_ID    = (uint8_t) ((TRACE_WORD & C3P_TRACE_WORD_SPATIAL_MASK) >> C3P_TRACE_WORD_PATH_OFFSET);
          bool path_root_created = false;
          TracePath* path_container = _trace_paths.getByPriority((int32_t) PATH_ID);
          if (nullptr == path_container) {
            path_container = new TracePath(PATH_ID);
            path_root_created = (nullptr != path_container);
          }
          if (nullptr != path_container) {
            if (path_root_created) {
              _trace_paths.insert(path_container, (int32_t) PATH_ID);
            }
            if (TraceAction::PATH_START == ACTION) {
              path_container->recordStart(SPATIAL_WORD, (const TracePoint*) &tmp_point);
            }
            else {
              path_container->recordStop(SPATIAL_WORD, (const TracePoint*) &tmp_point);
            }
          }
        }
        break;
      default:
        break;
    }
  }
}


void C3PTrace::serialize(StringBuilder* out, const TCode FORMAT) {
  switch (FORMAT) {
    case TCode::STR:
      for (unsigned int i = 0; i < _trace_points.count(); i++) {
        TracePoint point = _trace_points.peek(i);
        point.serialize(out, FORMAT);
        out->concat('\n');
      }
      for (int i = 0; i < _trace_paths.size(); i++) {
        TracePath* path = _trace_paths.get(i);
        if (nullptr != path) {
          path->serialize(out, FORMAT);
          //out->concat('\n');
        }
      }
      break;
    // TODO: One of these is required for best storage density.
    case TCode::CBOR:
      break;
    case TCode::BINARY:
      break;
    default:   break;
  }
}



void C3PTrace::reset() {
  _trace_points.clear();
  const int PATH_COUNT = _trace_paths.size();
  for (int i = 0; i < PATH_COUNT; i++) {
    _trace_paths.get(i)->reset();
  }
}


ISR_FUNC bool C3PTrace::recording(bool en) {
  if (_trace_points.allocated()) {
    if (en) {
      _recording_began = micros();
       if (0 == _recording_began) {
         _recording_began++;  // Disallow zero in this field.
       }
      _recording_ended = 0;
    }
    else {
      _recording_ended = micros();
       if (0 == _recording_ended) {
         _recording_ended++;  // Disallow zero in this field.
       }
    }
  }
  return true;
}


/*******************************************************************************
* TracePoint
*******************************************************************************/

int TracePoint::serialize(StringBuilder* out, const TCode FORMAT) {
  int ret = -1;
  switch (FORMAT) {
    case TCode::STR:    out->concatf("F%03u-L%05u:\t%u", fileID(), lineID(), ts_micros);  break;
    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_map(3);
        encoder.write_string("F");    encoder.write_int(fileID());
        encoder.write_string("L");    encoder.write_int(lineID());
        encoder.write_string("T");    encoder.write_int(ts_micros);
        ret = 0;
      }
      break;
    case TCode::BINARY:
      break;
    default:   break;
  }
  return ret;
}


/*******************************************************************************
* TracePath
*******************************************************************************/

int TracePath::serialize(StringBuilder* out, const TCode FORMAT) {
  int ret = -1;
  if (0 < _start_point.ts_micros) {
    const int ENDPOINT_COUNT = _pathways.size();
    switch (FORMAT) {
      case TCode::STR:
        {
          StringBuilder tmp("TracePath from ");
          _start_point.serialize(&tmp, FORMAT);
          StringBuilder::styleHeader2(out, (char*) tmp.string());
          StopWatch::printDebugHeader(out);
          for (int i = 0; i < ENDPOINT_COUNT; i++) {
            StopWatch* path_stopwatch = _pathways.getByPriority((int32_t) _start_point.trace_word);
            if (nullptr != path_stopwatch) {
              TracePoint end_point(0, path_stopwatch->tag());
              tmp.clear();
              //tmp_point.export_point(&tmp);
              tmp.concatf("F%03u-L%05u", end_point.fileID(), end_point.lineID());
              path_stopwatch->printDebug((char*) tmp.string(), out);
            }
          }
        }
        break;
      case TCode::CBOR:
        {
          cbor::output_stringbuilder output(out);
          cbor::encoder encoder(output);
          encoder.write_map(2);
          encoder.write_string("start");   encoder.write_int(_start_point.trace_word);
          encoder.write_string("stops");   encoder.write_array(ENDPOINT_COUNT);
          for (int i = 0; i < ENDPOINT_COUNT; i++) {
            StopWatch* path_stopwatch = _pathways.get(i);
            if (nullptr != path_stopwatch) {
              TracePoint end_point(0, path_stopwatch->tag());  // Reconstruct a point from the StopWatch tag.
              encoder.write_map(2);
              encoder.write_string("pnt");    encoder.write_int(end_point.trace_word);
              encoder.write_string("prof");   path_stopwatch->serialize(out, FORMAT);
            }
          }
          ret = 0;
        }
        break;
      case TCode::BINARY:
        break;
      default:   break;
    }
  }
  return ret;
}


ISR_FUNC bool TracePath::recordStart(const uint32_t TRACE_WORD, const TracePoint* POINT) {
  if (0 == _start_point.ts_micros) {
    _start_point.ts_micros = POINT->ts_micros;
    _start_point.trace_word = POINT->trace_word;
    return true;
  }
  return false;
}


ISR_FUNC bool TracePath::recordStop(const uint32_t TRACE_WORD, const TracePoint* POINT) {
  bool ret = false;
  if (_start_point.ts_micros > 0) {
    StopWatch* path_stopwatch = _pathways.getByPriority((int32_t) POINT->trace_word);
    bool path_timing_created  = false;
    if (nullptr == path_stopwatch) {
      path_stopwatch = new StopWatch(POINT->trace_word);
      path_timing_created = (nullptr != path_stopwatch);
    }
    if (nullptr != path_stopwatch) {
      ret = path_stopwatch->addRuntime((uint32_t) _start_point.ts_micros, (uint32_t) POINT->ts_micros);
      if (path_timing_created) {
        ret &= (_pathways.insert(path_stopwatch, (int32_t) POINT->trace_word));
      }
    }
    if (path_timing_created & !ret) {  // Clean up any mess we might have left
      delete path_stopwatch;            // hanging after an allocation failure.
    }
  }
  _start_point.ts_micros = 0;
  return ret;
}


#endif  // CONFIG_C3P_TRACE_ENABLED
