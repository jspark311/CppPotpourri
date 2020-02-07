# CppPotpourri

A package of discrete solutions to common problems in Arduino.

----------------------

## Data Structures

### StringBuilder

A library for dynamic string abstraction.

### LightLinkedList

### SensorFilter

A library for dynamic depth filtering of scalar and vector quantities.

### ParsingConsole

A library for a configurable debug console.


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

----------------------

#### License

Original code is Apache 2.0.

Code adapted from others' work inherits their license terms, which were preserved in the commentary where it applies.
