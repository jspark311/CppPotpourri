/*
File:   BufferShuttle.h
Author: J. Ian Lindsay
Date:   2022.05.28

This class is intended to move large amounts of buffer over some I/O channel
  that would result in unacceptably high peak-memory usage.

NOTE: Instances of this class will require polling, and come with concurrency
  stipulations that will be difficult to meet in many cases, and cannot be
  controlled for in this class.
Use this as a debug support tool, and not as a premise for actual production
  features.
*/


#include "CppPotpourri.h"
#include "AbstractPlatform.h"
#include "StringBuilder.h"
#include "ParsingConsole.h"
#include "GPSWrapper.h"
#include "UARTAdapter.h"
#include "KeyValuePair.h"
#include "SensorFilter.h"
#include "cbor-cpp/cbor.h"
#include "Image/Image.h"


class BufferShuttle {
  public:
    BufferShuttle() {};
    ~BufferShuttle() {};

    // TODO: Use lambdas to do transforms on buffer on polling. This might be as
    //   simple as a copy, or may involve a string conversion function of some sort.
};
