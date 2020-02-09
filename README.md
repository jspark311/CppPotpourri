# CppPotpourri

An architecture-agnostic package of discrete solutions to common problems in Arduino.

----------------------

## Data Structures

#### StringBuilder

A library for dynamic string abstraction.

#### LightLinkedList

A template for a linked list.

#### PriorityQueue

A template for a priority queue.

#### RingBuffer

A template for a ring buffer.

#### Vector3

A template for vectors in 3-space.

#### StopWatch

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
  * RingBuffer<T>

#### High-level classes

These have dependency on other classes in this repo

    ParsingConsole
      |
      +---StringBuilder
      |
      +---LightLinkedList

    PriorityQueue<T>
      |
      +---StringBuilder

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
