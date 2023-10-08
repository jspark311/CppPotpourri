/*
File:   SensorWrapper.cpp
Author: J. Ian Lindsay
Date:   2013.11.28

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

#include "SensorWrapper.h"

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
const char* const C3PSensor::callbackCodeStr(const SensorCallbackCode E) {
 switch (E) {
   case SensorCallbackCode::SAY_HELLO:       return "SAY_HELLO";
   case SensorCallbackCode::DATA_UPDATED:    return "DATA_UPDATED";
   case SensorCallbackCode::CONF_CHANGE:     return "CONF_CHANGE";
   case SensorCallbackCode::SAY_GOODBYE:     return "SAY_GOODBYE";
   case SensorCallbackCode::HARDWARE_FAULT:  return "HARDWARE_FAULT";
   default:     break;
 }
 return "UNSPECIFIED";
}


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
/* Destructor */
C3PSensor::~C3PSensor() {
  // Unregisters Datum objects from anything that may have been listening.
  // TODO: Calling pure virtual fxns from a protected destructor is apparently not good practice.
  //uint32_t datum_count = datumCount();
  //for (uint32_t i = 0; i < datum_count; i++) {
  //  _sensor_cb_general(SensorCallbackCode::SAY_GOODBYE, getDatum(i));
  //}
  _sensor_cb_general(SensorCallbackCode::SAY_GOODBYE, nullptr);
}


void C3PSensor::_print_c3p_sensor(StringBuilder* output) {
 output->concatf("%12s: %sound   %sabled   %salibrated   %snitialized   Pins%sconf'd\n",
   name,
   devFound() ? "F":"Not f",
   sensor_enabled() ? "En":"Dis",
   sensor_calibrated() ? "C":"Unc",
   sensor_initialized() ? "I":"Uni",
   pinsConfigured() ? " ":" not "
 );
}


void SensorDatum::dirty(bool x) {
  _dirty = x;
  if (x) _last_update = millis();
};

/*******************************************************************************
* Fxns that deal with sensor representation...                                 *
*******************************************************************************/

