/*
* Abstract shape to which platform implementations must conform.
* This is broadly modeled from Arduino to facilitate cross-porting.
*/
#include <inttypes.h>
#include <stdint.h>
#include "BusQueue.h"
#include "StringBuilder.h"

#ifndef __ABSTRACT_PLATFORM_TEMPLATE_H__
#define __ABSTRACT_PLATFORM_TEMPLATE_H__

#if defined(ARDUINO)
  #include <Arduino.h>
  // Dance around existing preprocessor defines for Arduino-ish environments.
  #undef INPUT
  #undef OUTPUT
  #undef INPUT_PULLUP
  #undef INPUT_PULLDOWN
  #undef FALLING
  #undef RISING
  #undef CHANGE
#endif

/*
* Because these values don't matter to us internally, we use Arduino's defs
*   where overlap occurs.
*/
enum class GPIOMode : uint8_t {
  INPUT             = 0,
  OUTPUT            = 1,
  INPUT_PULLUP      = 2,
  INPUT_PULLDOWN    = 3,
  OUTPUT_OD         = 4,
  BIDIR_OD,
  BIDIR_OD_PULLUP,
  ANALOG_OUT,
  ANALOG_IN,
  UNINIT            = 255  // This is -1 when cast to int8.
};

enum class IRQCondition : uint8_t {
  FALLING   = 2,
  RISING    = 3,
  CHANGE    = 4,
  NONE      = 255
};

#if defined(ARDUINO)
  // Reinstate the Arduino definitions that we collided with...
  // TODO: This works under Teensyduino. Not certain it will work anywhere else.
  //   Find a better solution....
  //#define INPUT            0
  //#define OUTPUT           1
  //#define INPUT_PULLUP     2
  //#define INPUT_PULLDOWN   3
  //#define FALLING          2
  //#define RISING           3
  //#define CHANGE           4
#else
  extern "C" {
#endif


  int8_t platform_init();

  /* Delays and marking time. */
  void sleep_ms(uint32_t);
  void sleep_us(uint32_t);
  long unsigned millis();
  long unsigned micros();

  /* GPIO */
  int8_t pinMode(uint8_t pin, GPIOMode);
  int8_t analogWrite(uint8_t pin, float percentage);
  int8_t analogWriteFrequency(uint8_t pin, uint32_t freq);
  int8_t setPin(uint8_t pin, bool val);
  int8_t readPin(uint8_t pin);
  void   unsetPinFxn(uint8_t pin);
  int8_t setPinFxn(uint8_t pin, IRQCondition condition, FxnPointer fxn);

  /* Randomness */
  uint32_t randomUInt32();   // Blocks until one is available.
  int8_t random_fill(uint8_t* buf, size_t len);

  /* Time, date, and RTC abstraction */
  // TODO: This might be migrated into a separate abstraction.
  bool rtcInitilized();
  bool rtcAccurate();
  bool setTimeAndDateStr(char*);   // Takes a string of the form given by RFC-2822: "Mon, 15 Aug 2005 15:52:01 +0000"   https://www.ietf.org/rfc/rfc2822.txt

  /*
  * RTC get/set with discrete value breakouts.
  *
  * @param y   Year
  * @param m   Month [1, 12]
  * @param d   Day-of-month [1, 31]
  * @param h   Hours [0, 23]
  * @param mi  Minutes [0, 59]
  * @param s   Seconds [0, 59]
  * @return true on success.
  */
  bool setTimeAndDate(uint16_t y, uint8_t m, uint8_t d, uint8_t h, uint8_t mi, uint8_t s);
  bool getTimeAndDate(uint16_t* y, uint8_t* m, uint8_t* d, uint8_t* h, uint8_t* mi, uint8_t* s);

  uint32_t epochTime();            // Returns an integer representing the current datetime.
  void currentDateTime(StringBuilder*);    // Writes a human-readable datetime to the argument.

  /* Functions that are provided by this package. */
  const char* getPinModeStr(GPIOMode);

#if !defined(ARDUINO)
}  // extern "C"
#endif

#endif  // __ABSTRACT_PLATFORM_TEMPLATE_H__
