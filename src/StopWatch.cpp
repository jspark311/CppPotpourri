/*
File:   StopWatch.cpp
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

#include "CppPotpourri.h"
#include "TimerTools.h"
#include "StringBuilder.h"

/**
* Constructor
*/
StopWatch::StopWatch(uint32_t _t) : _tag(_t) {
  reset();
}


void StopWatch::reset() {
  _run_time_last    = 0;
  _run_time_best    = 0xFFFFFFFF;   // Need __something__ to compare against...
  _run_time_worst   = 0;
  _run_time_average = 0;
  _run_time_total   = 0;
  _executions       = 0;   // How many times has this task been used?
}


bool StopWatch::addRuntime(const uint32_t START_TIME, const uint32_t STOP_TIME) {
  _executions++;
  _run_time_last    = delta_assume_wrap(STOP_TIME, START_TIME);
  _run_time_best    = strict_min(_run_time_last, _run_time_best);
  _run_time_worst   = strict_max(_run_time_last, _run_time_worst);
  _run_time_total  += _run_time_last;
  _run_time_average = _run_time_total / _executions;
  _start_micros     = 0;
}


bool StopWatch::markStop() {
  const uint32_t STOP_TIME = micros();
  bool ret = false;
  if (_start_micros > 0) {
    addRuntime(_start_micros, STOP_TIME);
    ret = true;
  }
  return ret;
}


void StopWatch::printDebug(const char* label, StringBuilder* output) {
  if (_executions) {
    output->concatf("%14s %10u %10u %10u %10u %10u %10u\n",
      label,
      _executions,
      (unsigned long) _run_time_total,
      (unsigned long) _run_time_average,
      (unsigned long) _run_time_worst,
      (unsigned long) _run_time_best,
      (unsigned long) _run_time_last
    );
  }
  else {
    output->concatf("%14s <NO DATA>\n", label);
  }
}


void StopWatch::printDebugHeader(StringBuilder* output) {
  output->concat("          Name      Execd   total us    average      worst       best       last\n");
  output->concat("--------------------------------------------------------------------------------\n");
}



/*******************************************************************************
* C3PTrace
*******************************************************************************/

#if defined(CONFIG_C3P_TRACE_ENABLED)
// We were asked to include the profiler.
//   manage usage as single lines that aren't hard to read.
static C3PTrace global_profiler(CONFIG_C3P_TRACE_MAX_MEMORY);
static C3PTrace* C3PTrace::tracerTool() {  return &global_profiler;  }


C3PTrace::C3PTrace(const uint32_t MAX_POINTS) :
  _trace_points(MAX_POINTS),
  _recording_began(0), _recording_ended(0), _trace_count(0), _mode_oneshot(false) {}

C3PTrace::~C3PTrace() {
  while (_trace_paths.count() > 0) {  delete _trace_paths.dequeue();  }
}


/*
* This function is the ultimate intake for trace.
* TODO: Compiler attributes on this function.
*/
void C3PTrace::trace(const uint32_t TRACE_WORD) {
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
      case TraceAction::START_POINT:
      case TraceAction::STOP_POINT:
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
            if (TraceAction::START_POINT == ACTION) {
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


void C3PTrace::generateReport(StringBuilder* out, const TCode FORMAT) {
  switch (FORMAT) {
    case TCode::STR:
      break;
    case TCode::CBOR:
    default:   break;
  }
}



void C3PTrace::reset() {
  _trace_points.clear();
  const int PATH_COUNT = _trace_paths.count();
  for (int i = 0; i < PATH_COUNT; i++) {
    _trace_paths.get(i)->reset();
  }
}


bool C3PTrace::recording(bool en) {
  if (_trace_points.allocated()) {
    if (en) {
      _recording_began = micros();
      _recording_ended = 0;
    }
    else {
      _recording_ended = micros();
    }
  }
}


/*******************************************************************************
* TracePoint
*******************************************************************************/

void TracePoint::export(StringBuilder* out, const TCode FORMAT) {
  switch (FORMAT) {
    case TCode::STR:    out->concatf("F%u:L%u", fileID(), lineID());  break;
    case TCode::CBOR:
    default:   break;
  }
}


/*******************************************************************************
* TracePath
*******************************************************************************/

void TracePath::export(StringBuilder* out, const TCode FORMAT) {
  if (0 < _start_point.ts_micros) {
    switch (FORMAT) {
      case TCode::STR:
        {
          StringBuilder tmp("TracePath from ");
          _start_point.export(tmp, FORMAT);
          StringBuilder::styleHeader2(out, (char*) tmp.string());
          const int ENDPOINT_COUNT = _pathways.count();
          StopWatch::printDebugHeader(out);
          for (int i = 0; i < ENDPOINT_COUNT; i++) {
            StopWatch* path_profiler = _pathways.getByPriority((int32_t) POINT->trace_word);
            if (nullptr != path_profiler) {
              TracePoint tmp_point(0, path_profiler->tag());
              tmp.clear();
              tmp_point->printDebug(&tmp);
              path_profiler->printDebug((char*) tmp.string(), out);
            }
          }
        }
        break;
      case TCode::CBOR:
      default:   break;
    }
  }
}


bool TracePath::recordStart(const uint32_t TRACE_WORD, const TracePoint* POINT) {
  if (0 == _start_point->ts_micros) {
    _start_point(POINT);
    return true;
  }
  return false;
}


bool TracePath::recordStop(const uint32_t TRACE_WORD, const TracePoint* POINT) {
  bool ret = false;
  if (_start_point.ts_micros > 0) {
    StopWatch* path_profiler = _pathways.getByPriority((int32_t) POINT->trace_word);
    bool path_timing_created = false;
    if (nullptr == path_profiler) {
      path_profiler = new StopWatch(POINT->trace_word);
      path_timing_created = (nullptr != path_profiler);
    }
    if (nullptr != path_profiler) {
      ret = path_profiler->addRuntime((uint32_t) _ts_start, (uint32_t) POINT->ts_micros);
      if (path_timing_created) {
        ret &= (_pathways.insert(path_profiler, (int32_t) POINT->trace_word));
      }
    }
  }
  if (path_timing_created & !ret) {  // Clean up any mess we might have left
    delete path_profiler;            // hanging after an allocation failure.
  }
  _start_point.ts_micros = 0;
  return ret;
}


#endif  // CONFIG_C3P_TRACE_ENABLED
