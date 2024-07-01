/*
File:   SensorFilterTests.cpp
Author: J. Ian Lindsay
Date:   2023.06.09

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

#include "AbstractPlatform.h"
#include "TimeSeries/SensorFilter.h"

/*******************************************************************************
* SensorFilter globals
*******************************************************************************/

/*******************************************************************************
* Scheduler test routines
*******************************************************************************/
/*
*
*/
int sensor_filter_init() {
  int ret = 0;
  return ret;
}


/*
*
*/
int sensor_filter_initial_conditions() {
  int ret = 0;
  return ret;
}


/*
* TODO
*/
int sensor_filter_stats_tests() {
  int ret = -1;
  ret = 0;
  return ret;
}


/*
*
*/
int sensor_filter_rewindowing() {
  int ret = 0;
  // TODO
  return ret;
}


/*
*
*/
int sensor_filter_nominal_operation_0() {
  int ret = 0;
  // TODO
  return ret;
}


/*
*
*/
int sensor_filter_nominal_operation_1() {
  int ret = 0;
  // TODO
  return ret;
}


/*
*
*/
int sensor_filter_nominal_operation_2() {
  int ret = 0;
  // TODO
  return ret;
}


/*
* Test the transfer of an entire package of timeseries data all at once.
*/
int sensor_filter_data_sharing_0() {
  int ret = -1;
  ret = 0;  // TODO: Wrong
  return ret;
}


/*
*
*/
int sensor_filter_data_sharing_1() {
  int ret = 0;
  // TODO
  return ret;
}


/*
*
*/
int sensor_filter_teardown() {
  int ret = 0;
  // TODO
  return ret;
}




void print_types_sensorfilter() {
  //printf("\tSensorFilter<uint8_t>    %u\t%u\n", sizeof(SensorFilter<uint8_t>), alignof(SensorFilter<uint8_t>));
  //printf("\tSensorFilter<int32_t>    %u\t%u\n", sizeof(SensorFilter<int32_t>), alignof(SensorFilter<int32_t>));
  //printf("\tSensorFilter<float>      %u\t%u\n", sizeof(SensorFilter<float>),   alignof(SensorFilter<float>));
  //printf("\tSensorFilter<double>     %u\t%u\n", sizeof(SensorFilter<double>),  alignof(SensorFilter<double>));
}



/*******************************************************************************
* SensorFilter main function.
*******************************************************************************/
int sensor_filter_tests_main() {
  int ret = 0;   // Failure is the default result.

  return ret;
}
