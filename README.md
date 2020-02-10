# CppPotpourri

An architecture-agnostic package of discrete solutions to common problems in Arduino.

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

#### [StopWatch](extras/doc/StopWatch.md)

A class for implementing a stop watch from the platform's notion of microseconds.

## High-level processing libraries

#### SensorFilter

A library for dynamic-depth filtering of scalar and vector quantities.

#### ParsingConsole

A library for a configurable serial console.

----------------------

## Dependency graph

#### Basal classes

These have no dependencies outside of those normally supplied by the environment.

  * StringBuilder
  * Vector3<T>
  * LightLinkedList<T>
  * PriorityQueue<T>
  * RingBuffer<T>

#### High-level classes

These have dependency on other classes in this repo

    ParsingConsole
      |
      +---StringBuilder
      |
      +---LightLinkedList

    SensorFilter<T>
      |
      +---StringBuilder

    StopWatch
      |
      +---StringBuilder

----------------------

#### License

Original code is Apache 2.0.

Code adapted from others' work inherits their license terms, which were preserved in the commentary where it applies.
