/**
File:   C3PTrace.h
Author: J. Ian Lindsay
Date:   2021.10.16

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


"Trace" (proper noun) is a debugging tool that helps us chart program behavior
  under conditions of real-world workloads. As a noun, "trace" will refer to
As a verb, "trace" will be the act of collecting data points from program
  execution. These points and their embroidary constitute a "trace" (noun).
We need good metaphor for what this class does, so that we can contain the
  low-level language.

For our purposes here, computer source code is a 2D plane with "file ID" and
  "line number" forming the axes. The act of compiling it into object code
  maps the source plane onto a object plane (which hardware can execute, but
  which we can't easilly read).
With system-time being the 3rd axis, the profiler will describe the behavior
  of our code as a singular path through a 3-space object, with that object
  being ultimately defined by our source code's lexical content.

"TracePoint" is an empirically collected point on that path. It encodes a
  location in the runtime (which is a 3-space).
"TracePath" is a grouping of two TracePoints, and a known temporal distance
  between them. For the sake of profiling, TracePaths also collect aggregate
  statistics on the pathway under measurement.

TODO: Points and lines collected from within ISR stack frames (or threads)
  will be akin to being in a tesseract. For points, this won't be an
  annoyance. But unless we inform C3PTrace that a trace() call is originating
  from an ISR, there will be no way to discover the discontinuity in lines
  drawn with points in different stack frames, which will obfuscate timing
  relationships. 
This may not be a problem as long as we understand that our trace will be a
  projection of a 4D object into 3-space, and take care to trace lines without
  crossing stack frames.

TODO: Add wrapper macros in header file so access is cheaper, and usage easier.
*/
#include "../Meta/Rationalizer.h"

#ifndef __C3P_TRACE_H
#define __C3P_TRACE_H

#include "TimerTools.h"
class StringBuilder;


/*******************************************************************************
* C3PTrace
*******************************************************************************/
#if defined(CONFIG_C3P_TRACE_ENABLED)

// Define a few convenience values.
// This is the mask of useful bits in the tags passed into trace().
// NOTE: The defines below imply a field order and justification.
#define C3P_TRACE_WORD_MASK  (0xFFFFFFFF >> (32 - C3P_TRACE_WORD_TOTAL_BITS))
// Define offset values for each field in the trace word.
#define C3P_TRACE_WORD_LINE_OFFSET  (0)
#define C3P_TRACE_WORD_FILE_OFFSET  (CONFIG_C3P_TRACE_WORD_LINE_BITS)
#define C3P_TRACE_WORD_PATH_OFFSET  (CONFIG_C3P_TRACE_WORD_LINE_BITS + CONFIG_C3P_TRACE_WORD_FILE_BITS)
#define C3P_TRACE_WORD_ACTN_OFFSET  (CONFIG_C3P_TRACE_WORD_LINE_BITS + CONFIG_C3P_TRACE_WORD_FILE_BITS + C3P_TRACE_WORD_PATH_OFFSET)

// Define mask values for each field in the trace word.
#define C3P_TRACE_WORD_LINE_MASK    ((0xFFFFFFFF >> (32 - CONFIG_C3P_TRACE_WORD_LINE_BITS)) << C3P_TRACE_WORD_LINE_OFFSET)
#define C3P_TRACE_WORD_FILE_MASK    ((0xFFFFFFFF >> (32 - CONFIG_C3P_TRACE_WORD_FILE_BITS)) << C3P_TRACE_WORD_FILE_OFFSET)
#define C3P_TRACE_WORD_PATH_MASK    ((0xFFFFFFFF >> (32 - CONFIG_C3P_TRACE_WORD_PATH_BITS)) << C3P_TRACE_WORD_PATH_OFFSET)
#define C3P_TRACE_WORD_ACTN_MASK    ((0xFFFFFFFF >> (32 - CONFIG_C3P_TRACE_WORD_ACTN_BITS)) << C3P_TRACE_WORD_ACTN_OFFSET)

// A mask to filter out the location-related bits in the trace word.
#define C3P_TRACE_WORD_SPATIAL_MASK (C3P_TRACE_WORD_FILE_MASK | C3P_TRACE_WORD_LINE_MASK)


