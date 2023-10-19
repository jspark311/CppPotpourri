/*
File:   ScalarPipe.h
Author: J. Ian Lindsay
Date:   2023.10.18

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


ScalarPipe is an abstract interface for building composable numeric data
  pipelines that control for both error and units.
*/

#include "../../StringBuilder.h"
#include "../../EnumeratedTypeCodes.h"
#include "../../CppPotpourri.h"

#ifndef __C3P_SCALAR_PIPE_H__
#define __C3P_SCALAR_PIPE_H__

/*******************************************************************************
* An interface class for accepting high-quality scalar data.
*
* Rules:
*  1) The pusher allocates any values that go down the pipe, and retains both
*     local reference as well as ownership of the memory.
*  2) Downstream (in the pipeline) classes may not mutate any of the objects
*     which they might receive as arguments to their pushScalar() functions.
*  3) Downstream (in the pipeline) classes may not use any of the pushed data
*     by reference outside of the stack frame of pushScalar().
*  4) The value parameter cannot be NULL, and must be a numeric.
*  5) The value's underlying type should be assumed to not change between calls
*     to pushScalar(), since classes in the pipeline might wait until first push
*     to allocate memory for a type that wasn't known up until that point.
*  6) The units parameter may change between calls to pushScalar(). Some sensors
*     do auto-scaling to an extent that changes the scale of the data, and
*     order-of-magnitude is expressed in the SIUnit string.
*     See EnumeratedTypeCodes.h for details.
*  7) The error parameter is construed to be a +/- value, and is assumed to be
*     in the same units as the value. IE: (0.41184 Teslas +/- 0.01 Tesla)
*  8) The error parameter is optional, and should never be zero without a
*     rational basis. If error is unknown, the pipeline must propagate the fact.
*  9) The error parameter is mutable within the pipeline. That is, certain
*     operations done on the data will change the error figure. In such cases,
*     the class should retain its own C3PValue object that tracks the new error.
* 10) The error's underlying type has no relationship to that of the value. That
*     is, classes should be prepared for the possibility that the underlying
*     type of value might be (say) INT16, and that of its error report might be
*     FLOAT.
*******************************************************************************/
class ScalarPipe {
  public:
    /**
    * A class would implement this interface to accept a value from outside
    *   that has been tagged with an optional real-world unit and error bars.
    *
    * @return -1 to reject value, 0 to accept.
    */
    virtual int8_t pushScalar(C3PValue* value, SIUnit* units, C3PValue* error) =0;
};


/*******************************************************************************
* Helpers and utility classes surrounding ScalarPipe.
*******************************************************************************/

/*
* A (template?) class to terminate a pipe in a deep-copied, update-friendly way.
* Useful as end-points for sensor data.
*/
//class ScalarSink : public C3PValue, public ScalarPipe {
//  public:
//    /* Implementation of ScalarPipe */
//    int8_t pushScalar(C3PValue* value, SIUnit* units, C3PValue* error);
//};


/*
* A class that renders each incoming datum as a fully-qualified, human-readable
*   string, and acts as a source for a BufferAccepter pipeline.
*/
//class ScalarStringReporter : public ScalarPipe {
//  public:
//    int8_t setEfferent(BufferAccepter*);
//
//    /* Implementation of ScalarPipe */
//    int8_t pushScalar(C3PValue* value, SIUnit* units, C3PValue* error);
//};


/*
* A class to do such things as convert incoming data to given specific units
*   without inducing type-based aliasing as a consequence.
* This base class will fail if an attempt is made to change the fundamental
*   units of incoming data. But it will handle OoM conversion, as well as
*   anything that can be done in terms of fundamental constants.
* Any unit conversion application more complex than this (requires outside
*   assumptions) will need to be implemented as a child of this.
*/
//class ScalarUnitConverter : public ScalarPipe {
//  public:
//    int8_t setUnits(SIUnit*);
//
//    /* Implementation of ScalarPipe */
//    int8_t pushScalar(C3PValue* value, SIUnit* units, C3PValue* error);
//};


/*
* A class that buffers and records a time-series of incoming values, builds
*   statistical measurements from them, and optionally applies filtering.
* Statistical results are themselves usable as the heads of new pipelines.
*/
//class ScalarTimeSeries : public ScalarPipe {
//  public:
//    int8_t setOutput(StatisticalResult enum, ScalarPipe*);
//
//    /* Implementation of ScalarPipe */
//    int8_t pushScalar(C3PValue* value, SIUnit* units, C3PValue* error);
//};




#endif  // __C3P_SCALAR_PIPE_H__