/*******************************************************************************
* Functions that manage the "dirty" state and autoreporting behavior of data.  *
*******************************************************************************/
// /* Returns the timestamp of the last sensor read in milliseconds. */
// long SensorWrapper::lastUpdate() {
//   return updated_at;
// }
//
// /* Is ANY datum in this sensor dirty? */
// bool SensorWrapper::isDirty() {
//   SensorDatum* current = datum_list;
//   while (current) {
//     if (current->dirty()) {
//       return true;
//     }
//     current = current->next();
//   }
//   return false;
// }
//
//
// /*
// * Mark a given datum in the sensor as having been updated.
// * Also marks the sensor-as-a-whole as having been updated.
// */
// SensorError SensorWrapper::mark_dirty(uint8_t dat) {
//   SensorDatum* current = get_datum(dat);
//   if (current) {
//     current->dirty(true);
//     updated_at = micros();
//     return SensorError::NO_ERROR;
//   }
//   return SensorError::INVALID_DATUM;
// }
//
//
// /*
// * Mark each datum in this sensor as having been read.
// */
// bool SensorWrapper::isHardware() {
//   SensorDatum* current = datum_list;
//   while (current) {
//     if (current->hardware()) {
//       return true;
//     }
//     current = current->next();
//   }
//   return false;
// }
//
//
// /*
// * Mark each datum in this sensor as having been read.
// */
// SensorError SensorWrapper::markClean() {
//   SensorDatum* current = datum_list;
//   while (current) {
//     current->dirty(false);
//     current = current->next();
//   }
//   return SensorError::NO_ERROR;
// }
//
//
// /*
// * Sets all data in this sensor to the given autoreporting state.
// */
// void SensorWrapper::setAutoReporting(SensorReporting nu_ar_state) {
//   SensorDatum* current = datum_list;
//   while (current) {
//     current->autoreport(nu_ar_state);
//     current = current->next();
//   }
// }
//
// /*
// * Sets a given datum in this sensor to the given autoreporting state.
// */
// SensorError SensorWrapper::setAutoReporting(uint8_t dat, SensorReporting nu_ar_state) {
//   SensorDatum* current = get_datum(dat);
//   if (current) {
//     current->autoreport(nu_ar_state);
//     return SensorError::NO_ERROR;
//   }
//   return SensorError::INVALID_DATUM;
// }
//
//
//
// /*******************************************************************************
// * Functions that generate string outputs.                                      *
// *******************************************************************************/
//
// SensorError SensorWrapper::readAsString(StringBuilder* buffer) {
//   return SensorError::NO_ERROR;
// }
//
//
// /*
// * Writes a string representation of the given datum to the provided StringBuffer.
// * Returns an error code.
// */
// SensorError SensorWrapper::readAsString(uint8_t dat, StringBuilder* buffer) {
//   SensorDatum* current = get_datum(dat);
//   if (current) {
//     current->dirty(false);
//     return current->printValue(buffer);
//   }
//   return SensorError::INVALID_DATUM;
// }
//
//
// void SensorWrapper::printSensorDataDefs(StringBuilder* output) {
//   int idx = 0;
//   SensorDatum* current = get_datum(idx);
//   while (nullptr != current) {
//     output->concatf("%d: 0x%02x  %s (%s)    %s\n", idx, current->def->flgs,
//       current->def->units, getTypeCodeString(current->def->type_id), current->def->desc
//     );
//     current = get_datum(++idx);
//   }
// }
//
//
// void SensorWrapper::printSensorData(StringBuilder* output) {
//   int idx = 0;
//   SensorDatum* current = get_datum(idx);
//   while (nullptr != current) {
//     output->concatf("%d:  ", idx);
//     current->printValue(output);
//     output->concatf(" %s\n", current->def->units);
//     current = get_datum(++idx);
//   }
// }
//
//
// SensorError SensorWrapper::issue_def_map(TCode tcode, StringBuilder* output) {
//   int idx = 0;
//   SensorDatum* current = get_datum(idx);
//
//   #if defined(__BUILD_HAS_CBOR)
//   cbor::output_dynamic coutput;
//   cbor::encoder encoder(coutput);
//   // We shall have an array of maps...
//   encoder.write_map(3);
//   encoder.write_string("name");
//   encoder.write_string(name);
//   encoder.write_string("uid");
//   encoder.write_bytes((uint8_t*) &uuid, sizeof(UUID));
//   encoder.write_string("data");
//   uint8_t arg_count = datum_list->argCount();
//   encoder.write_array(arg_count);
//   while (nullptr != current) {
//     encoder.write_map(4);
//     encoder.write_string("units");
//     encoder.write_string(current->def->units);
//     encoder.write_string("desc");
//     encoder.write_string(current->def->desc);
//     encoder.write_string("type");
//     encoder.write_string(getTypeCodeString(current->def->type_id));
//     encoder.write_string("flgs");
//     encoder.write_int((unsigned int) current->def->flgs);
//     current = current->next();
//   }
//   int final_size = coutput.size();
//   if (final_size) {
//     output->concat(coutput.data(), final_size);
//   }
//   #endif   //__BUILD_HAS_CBOR
//   return SensorError::NO_ERROR;
// }
//
//
// SensorError SensorWrapper::issue_value_map(TCode tcode, StringBuilder* output) {
//   switch (tcode) {
//     #if defined(__BUILD_HAS_CBOR)
//     case TCode::CBOR:
//       {
//         cbor::output_dynamic coutput;
//         cbor::encoder encoder(coutput);
//         encoder.write_map(1);
//         encoder.write_string(sensorName());
//
//         int final_size = coutput.size();
//         if (final_size) {
//           output->concat(coutput.data(), final_size);
//         }
//         if (0 == Argument::encodeToCBOR((Argument*) &datum_list, output)) {
//           return SensorError::NO_ERROR;
//         }
//       }
//       break;
//     #endif   //__BUILD_HAS_CBOR
//
//     #if defined(__BUILD_HAS_JSON)
//     case TCode::JSON:
//       return SensorError::NO_ERROR;
//     #endif   //__BUILD_HAS_CBOR
//     default:
//       break;
//   }
//   return SensorError::BAD_TYPE_CONVERT;
// }
