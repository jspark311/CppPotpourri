# CppPotpourri

An architecture-agnostic package of discrete solutions to common problems in C++.

----------------------

## Generally useful things

The library's main header (CppPotpourri.h) contains some basic things that I use as a premise
elsewhere in the library. Mostly inlines for type-safe min/max functions.

There is also a type-code enum system in use in this library.


## Data Structures

#### [StringBuilder](extras/doc/StringBuilder.md)

A library for dynamic string abstraction.

#### [LightLinkedList](extras/doc/LightLinkedList.md)

A template for a linked list.

#### [PriorityQueue](extras/doc/PriorityQueue.md)

A template for a priority queue.

#### [RingBuffer](extras/doc/RingBuffer.md)

A template for a ring buffer.

#### [Vector3](extras/doc/Vector3.md)

A template for vectors in 3-space.

#### [FlagContainer](extras/doc/FlagContainer.md)

A class to contain a mess of bitwise flags.

#### [StopWatch](extras/doc/StopWatch.md)

A class for implementing a stop watch from the platform's notion of microseconds.

#### [WakeLock](extras/doc/WakeLock.md)

A class for tracking power demands on hardware. Based roughly on [android.os.PowerManager.WakeLock](https://developer.android.com/reference/kotlin/android/os/PowerManager.WakeLock).

#### [Quaternion](extras/doc/Quaternion.md)

A class for encapsulating quaternions. Not as nice as Vector3 yet.

#### [UUID](extras/doc/UUID.md)

A collection of functions for common operations on UUIDs.

## High-level processing libraries

#### [SensorFilter and SensorFilter3](extras/doc/SensorFilter.md)

A library for dynamic-depth filtering of scalar and vector quantities.

#### [ParsingConsole](extras/doc/ParsingConsole.md)

A library for a configurable serial console.

#### [BusQueue](extras/doc/BusQueue.md)

A template for implementing I/O queues in a hardware-agnostic manner.

#### [ElementPool](extras/doc/ElementPool.md)

A template to implement a preallocation pool for heap-resident objects.

#### [GPSWrapper](extras/doc/GPSWrapper.md)

A class conversion of [minmea](https://github.com/cloudyourcar/minmea).

#### [Storage](extras/doc/Storage.md)

A hardware-agnostic abstraction for non-volatile storage.

#### [Image](extras/doc/Image.md)

A hard-fork of Adafruit's widespread graphics library with modifications to
allow a frame buffer, and/or function as a software-only bitmap library.

#### [TripleAxisPipe](extras/doc/TripleAxisPipe.md)

An interchange interface and utility classes for exchange and processing of vector data.

----------------------

## Dependency graph

#### Basal classes

These have no dependencies outside of those normally supplied by the environment.

  * StringBuilder
  * Quaternion
  * Vector3<T>
  * LightLinkedList<T>
  * PriorityQueue<T>
  * RingBuffer<T>

#### High-level classes

These have dependency on other classes in this repo...

    ParsingConsole
      |
      +---StringBuilder
      |
      +---LightLinkedList

    Image
      |
      +---StringBuilder

    SensorFilter<T>
      |
      +---StringBuilder

    SensorFilter3<T>
      |
      +---StringBuilder
      |
      +---Vector3<T>

    TripleAxisPipe
      |
      +---StringBuilder
      |
      +---SensorFilter3<float>
      |
      +---Vector3<T>

    GPSWrapper
      |
      +---StringBuilder

    ElementPool<T>
      |
      +---RingBuffer<T*>

    BusQueue<T>
      |
      +---ElementPool<T>
      |
      +---PriorityQueue<T*>
      |
      +---StringBuilder

    Storage
      |
      +---StringBuilder

    StopWatch
      |
      +---StringBuilder

    UUID
      |
      +---StringBuilder

----------------------

## Building the documentation

Documentation can be built with the supplied Doxyfile by running...

From the root of the repository, you can either...

    doxygen Doxyfile

Or...

    ~/CppPotpourri $ cd extras/doc/
    ~/CppPotpourri/extras/doc/ $ make

Doc will be output to `extras/doc/doxygen`.

----------------------

## Running the unit tests and verifying coverage

From the root of the repository, you can...

    ~/CppPotpourri $ cd extras/unit_tests/
    ~/CppPotpourri/extras/unit_tests/ $ make

An instrumented build of the unit tests will be created and executed. If the unit tests executed without failures, the output will then be left as HTML in `CppPotpourri/extras/unit_tests/build/coverage`. Please file an issue if that isn't what happens.

**NOTE:** The build will be forced to 32-bit, so if you aren't capable of building and running 32-bit binaries on your 64-bit machine, please don't file an issue. The library does, in fact, pass all tests under 64-bit. But because most usage is on 32-bit MCUs, the type sizes and unit tests should reflect that use-case. You can force a 64-bit build by commenting out the `-m32` line in the Makefile.

----------------------

#### License

Original code is Apache 2.0.

Code adapted from others' work inherits their license terms, which were preserved in the commentary where it applies.
