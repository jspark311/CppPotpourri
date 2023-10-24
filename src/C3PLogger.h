/**
File:   C3PLogger.h
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


This is a special-purpose text-handling class. It accepts log and renders it
  into a string suitable for being fed to a serial port, socket, file, etc.
  Using it is completely optional, and is only useful for sophisticated output
  of logs under conditions that don't automatically provide for it (such as
  stdio, or a serial port).

Platforms that have built-in faculties for logging should not use this class,
  but instead implement the c3p_log() functions in such a way as to wrap their
  existing APIs. See associated notes in AbstractPlatform.h.
*/

#include "StringBuilder.h"
#include "TimerTools.h"
#include "Pipes/BufferAccepter/BufferAccepter.h"

#ifndef __CPPPOTPOURRI_LOGGER_H__
#define __CPPPOTPOURRI_LOGGER_H__

// This is the maximum length of a log tag. Tags
//   exceeding this length will be truncated.
#define LOG_TAG_MAX_LEN             24

// These flags only apply to the optional C3PLogger class.
#define LOGGER_FLAG_PRINT_LEVEL   0x01  // Should output include the severity?
#define LOGGER_FLAG_PRINT_TIME    0x02  // Should output include the timestamp?
#define LOGGER_FLAG_PRINT_TAG     0x04  // Should output include the tag?

/*
* Class for emulating a logging faculty on platforms that
*   don't otherwise support it.
*/
class C3PLogger {
  public:
    C3PLogger(const uint8_t F = 0, BufferAccepter* SINK = nullptr)
      : _sink(SINK), _tag_ident(1), _flags(F), _verb_limit(LOG_LEV_DEBUG) {};
    ~C3PLogger() {};

    // On builds that use this class, this function will be called by c3p_log().
    int8_t print(uint8_t severity, const char* tag, StringBuilder*);

    void fetchLog(StringBuilder*);

    inline void    setSink(BufferAccepter* x) {    _sink = x;      };
    inline void    verbosity(uint8_t x) {    _verb_limit = x;      };
    inline uint8_t verbosity() {             return _verb_limit;   };

    inline bool    printSeverity() {         return _class_flag(LOGGER_FLAG_PRINT_LEVEL);    };
    inline bool    printTime() {             return _class_flag(LOGGER_FLAG_PRINT_TIME);     };
    inline bool    printTag() {              return _class_flag(LOGGER_FLAG_PRINT_TAG);      };
    inline void    printSeverity(bool x) {   _class_set_flag(LOGGER_FLAG_PRINT_LEVEL, x);    };
    inline void    printTime(bool x) {       _class_set_flag(LOGGER_FLAG_PRINT_TIME, x);     };
    inline void    printTag(bool x) {        _class_set_flag(LOGGER_FLAG_PRINT_TAG, x);      };


  private:
    StringBuilder   _log;          // Local buffer.
    BufferAccepter* _sink;         // Optional sink for formatted text.
    uint8_t         _tag_ident;    // Padding scalar to keep output aligned.
    uint8_t         _flags;        // Options that control output.
    uint8_t         _verb_limit;   // Used as a global limit.

    void _store_or_forward(StringBuilder*);

    inline bool _class_flag(uint8_t f) {   return ((_flags & f) == f);   };
    inline void _class_clear_flag(uint8_t _flag) {  _flags &= ~_flag;    };
    inline void _class_set_flag(uint8_t _flag) {    _flags |= _flag;     };
    inline void _class_set_flag(bool nu, uint8_t _flag) {
      _flags = (nu) ? (_flags | _flag) : (_flags & ~_flag);
    };
};



/*******************************************************************************
* C3PTrace
* "Trace" (proper noun) is a debugging tool that helps us chart program behavior
*   under conditions of real-world workloads. As a noun, "trace" will refer to
* As a verb, "trace" will be the act of collecting data points from program
*   execution. These points and their embroidary constitute a "trace" (noun).
* We need good metaphor for what this class does, so that we can contain the
*   low-level language.
*
* For our purposes here, computer source code is a 2D plane with "file ID" and
*   "line number" forming the axes. The act of compiling it into object code
*   maps the source plane onto a object plane (which hardware can execute, but
*   which we can't easilly read).
* With system-time being the 3rd axis, the profiler will describe the behavior
*   of our code as a singular path through a 3-space object, with that object
*   being ultimately defined by our source code's lexical content.
*
* "TracePoint" is an empirically collected point on that path. It encodes a
*   location in the runtime (which is a 3-space).
* "TracePath" is a grouping of two TracePoints, and a known temporal distance
*   between them. For the sake of profiling, TracePaths also collect aggregate
*   statistics on the pathway under measurement.
*
*
* TODO: Points and lines collected from within ISR stack frames (or threads)
*   will be akin to being in a tesseract. For points, this won't be an
*   annoyance. But unless we inform C3PTrace that a trace() call is originating
*   from an ISR, there will be no way to discover the discontinuity in lines
*   drawn with points in different stack frames, which will obfuscate timing
*   relationships.
* This may not be a problem as long as we understand that our trace will be a
*   projection of a 4D object into 3-space, and take care to trace lines without
*   crossing stack frames.
*******************************************************************************/

// TODO: Add wrapper macros in header file so access is cheaper, and usage easier.

