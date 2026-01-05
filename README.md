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

#### [KeyValuePair](extras/doc/KeyValuePair.md)

A data type for describing, parsing, and packing rich data types.

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

#### [Quaternion](extras/doc/Quaternion.md)

A class for encapsulating quaternions. Not as nice as Vector3 yet.

#### [UUID](extras/doc/UUID.md)

A collection of functions for common operations on UUIDs.

## High-level processing libraries

#### [SensorFilter and SensorFilter3](extras/doc/SensorFilter.md)

A library for dynamic-depth filtering of scalar and vector quantities.

#### [ParsingConsole](extras/doc/ParsingConsole.md)

A library for a configurable serial console.

#### [Identity](extras/doc/Identity.md)

An interface for representing and handling identities.

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

    Identity
      |
      +---UUID
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

    KeyValuePair
      |
      +---StringBuilder
      |
      +---(see notes in KeyValuePair doc)

----------------------

## Building the documentation

Documentation can be built with the supplied Doxyfile by running...

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

#### Contributing

I will gladly examine all merge/pull requests. But you have to be ok with your contributed code being under this repo's license. You will be listed in the [extras/doc/CONTRIB.md](CONTRIB.md) file.

----------------------

#### Code formatting notes

  * Hard-tabs are exterminated upon discovery, and replaced with two spaces. Inline commentary is unreadable otherwise.
  * I have tried to remain within an 80-character horizontal ruling, but this sometimes hurts legibility.
  * The ASCII-art banners in this repo was generated with [this tool](https://www.developmenttools.com/ascii-art-generator/#p=display&f=Slant) and is font "Slant" with all defaults. Uniformity of font face is optional, but the entire banner must be within `/**/` commentary delimiters to avoid confusing the compiler's parser/lexer with stray `\`, or other unintentional escape sequences.

----------------------

#### License

Original code is Apache 2.0.

Code adapted from others' work inherits their license terms, which were preserved in the commentary where it applies.
