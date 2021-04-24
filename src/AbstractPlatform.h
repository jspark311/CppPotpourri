/*
* Abstract shape to which platform implementations must conform.
* This is broadly modeled from Arduino to facilitate cross-porting.
*/
#include <inttypes.h>
#include <stdint.h>
#include "BusQueue.h"
#include "StringBuilder.h"

class ParsingConsole;

#ifndef __ABSTRACT_PLATFORM_TEMPLATE_H__
#define __ABSTRACT_PLATFORM_TEMPLATE_H__

#if defined(__HAS_CRYPT_WRAPPER)
  #include <CryptoBurrito.h>
#endif

/**
* These are _pflags definitions.
*/
#define ABSTRACT_PF_FLAG_P_STATE_MASK     0x00000007  // Bits 0-2: platform-state.
#define ABSTRACT_PF_FLAG_PRIOR_BOOT       0x00000010  // Do we have memory of a prior boot?
#define ABSTRACT_PF_FLAG_RNG_READY        0x00000020  // RNG ready?
#define ABSTRACT_PF_FLAG_SERIALED         0x00000040  // Do we have a serial number?
#define ABSTRACT_PF_FLAG_HAS_STORAGE      0x00000080  // Does the hardware have app-usable NVM?
#define ABSTRACT_PF_FLAG_HAS_LOCATION     0x00000100  // Hardware is locus-aware.
#define ABSTRACT_PF_FLAG_INNATE_DATETIME  0x00000200  // Can the hardware remember the datetime?
#define ABSTRACT_PF_FLAG_RTC_READY        0x00000400  // RTC ready?
#define ABSTRACT_PF_FLAG_RTC_SET          0x00000800  // RTC trust-worthy?
#define ABSTRACT_PF_FLAG_BIG_ENDIAN       0x00001000  // Big-endian?
#define ABSTRACT_PF_FLAG_ALU_WIDTH_MASK   0x00006000  // Bits 13-14: ALU width, as 2^n.
#define ABSTRACT_PF_FLAG_STACK_GROWS_UP   0x00008000  // Stack grows upward.
#define ABSTRACT_PF_FLAG_HAS_IDENTITY     0x80000000  // Do we know who we are?


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


/* Shutdown causes. */
enum class ShutdownCause : uint8_t {
  UNSPECIFIED   = 0,  //
  FATAL_ERR     = 1,  // Something bad happened that couldn't be solved.
  USER          = 2,  // The user requested a shutdown.
  CONF_RELOAD   = 3,  // A conf change forced a reboot.
  REFLASH       = 4,  // The program was changed.
  TIMEOUT       = 5,  // The unit sat idle for too long.
  WATCHDOG      = 6,  // Something hung the firmware.
  BROWNOUT      = 7,  // The power sagged too far for comfort.
  THERMAL       = 8,  // The unit is too hot for proper operation.
  PWR_SAVE      = 9   // Orderly shutdown to save power.
};


/* A unifying type for different threading models. */
typedef void* (*ThreadFxnPtr)(void*);

typedef struct __platform_thread_opts {
  char*     thread_name;
  uint16_t  stack_sz;
} PlatformThreadOpts;



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
  int8_t rtcInit();
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

  uint64_t epochTime();            // Returns an integer representing the current datetime.
  void currentDateTime(StringBuilder*);    // Writes a human-readable datetime to the argument.

  uint8_t last_restart_reason();


  /* Functions that convert platform-general enums to strings. */
  const char* getPinModeStr(const GPIOMode);
  const char* shutdownCauseStr(const ShutdownCause);
  const char* getIRQConditionString(const IRQCondition);



/**
* This is base platform support. It is a pure virtual.
*/
class AbstractPlatform {
  public:
    #if defined(__HAS_CRYPT_WRAPPER)
    BurritoPlate crypto;
    #endif

    /* Accessors for platform capability discovery. */
    #if defined(__BUILD_HAS_THREADS)
      inline bool hasThreads() const {        return true;     };
    #else
      inline bool hasThreads() const {        return false;    };
    #endif
    #if defined(__HAS_CRYPT_WRAPPER)
      inline bool hasCryptography() const {   return true;     };
    #else
      inline bool hasCryptography() const {   return false;    };
    #endif
    inline bool hasSerialNumber() { return _check_flags(ABSTRACT_PF_FLAG_SERIALED);        };
    inline bool hasLocation() {     return _check_flags(ABSTRACT_PF_FLAG_HAS_LOCATION);    };
    inline bool hasTimeAndDate() {  return _check_flags(ABSTRACT_PF_FLAG_INNATE_DATETIME); };
    inline bool rtcInitilized() {   return _check_flags(ABSTRACT_PF_FLAG_RTC_READY);       };
    inline bool rtcAccurate() {     return _check_flags(ABSTRACT_PF_FLAG_RTC_SET);         };
    inline bool hasStorage() {      return _check_flags(ABSTRACT_PF_FLAG_HAS_STORAGE);     };
    inline bool bigEndian() {       return _check_flags(ABSTRACT_PF_FLAG_BIG_ENDIAN);      };
    inline uint8_t aluWidth() {
      // TODO: This is possible to do without the magic number 13... Figure out how.
      return (8 << ((_pflags & ABSTRACT_PF_FLAG_ALU_WIDTH_MASK) >> 13));
    };

    /* These are bootstrap checkpoints. */
    int8_t configureConsole(ParsingConsole*);
    inline uint8_t platformState() {   return (_pflags & ABSTRACT_PF_FLAG_P_STATE_MASK);  };

    void printCryptoOverview(StringBuilder*);

    virtual int8_t init()  =0;
    virtual void printDebug(StringBuilder*)  =0;

    /* Functions that don't return. */
    virtual void firmware_reset(uint8_t)     =0;
    virtual void firmware_shutdown(uint8_t)  =0;


  protected:
    const char* _board_name;
    StringBuilder _syslog;

    AbstractPlatform(const char* n) : _board_name(n) {};

    void _print_abstract_debug(StringBuilder* out);
    void _discover_alu_params();


    /* Inlines for altering and reading the flags. */
    inline void _alter_flags(bool en, uint32_t mask) {
      _pflags = (en) ? (_pflags | mask) : (_pflags & ~mask);
    };
    inline bool _check_flags(uint32_t mask) {
      return (mask == (_pflags & mask));
    };
    inline void _set_init_state(uint8_t s) {
      _pflags = ((_pflags & ~ABSTRACT_PF_FLAG_P_STATE_MASK) | s);
    };


  private:
    uint32_t   _pflags    = 0;
};


AbstractPlatform* platformObj();   // TODO: Until something smarter is done.


#endif  // __ABSTRACT_PLATFORM_TEMPLATE_H__