#if defined(CONFIG_C3P_TRACE_ENABLED)
  // TODO: Properly wrap these into the pre-processor check file (Rationalizer).
  // TODO: Feature control for trace can be restricted by setting any of these values to 0.

  // How much heap should we allocate for the trace log?
  #ifndef CONFIG_C3P_TRACE_MAX_POINTS
    #define CONFIG_C3P_TRACE_MAX_POINTS    1024
  #endif

  // How many lines can a file have?
  #ifndef CONFIG_C3P_TRACE_WORD_LINE_BITS
    #define CONFIG_C3P_TRACE_WORD_LINE_BITS   14
  #endif

  // How many files can safely contain trace calls?
  #ifndef CONFIG_C3P_TRACE_WORD_FILE_BITS
    #define CONFIG_C3P_TRACE_WORD_FILE_BITS    9
  #endif

  // How many pathways can we distinguish? Maximum value of 8, and a minimum of 1.
  #ifndef CONFIG_C3P_TRACE_WORD_PATH_BITS
    #define CONFIG_C3P_TRACE_WORD_PATH_BITS    6
  #endif

  // How many pathways can we distinguish? Maximum value of 8, and a minimum of 1.
  #ifndef CONFIG_C3P_TRACE_WORD_ACTN_BITS
    #define CONFIG_C3P_TRACE_WORD_ACTN_BITS    3
  #endif

  // How many bits did we define for use in the trace words?
  #define C3P_TRACE_WORD_TOTAL_BITS ( \
    CONFIG_C3P_TRACE_WORD_PATH_BITS + CONFIG_C3P_TRACE_WORD_FILE_BITS + \
    CONFIG_C3P_TRACE_WORD_LINE_BITS + CONFIG_C3P_TRACE_WORD_ACTN_BITS)

  #if (32 < CONFIG_C3P_TRACE_WORD_TOTAL_BITS)
    #error TRACE_WORD bit size exceeds the length of its storage type (32-bit).
  #endif

  // Define a few convenience values. This is the mask of useful bits in the
  //   tags passed into trace().
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
  #define C3P_TRACE_WORD_SPATIAL_MASK ((0xFFFFFFFF >> (32 - CONFIG_C3P_TRACE_WORD_FILE_BITS)) << C3P_TRACE_WORD_FILE_OFFSET)


/*
* Any given call to trace will contain one of these enums to denote the
*   context of the trace.
*/
enum class TraceAction : uint8_t {
  POI             = 0,   // No profiling. Records a point.
  START_POINT     = 1,   // Mark the starting time for path profiling. Records a log.
  STOP_POINT      = 2,   // Mark the ending time for path profiling. Records a log.
  //GROUP_ENGAGE    = 3,   // Enable a group's profiling.
  //GROUP_DISENGAGE = 4,   // Disable a group's profiling.
  //START_RECORDING = 5,   // Controls the responsiveness of the profiler.
  //STOP_RECORDING  = 6,   // Controls the responsiveness of the profiler.
  INVALID          // Catch-all to indicate a lookup failure.
};


/* A container class for representing a single point in runtime. */
TracePoint {
  public:
    unsigned int ts_micros;    // System time when the trace arrived at our gates.
    uint32_t     trace_word;   // The trace word for this point.
    // NOTE: Things we might put in this data (at high RAM cost)
    // uint32_t  stack_ptr;

    TracePoint(unsigned int TIMESTAMP, uint32_t TRACE_WORD) : ts_micros(TIMESTAMP), trace_word(TRACE_WORD) {};
    TracePoint(TracePoint* tp) : TracePoint(tp->ts_micros, tp->trace_word) {};
    TracePoint() : TracePoint(0, 0) {};

    inline uint16_t lineID() {  return ((trace_word & C3P_TRACE_WORD_LINE_MASK) >> C3P_TRACE_WORD_LINE_OFFSET);  };
    inline uint16_t fileID() {  return ((trace_word & C3P_TRACE_WORD_FILE_MASK) >> C3P_TRACE_WORD_FILE_OFFSET);  };

    void export(StringBuilder*, const TCode FORMAT = TCode::STR);
};


/*
* A container class for profiling execution from a single pathway in runtime.
*
* Rules:
*   1) The path ID defines the root of what might be several distinct pathways
*      once their end-points are known.
*   2) TracePath objects will be heap-allocated and instanced on first use.
*      Because this action carries overhead costs that would differ from all
*      subsequent calls to trace(), programs are encouraged to make explicit
*      calls to definePath() ahead of the first trace() call using the pathID.
*   3) TracePath does not itself retain a temporal measurement beyond the last
*      unresolved report of a START point.
*/
class TracePath {
  public:
    const uint8_t     PATH_ID;      // ID by which this objects is recognized.

    TracePath(const uint8_t P) : PATH_ID(P) {};
    ~TracePath() {  reset();  };

    void reset() {
      _start_point.ts_micros = 0;
      while (_pathways.count() > 0) {  delete _pathways.dequeue();  }
    };

    bool recordStart(const uint32_t TRACE_WORD, const TracePoint*);
    bool recordStop(const uint32_t TRACE_WORD, const TracePoint*);

    void export(StringBuilder*, const TCode FORMAT = TCode::STR);


  private:
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
    void trace(const uint32_t TRACE_WORD);

    void reset();
    void generateReport(StringBuilder*, const TCode FORMAT = TCode::STR);

    // If trace collection enabled?
    inline bool recording() {    return ((_recording_began > 0) & (0 == _recording_ended));  };
    bool recording(bool);

    static C3PTrace* tracerTool();


  private:
    unsigned int _recording_began;
    unsigned int _recording_ended;
    uint32_t     _trace_count;

    bool _mode_oneshot = false;   // Start tracing on signal and run until memory is exhausted. Clobber nothing.

    // TODO: For best results, we should store the trace results in a data structure
    //   that is as close to constant search time as possible. That we are using a
    //   structure with an uncontrolled search time is acceptable for now.
    RingBuffer<TracePoint>    _trace_points;
    PriorityQueue<TracePath*> _trace_paths;
};

#endif  // CONFIG_C3P_TRACE_ENABLED
#endif  // __CPPPOTPOURRI_LOGGER_H__