/*
* Any given call to trace will contain one of these enums to denote the
*   context of the trace.
*/
enum class TraceAction : uint8_t {
  POI             = 0,   // No profiling. Records a point.
  PATH_START      = 1,   // Mark the starting point for path profiling. Records a point.
  PATH_STOP       = 2,   // Mark the ending point for path profiling. Records a point.
  //GROUP_ENGAGE    = 3,   // Enable a group's profiling.
  //GROUP_DISENGAGE = 4,   // Disable a group's profiling.
  //START_RECORDING = 5,   // Controls the responsiveness of the profiler.
  //STOP_RECORDING  = 6,   // Controls the responsiveness of the profiler.
  INVALID          // Catch-all to indicate a lookup failure.
};


/* A container class for representing a single point in runtime. */
class TracePoint {
  public:
    unsigned int ts_micros;    // System time when the trace arrived at our gates.
    uint32_t     trace_word;   // The trace word for this point.

    TracePoint(unsigned int TIMESTAMP, uint32_t TRACE_WORD) : ts_micros(TIMESTAMP), trace_word(TRACE_WORD) {};
    TracePoint(TracePoint* tp) : TracePoint(tp->ts_micros, tp->trace_word) {};
    TracePoint(int) : TracePoint(0, 0) {}; // Stub to satisfy RingBuffer.
    TracePoint() : TracePoint(0, 0) {};

    inline uint16_t lineID() {  return ((trace_word & C3P_TRACE_WORD_LINE_MASK) >> C3P_TRACE_WORD_LINE_OFFSET);  };
    inline uint16_t fileID() {  return ((trace_word & C3P_TRACE_WORD_FILE_MASK) >> C3P_TRACE_WORD_FILE_OFFSET);  };

    int serialize(StringBuilder*, const TCode FORMAT = TCode::STR);
};


/*
* A container class for profiling execution from a single pathway in runtime.
*
* Rules:
*   1) The path ID defines the root of what might be several distinct pathways
*      once their end-points are known.
*   2) TracePath does not itself retain any point measurements beyond the last
*      unresolved report of a START point.
*/
class TracePath {
  public:
    const uint8_t     PATH_ID;      // ID by which this objects is recognized.

    TracePath(const uint8_t P) : PATH_ID(P) {};
    ~TracePath() {  reset();  };

    void reset() {
      _start_point.ts_micros  = 0;
      _start_point.trace_word = 0;
      while (_pathways.size() > 0) {  delete _pathways.dequeue();  }
    };

    bool recordStart(const uint32_t TRACE_WORD, const TracePoint*);
    bool recordStop(const uint32_t TRACE_WORD, const TracePoint*);

    int serialize(StringBuilder*, const TCode FORMAT = TCode::STR);


  //private:
    TracePoint _start_point;   // The start marker. Set once.
    // The central profiling object.
    // The priority in the queue is the STOP location in the source plane.
    // The tag in the StopWatch is set to the START location in the source plane.
    PriorityQueue<StopWatch*> _pathways;
};


/*
* This is an object to facilitate trace and profiling.
*
*
* ---< Usage modes >------------------------------------------------------------
*   Continuous:
*     Records traces forever, and over-writes the oldest traces once the memory
*     limit is reached.
*
*   One-shot:
*     Begins recording on a signal, and continues to record until it is either
*     interrupted, or the memory limit is reached (whichever happens first).
*/
class C3PTrace {
  public:
    C3PTrace(const uint32_t MAX_POINTS);
    ~C3PTrace();

    /*
    * This function is the ultimate intake for trace.
    * TODO: Compiler attributes on this function.
    */
    void leave_trace(const uint32_t TRACE_WORD);

    void reset();
    void serialize(StringBuilder*, const TCode FORMAT = TCode::STR);

    // If trace collection enabled?
    inline bool recording() {    return ((_recording_began > 0) & (0 == _recording_ended));  };
    bool recording(bool);

    static C3PTrace* tracerTool;


  private:
    unsigned int _recording_began;
    unsigned int _recording_ended;
    uint32_t     _trace_count;
    bool _mode_oneshot;   // Start tracing on signal and run until memory is exhausted. Clobber nothing.

    // TODO: For best results, we should store the trace results in a data structure
    //   that is as close to constant search time as possible. That we are using a
    //   structure with an uncontrolled search time is acceptable for now.
    RingBuffer<TracePoint>    _trace_points;
    PriorityQueue<TracePath*> _trace_paths;
};

#endif  // CONFIG_C3P_TRACE_ENABLED
#endif  // __C3P_TRACE_H
