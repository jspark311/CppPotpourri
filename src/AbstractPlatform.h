/*
* Abstract shape to which platform implementations must conform.
* This is broadly modeled from Arduino to facilitate cross-porting.
*/
#ifndef __ABSTRACT_PLATFORM_TEMPLATE_H__
#define __ABSTRACT_PLATFORM_TEMPLATE_H__

#include <inttypes.h>
#include <stdint.h>
#include <limits.h>
#include "CppPotpourri.h"
#include "StringBuilder.h"
#include "CryptoBurrito/Cryptographic.h"

class C3PConsole;
class CryptoProcessor;

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

/**
* The API to the Logger supports log severity and source tags (as strings).
* We adopt the SYSLOG severity conventions. Because those work really well. All
*   code written against the logging functions should use one of these defines
*   as their severity argument.
*/
#define LOG_LEV_EMERGENCY  0  // Emergency
#define LOG_LEV_ALERT      1  // Alert
#define LOG_LEV_CRIT       2  // Critical
#define LOG_LEV_ERROR      3  // Error
#define LOG_LEV_WARN       4  // Warning
#define LOG_LEV_NOTICE     5  // Notice
#define LOG_LEV_INFO       6  // Informational
#define LOG_LEV_DEBUG      7  // Debug


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
  char*     thread_name;  // Identifier.
  uint16_t  stack_sz;
  int8_t    priority;     // Thread priority from baseline.
  uint8_t   core;         // Core-boundedness, if possible.
} PlatformThreadOpts;



/*******************************************************************************
*     ____  _          ______            __             __
*    / __ \(_)___     / ____/___  ____  / /__________  / /
*   / /_/ / / __ \   / /   / __ \/ __ \/ __/ ___/ __ \/ /
*  / ____/ / / / /  / /___/ /_/ / / / / /_/ /  / /_/ / /
* /_/   /_/_/ /_/   \____/\____/_/ /_/\__/_/   \____/_/
*******************************************************************************/
  int8_t pinMode(uint8_t pin, GPIOMode);
  int8_t analogWrite(uint8_t pin, float percentage);
  int8_t analogWriteFrequency(uint8_t pin, uint32_t freq);
  int8_t setPin(uint8_t pin, bool val);
  int8_t readPin(uint8_t pin);
  void   unsetPinFxn(uint8_t pin);
  int8_t setPinFxn(uint8_t pin, IRQCondition condition, FxnPointer fxn);

  /**
  * An optional interface class for providing GPIO capabilities.
  * NOTE: The platform is in no way obliged to use this.
  */
  class GPIOWrapper {
    public:
      virtual int8_t gpioMode(uint8_t pin, GPIOMode m)    =0;
      virtual int8_t digitalWrite(uint8_t pin, bool val)  =0;
      virtual int8_t digitalRead(uint8_t pin)             =0;
  };


/*******************************************************************************
*     ______      __
*    / ____/___  / /__________  ____  __  __
*   / __/ / __ \/ __/ ___/ __ \/ __ \/ / / /
*  / /___/ / / / /_/ /  / /_/ / /_/ / /_/ /
* /_____/_/ /_/\__/_/   \____/ .___/\__, /
*                           /_/    /____/
*******************************************************************************/
  /**
  * This function may block until a random number is availible.
  *
  * @return A 32-bit unsigned random number. This can be cast as needed.
  */
  uint32_t randomUInt32();

  /**
  * This function may block until enough random numbers are availible.
  *
  * @param buf is the buffer to which random bytes should be written.
  * @param len is the number of bytes to write.
  */
  int8_t random_fill(uint8_t* buf, size_t len);



/*******************************************************************************
*   _______                                   __   ____        __
*  /_  __(_)___ ___  ___     ____ _____  ____/ /  / __ \____ _/ /____
*   / / / / __ `__ \/ _ \   / __ `/ __ \/ __  /  / / / / __ `/ __/ _ \
*  / / / / / / / / /  __/  / /_/ / / / / /_/ /  / /_/ / /_/ / /_/  __/
* /_/ /_/_/ /_/ /_/\___/   \__,_/_/ /_/\__,_/  /_____/\__,_/\__/\___/
*******************************************************************************/
  /* System Time functions. These do not rely on an RTC. */
  void sleep_ms(uint32_t);
  void sleep_us(uint32_t);
  long unsigned millis();
  long unsigned micros();
  long unsigned millis_since(const long unsigned);
  long unsigned micros_since(const long unsigned);
  long unsigned millis_until(const long unsigned);
  long unsigned micros_until(const long unsigned);

  /* Real Time functions. These rely on an RTC. */
  int8_t rtcInit();  // TODO: This might be migrated into a separate abstraction.
  bool setTimeAndDateStr(char*);   // Takes a string of the form given by RFC-2822: "Mon, 15 Aug 2005 15:52:01 +0000"   https://www.ietf.org/rfc/rfc2822.txt

  /**
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



/*******************************************************************************
*     ______                         _____                              __
*    / ____/___  __  ______ ___     / ___/__  ______  ____  ____  _____/ /_
*   / __/ / __ \/ / / / __ `__ \    \__ \/ / / / __ \/ __ \/ __ \/ ___/ __/
*  / /___/ / / / /_/ / / / / / /   ___/ / /_/ / /_/ / /_/ / /_/ / /  / /_
* /_____/_/ /_/\__,_/_/ /_/ /_/   /____/\__,_/ .___/ .___/\____/_/   \__/
*                                           /_/   /_/
*
* Functions that convert platform-general enums to strings.
*******************************************************************************/
  const char* getPinModeStr(const GPIOMode);
  const char* shutdownCauseStr(const ShutdownCause);
  const char* getIRQConditionString(const IRQCondition);



