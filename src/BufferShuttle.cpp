/*
File:   BufferShuttle.cpp
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


#include "BufferShuttle.h"
