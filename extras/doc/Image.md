# Image

A hard-fork of Adafruit's widespread graphics library with modifications to
allow a frame buffer, and/or function as a software-only bitmap library.

Adafruit's original [license](src/Image/LICENSE.md).

#### Changes from Adafruit's baseline at time of fork:

* Added a frame buffer and associated handling logic.
* Consolidated the original device abstractions into this API, and opted for extension by inheritance.
* Added some drawing features.
* Removed print extension.
* Added serialization functions.
* Cleaned up class members. Booleans are now integer masks.

#### Buffer-to-pixel relationship

                      Top
       ________________________________
       | 0, 1, 2...   x -->
       | 1
    L  | 2
    e  |
    f  | y
    t  | |
       | V
       |


## Usage example as a frame buffer


#### Dependencies

This class uses StringBuilder as an optional buffer interchange class.