/*******************************************************************************
*     __                      _
*    / /   ____  ____ _____ _(_)___  ____ _
*   / /   / __ \/ __ `/ __ `/ / __ \/ __ `/
*  / /___/ /_/ / /_/ / /_/ / / / / / /_/ /
* /_____/\____/\__, /\__, /_/_/ /_/\__, /
*             /____//____/        /____/
*
* Logging is fundamentally a platform choice, since platform support is
*   ultimately required to print a character to a screen, file, socket, etc.
*
* CppPotpourri implements c3p_log(StringBuilder*) as a weak reference stub.
*   So if it is not provided by ManuvrPlatform (or the user's code), nothing
*   will happen when c3p_log() is called.
*
* Final implementation will supplant this behavior, if given. Good support
*   should ultimately migrate into ManuvrPlatform with the rest of the
*   platform-specific implementations of AbstractPlatform, I2CAdapter, et al.
*   From that point, the platform can make choices about which modes of
*   output/caching/policy will be available to the program.
*
* See Motherflux0r for a case where logging support is provided by user code.
* See ManuvrPlatform/ESP32 and ManuvrPlatform/Linux for cases where existing
*   support is being wrapped.
*******************************************************************************/
  void c3p_log(uint8_t log_level, const char* tag, const char* format, ...);
  void c3p_log(uint8_t log_level, const char* tag, StringBuilder*);



/*******************************************************************************
*     ____  __      __  ____                        ____  ____      __
*    / __ \/ /___ _/ /_/ __/___  _________ ___     / __ \/ __ )    / /
*   / /_/ / / __ `/ __/ /_/ __ \/ ___/ __ `__ \   / / / / __  |_  / /
*  / ____/ / /_/ / /_/ __/ /_/ / /  / / / / / /  / /_/ / /_/ / /_/ /
* /_/   /_/\__,_/\__/_/  \____/_/  /_/ /_/ /_/   \____/_____/\____/
*
* This is base platform support. It is a pure virtual.
* ManuvrPlatform provides examples of extending this class to support specific
*   concrete platforms.
*******************************************************************************/
class AbstractPlatform {
  public:
    #if defined(__HAS_CRYPT_WRAPPER)
      CryptoProcessor* crypto = nullptr;
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
    int8_t configureConsole(C3PConsole*);
    inline uint8_t platformState() {   return (_pflags & ABSTRACT_PF_FLAG_P_STATE_MASK);  };

    void printCryptoOverview(StringBuilder*);

    virtual int8_t init()  =0;
    virtual void printDebug(StringBuilder*)  =0;

    /* Functions that don't return. */
    virtual void firmware_reset(uint8_t)     =0;
    virtual void firmware_shutdown(uint8_t)  =0;


  protected:
    const char* _board_name;
    uint32_t    _pflags = 0;

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
};



/*******************************************************************************
*     __  ____
*    /  |/  (_)_________
*   / /|_/ / / ___/ ___/
*  / /  / / (__  ) /__
* /_/  /_/_/____/\___/
*******************************************************************************/
  /*
  * These are callbacks for C3PConsole that the application
  *   might rather use in isolation.
  */
  int callback_gpio_value(StringBuilder* text_return, StringBuilder* args);
  int callback_platform_info(StringBuilder* text_return, StringBuilder* args);
  int callback_reboot(StringBuilder* text_return, StringBuilder* args);

  /**
  * A convenience function for doing platform init from the application.
  * NOTE: It might not be called by the application.
  *
  * @return 0 on success. Non-zero otherwise.
  */
  int8_t platform_init();

  /**
  * Platform is always a singleton, and only that body of support code should be
  *   forced to care about the detail of "which platform". So this function must
  *   be implemented with a cast by the specific support code in ManuvrPlatform.
  * See the notes in ManuvrPlatform for details.
  */
  AbstractPlatform* platformObj();

  /**
  * @return the integer-code associated with the reason for the last restart.
  */
  uint8_t last_restart_reason();

#endif  // __ABSTRACT_PLATFORM_TEMPLATE_H__
