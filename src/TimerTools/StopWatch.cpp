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
#include "../Meta/Rationalizer.h"
#include "../C3PValue/KeyValuePair.h"
#include "TimerTools.h"


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


/*
* This function allows the caller to add both a start and a stop time from
*   outside timer measurements. This may be desirable for high-accuracy
*   use-cases in which collection-points must be controlled for carefully (such
*   as in C3PTrace).
*/
ISR_FUNC bool StopWatch::addRuntime(const unsigned long START_TIME, const unsigned long STOP_TIME) {
  _executions++;
  _run_time_last    = delta_assume_wrap((uint32_t) STOP_TIME, (uint32_t) START_TIME);
  _run_time_best    = strict_min(_run_time_last, _run_time_best);
  _run_time_worst   = strict_max(_run_time_last, _run_time_worst);
  _run_time_total  += _run_time_last;
  _run_time_average = _run_time_total / _executions;
  _start_micros     = 0;
  return true;
}


ISR_FUNC bool StopWatch::markStop() {
  const uint32_t STOP_TIME = micros();
  bool ret = false;
  if (_start_micros > 0) {
    addRuntime(_start_micros, STOP_TIME);
    ret = true;
  }
  return ret;
}


void StopWatch::printDebug(const char* label, StringBuilder* out) {
  C3PType* t_helper = getTypeHelper(TCode::STOPWATCH);
  if (nullptr != t_helper) {
    out->concatf("%14s ", label);
    t_helper->serialize(this, out, TCode::STR);
  }
}


void StopWatch::printDebugHeader(StringBuilder* out) {
  out->concat("          Name      Execd   total us    average      worst       best       last\n");
  out->concat("--------------------------------------------------------------------------------\n");
}





template <> int C3PTypeConstraint<StopWatch*>::serialize(void* _obj, StringBuilder* out, const TCode FORMAT) {
  int ret = -1;
  StopWatch* obj = ((StopWatch*) _obj);
  if (nullptr == _obj) {  return ret;  }

  switch (FORMAT) {
    case TCode::STR:
      if (obj->_executions) {
        out->concatf("%10u %10u %10u %10u %10u %10u\n",
          obj->_executions,
          (unsigned long) obj->_run_time_total,
          (unsigned long) obj->_run_time_average,
          (unsigned long) obj->_run_time_worst,
          (unsigned long) obj->_run_time_best,
          (unsigned long) obj->_run_time_last
        );
      }
      else {
        out->concat("<NO DATA>\n");
      }
      ret = 0;
      break;

    case TCode::BINARY:
      break;

    #if defined(__BUILD_HAS_CBOR)
    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_tag(C3P_CBOR_VENDOR_CODE | TcodeToInt(TCODE));
        if (0 != obj->_tag) {
          encoder.write_map(7);
          encoder.write_string("g");    encoder.write_int(obj->_tag);
        }
        else {
          encoder.write_map(6);
        }
        encoder.write_string("e");   encoder.write_int(obj->_executions);
        encoder.write_string("t");   encoder.write_int(obj->_run_time_total);
        encoder.write_string("a");   encoder.write_int(obj->_run_time_average);
        encoder.write_string("w");   encoder.write_int(obj->_run_time_worst);
        encoder.write_string("b");   encoder.write_int(obj->_run_time_best);
        encoder.write_string("l");   encoder.write_int(obj->_run_time_last);
        ret = 0;
      }
      break;
      #endif  // __BUILD_HAS_CBOR

    default:  break;
  }
  return ret;
}


template <> int8_t C3PTypeConstraint<StopWatch*>::construct(void* _obj, KeyValuePair* kvp) {
  int8_t ret = -1;
  if ((nullptr != _obj) & (nullptr != kvp)) {
    ret--;
    StopWatch* obj = *((StopWatch**) _obj);
    if (nullptr == obj) {
      obj = new StopWatch();       // Allocate, if necessary.
      *((StopWatch**) _obj) = obj; // And assign.
    }
    if (nullptr != obj) {
      const unsigned int KVP_COUNT = kvp->count();
      for (unsigned int i = 0; i < KVP_COUNT; i++) {
        KeyValuePair* current_kvp = kvp->retrieveByIdx(i);
       char* current_key = current_kvp->getKey();
       if (0 == StringBuilder::strcasecmp(current_key, "g")) {       current_kvp->getValue(&(obj->_tag));               }
       else if (0 == StringBuilder::strcasecmp(current_key, "e")) {  current_kvp->getValue(&(obj->_executions));        }
       else if (0 == StringBuilder::strcasecmp(current_key, "t")) {  current_kvp->getValue(&(obj->_run_time_total));    }
       else if (0 == StringBuilder::strcasecmp(current_key, "a")) {  current_kvp->getValue(&(obj->_run_time_average));  }
       else if (0 == StringBuilder::strcasecmp(current_key, "w")) {  current_kvp->getValue(&(obj->_run_time_worst));    }
       else if (0 == StringBuilder::strcasecmp(current_key, "b")) {  current_kvp->getValue(&(obj->_run_time_best));     }
       else if (0 == StringBuilder::strcasecmp(current_key, "l")) {  current_kvp->getValue(&(obj->_run_time_last));     }
      }
      ret = 0;   // StopWatch always succeeds. No required keys.
    }
  }
  return ret;
}
