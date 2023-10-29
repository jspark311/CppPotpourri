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

#include "../CppPotpourri.h"
#include "../StringBuilder.h"
#include "TimerTools.h"
#include "../cbor-cpp/cbor.h"


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
  return true;
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


void StopWatch::printDebug(const char* label, StringBuilder* out) {
  out->concatf("%14s ", label);
  serialize(out, TCode::STR);
}


void StopWatch::printDebugHeader(StringBuilder* out) {
  out->concat("          Name      Execd   total us    average      worst       best       last\n");
  out->concat("--------------------------------------------------------------------------------\n");
}


int StopWatch::serialize(StringBuilder* out, const TCode FORMAT) {
  int ret = -1;
  switch (FORMAT) {
    case TCode::STR:
      if (_executions) {
        out->concatf("%10u %10u %10u %10u %10u %10u\n",
          _executions,
          (unsigned long) _run_time_total,
          (unsigned long) _run_time_average,
          (unsigned long) _run_time_worst,
          (unsigned long) _run_time_best,
          (unsigned long) _run_time_last
        );
      }
      else {
        out->concat("<NO DATA>\n");
      }
      break;
    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_map(6);
        encoder.write_string("exec");   encoder.write_int(_executions);
        encoder.write_string("tot");    encoder.write_int(_run_time_total);
        encoder.write_string("avg");    encoder.write_int(_run_time_average);
        encoder.write_string("worst");  encoder.write_int(_run_time_worst);
        encoder.write_string("best");   encoder.write_int(_run_time_best);
        encoder.write_string("last");   encoder.write_int(_run_time_last);
        ret = 0;
      }
      break;
    case TCode::BINARY:
      break;
    default:   break;
  }
  return ret;
}
