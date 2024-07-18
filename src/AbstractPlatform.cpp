/*
* Abstract shape to which platform implementations must conform.
* This is broadly modeled from Arduino to facilitate cross-porting.
*/
#include <inttypes.h>
#include <stdint.h>
#include "AbstractPlatform.h"
#include "C3PLogger.h"
#include "Console/C3PConsole.h"
#include "CryptoBurrito/CryptoBurrito.h"

/**
* Issue a human-readable string representing the platform state.
*
* @return A string constant.
*/
const char* getPinModeStr(const GPIOMode mode) {
  switch (mode) {
    case GPIOMode::INPUT:           return "INPUT";
    case GPIOMode::OUTPUT:          return "OUTPUT";
    case GPIOMode::INPUT_PULLUP:    return "INPUT_PULLUP";
    case GPIOMode::INPUT_PULLDOWN:  return "INPUT_PULLDOWN";
    case GPIOMode::OUTPUT_OD:       return "OUTPUT_OD";
    case GPIOMode::BIDIR_OD:        return "BIDIR_OD";
    case GPIOMode::BIDIR_OD_PULLUP: return "BIDIR_OD_PULLUP";
    case GPIOMode::ANALOG_OUT:      return "ANALOG_OUT";
    case GPIOMode::ANALOG_IN:       return "ANALOG_IN";
    case GPIOMode::UNINIT:
    default:
      break;
  }
  return "UNINIT";
}

const char* shutdownCauseStr(const ShutdownCause cause) {
  switch (cause) {
    case ShutdownCause::FATAL_ERR:     return "FATAL_ERR";
    case ShutdownCause::USER:          return "USER";
    case ShutdownCause::CONF_RELOAD:   return "CONF_RELOAD";
    case ShutdownCause::REFLASH:       return "REFLASH";
    case ShutdownCause::TIMEOUT:       return "TIMEOUT";
    case ShutdownCause::WATCHDOG:      return "WATCHDOG";
    case ShutdownCause::BROWNOUT:      return "BROWNOUT";
    case ShutdownCause::THERMAL:       return "THERMAL";
    case ShutdownCause::PWR_SAVE:      return "PWR_SAVE";
    default:  break;
  }
  return "UNSPECIFIED";
}


/**
* Issue a human-readable string representing the condition that causes an
*   IRQ to fire.
*
* @param  An IRQ condition code.
* @return A string constant.
*/
const char* getIRQConditionString(const IRQCondition con_code) {
  switch (con_code) {
    case IRQCondition::RISING:   return "RISING";
    case IRQCondition::FALLING:  return "FALLING";
    case IRQCondition::CHANGE:   return "CHANGE";
    default:                     return "<UNDEF>";
  }
}


/*******************************************************************************
*
*******************************************************************************/

/**
* Do the boilerplate setup of the MCU that all applications will require.
*
* @return 0 on success. Negative on failure.
*/
int8_t __attribute__((weak)) platform_init() {
  int8_t ret = -127;
  if (platformObj()) {
    ret = platformObj()->init();
  }
  return ret;
}


void AbstractPlatform::_discover_alu_params() {
  // We infer the ALU width by the size of pointers.
  // TODO: This will not work down to 8-bit because of paging schemes.
  uint32_t default_flags = 0;
  switch (sizeof(uintptr_t)) {
    case 2:   default_flags |= (uint32_t) (1 << 13);     break;
    case 4:   default_flags |= (uint32_t) (2 << 13);     break;
    case 8:   default_flags |= (uint32_t) (3 << 13);     break;
    default:  break;   // Default case is 8-bit. Do nothing.
  }
  _alter_flags(true, default_flags);

  // Now determine the endianess with a magic number and a pointer dance.
  if (8 != aluWidth()) {
    const uint16_t TEST = 0xAA55;
    _alter_flags((0xAA == *((uint8_t*) &TEST)), ABSTRACT_PF_FLAG_BIG_ENDIAN);
  }

  uintptr_t initial_sp = 0;
  uintptr_t final_sp   = 0;
  _alter_flags((&final_sp > &initial_sp), ABSTRACT_PF_FLAG_STACK_GROWS_UP);
}


/**
* Prints details about this platform.
*
* @param  StringBuilder* The buffer to output into.
*/
void AbstractPlatform::_print_abstract_debug(StringBuilder* output) {
  output->concat("\tBuild date " __DATE__ " " __TIME__ "\n");
  output->concatf("\tALU: %u-bit", aluWidth());
  if (8 != aluWidth()) {
    output->concatf("(%cE)", bigEndian() ? 'B' : 'L');
  }
  output->concatf(", stack grows %s\n", _check_flags(ABSTRACT_PF_FLAG_STACK_GROWS_UP) ? "up" : "down");
  output->concatf("\tmillis():  %lu\n", millis());
  output->concatf("\tmicros():  %lu\n", micros());
}


/**
* Prints details about the cryptographic situation on the platform.
*
* @param  StringBuilder* The buffer to output into.
*/
void AbstractPlatform::printCryptoOverview(StringBuilder* out) {
  #if defined(__HAS_CRYPT_WRAPPER)
    out->concatf("-- Cryptographic support via %s.\n", __CRYPTO_BACKEND);
    int idx = 0;
    #if defined(CONFIG_C3P_MBEDTLS) && defined(MBEDTLS_SSL_TLS_C)
      out->concat("-- Supported TLS ciphersuites:");
      const int* cs_list = mbedtls_ssl_list_ciphersuites();
      while (0 != *(cs_list)) {
        if (0 == idx++ % 2) out->concat("\n--\t");
        out->concatf("\t%-40s", mbedtls_ssl_get_ciphersuite_name(*(cs_list++)));
      }
    #endif  // CONFIG_C3P_MBEDTLS

    out->concat("\n-- Supported ciphers:");
    idx = 0;
    Cipher* list = list_supported_ciphers();
    while (Cipher::NONE != *(list)) {
      if (0 == idx++ % 4) out->concat("\n--\t");
      out->concatf("\t%-20s", get_cipher_label((Cipher) *(list++)));
    }

    out->concat("\n-- Supported ECC curves:");
    CryptoKey* k_list = list_supported_curves();
    idx = 0;
    while (CryptoKey::NONE != *(k_list)) {
      if (0 == idx++ % 4) out->concat("\n--\t");
      out->concatf("\t%-20s", get_pk_label((CryptoKey) *(k_list++)));
    }

    out->concat("\n-- Supported digests:");
    idx = 0;
    Hashes* h_list = list_supported_digests();
    while (Hashes::NONE != *(h_list)) {
      if (0 == idx++ % 6) out->concat("\n--\t");
      out->concatf("\t%-10s", get_digest_label((Hashes) *(h_list++)));
    }
  #else
    out->concat("No cryptographic support.\n");
  #endif  // __HAS_CRYPT_WRAPPER
}


/*******************************************************************************
* Weak-references to placate builds that don't use AbstractPlatform.
* See notes associated with each function in AbstractPlatform.h.
*******************************************************************************/
void   __attribute__((weak)) unsetPinFxn(uint8_t pin) {}
int8_t __attribute__((weak)) setPinFxn(uint8_t pin, IRQCondition condition, FxnPointer fxn) {   return -1;  }
int8_t __attribute__((weak)) pinMode(uint8_t, GPIOMode) {   return -1;       }
int8_t __attribute__((weak)) setPin(uint8_t, bool) {        return -1;       }
int8_t __attribute__((weak)) readPin(uint8_t) {             return -1;       }
int8_t __attribute__((weak)) analogWrite(uint8_t pin, float val) {               return -1;  }
int8_t __attribute__((weak)) analogWriteFrequency(uint8_t pin, uint32_t freq) {  return -1;  }
void   __attribute__((weak)) c3p_log(uint8_t, const char*, StringBuilder*) {   }
AbstractPlatform* __attribute__((weak)) platformObj() {     return nullptr;  }

/*
* These are weak-referenced, in case the platform wants to handle timer overflow
*   in its own manner. Without such explicit control, the firmware system time
*   will simply overflow the range of the integer used to store it.
*
* TODO: It might be worth it to loosen atomic micros()/millis() requirements (or
*   make their 32-bit implementatitons more complicated). The other option is
*   making the entire system time 64-bit, even on 32-bit builds.
*   As it stands, 32-bit roll-over of micros() happens in about 71.5 minutes.
*   I think it is safe to assume that extending this field is not running from a
*   problem that will eventually catch us again. No one expects system time to
*   legitimately run to 584,542 years (64-bit micros() roll-over). So we could
*   solve the problem everywhere (permanently), and lose the annoying integer
*   length ambiguities surrounding system time by doubling the storage and
*   access requirements for 32-bit platforms.
*/
long unsigned __attribute__((weak)) millis_since(const long unsigned MARK) {
  const long unsigned NOW = millis();
  if (4 < sizeof(long unsigned)) {  return delta_assume_wrap((uint64_t) NOW, (uint64_t) MARK);  }
  else {                            return delta_assume_wrap((uint32_t) NOW, (uint32_t) MARK);  }
}

long unsigned __attribute__((weak)) micros_since(const long unsigned MARK) {
  const long unsigned NOW = micros();
  if (4 < sizeof(long unsigned)) {  return delta_assume_wrap((uint64_t) NOW, (uint64_t) MARK);  }
  else {                            return delta_assume_wrap((uint32_t) NOW, (uint32_t) MARK);  }
}

long unsigned __attribute__((weak)) millis_until(const long unsigned MARK) {
  const long unsigned NOW = millis();
  if (4 < sizeof(long unsigned)) {  return delta_assume_wrap((uint64_t) MARK, (uint64_t) NOW);  }
  else {                            return delta_assume_wrap((uint32_t) MARK, (uint32_t) NOW);  }
}

long unsigned __attribute__((weak)) micros_until(const long unsigned MARK) {
  const long unsigned NOW = micros();
  if (4 < sizeof(long unsigned)) {  return delta_assume_wrap((uint64_t) MARK, (uint64_t) NOW);  }
  else {                            return delta_assume_wrap((uint32_t) MARK, (uint32_t) NOW);  }
}


/*******************************************************************************
* Console callbacks
*******************************************************************************/

/**
* @page console-handlers
* @section gpio-tools MCU GPIO tools
*
* This is the console command for directly dealing with MCU GPIO pins.
*
* @subsection cmd-actions Actions
*
* Action    | Description | Additional arguments
* --------- | ----------- | --------------------
* `mode`    | Set the pin mode. | <pin> <new-mode>
* `val`     | Renders pin value to the console, or set the value. | <pin> [new-value]
*
* @subsection arguments Arguments
* Argument  | Purpose   | Required
* --------- | --------- | --------
* pin       | Interger value of the refrenced pin | No
* new-mode  | Interger value (0-8) indcating the mode to switch in to. | No
* new-value | 1 = on, 0 = 0ff | No
*
* For the "mode" action, the pin adn new-mode arguments are not required.  \n
*   If either are not proivided a full list of pins with names is printed to the console. \n
*
* For the "val" action, the new-value argument is optional, and can be either 0 or 1. \n
*   If omitted, the pin will be read, and its state rendered to the console. \n
*   If provided, the handler will try to set the pin to the given binary value. \n
*
* @subsection pin-modes Pin Modes
* Enum Value      | Interger Value
* --------------- | --------------
* INPUT           | 0
* OUTPUT          | 1
* INPUT_PULLUP    | 2
* INPUT_PULLDOWN  | 3
* OUTPUT_OD       | 4
* BIDIR_OD        | 5
* BIDIR_OD_PULLUP | 6
* ANALOG_OUT      | 7
* ANALOG_IN       | 8
*
*/
int callback_gpio_value(StringBuilder* text_return, StringBuilder* args) {
  int ret = 0;
  char* cmd = args->position_trimmed(0);
  int arg0  = args->position_as_int(1);
  int arg1  = args->position_as_int(2);

  if (0 == StringBuilder::strcasecmp(cmd, "mode")) {
    switch (args->count()) {
      case 3:
        switch ((GPIOMode) arg1) {
          case GPIOMode::INPUT:
          case GPIOMode::OUTPUT:
          case GPIOMode::INPUT_PULLUP:
          case GPIOMode::INPUT_PULLDOWN:
          case GPIOMode::OUTPUT_OD:
          case GPIOMode::BIDIR_OD:
          case GPIOMode::BIDIR_OD_PULLUP:
          case GPIOMode::ANALOG_OUT:
          case GPIOMode::ANALOG_IN:
            text_return->concatf("pinMode(%u, %s) Returns %d.\n", arg0, getPinModeStr((GPIOMode) arg1), pinMode(arg0, (GPIOMode)arg1));
            break;
          default:
            text_return->concat("Invalid GPIO mode.\n");
            break;
        }
        break;
      default:
        text_return->concatf("%u: %s\n", (uint8_t) GPIOMode::INPUT,           getPinModeStr(GPIOMode::INPUT));
        text_return->concatf("%u: %s\n", (uint8_t) GPIOMode::OUTPUT,          getPinModeStr(GPIOMode::OUTPUT));
        text_return->concatf("%u: %s\n", (uint8_t) GPIOMode::INPUT_PULLUP,    getPinModeStr(GPIOMode::INPUT_PULLUP));
        text_return->concatf("%u: %s\n", (uint8_t) GPIOMode::INPUT_PULLDOWN,  getPinModeStr(GPIOMode::INPUT_PULLDOWN));
        text_return->concatf("%u: %s\n", (uint8_t) GPIOMode::OUTPUT_OD,       getPinModeStr(GPIOMode::OUTPUT_OD));
        text_return->concatf("%u: %s\n", (uint8_t) GPIOMode::BIDIR_OD,        getPinModeStr(GPIOMode::BIDIR_OD));
        text_return->concatf("%u: %s\n", (uint8_t) GPIOMode::BIDIR_OD_PULLUP, getPinModeStr(GPIOMode::BIDIR_OD_PULLUP));
        text_return->concatf("%u: %s\n", (uint8_t) GPIOMode::ANALOG_OUT,      getPinModeStr(GPIOMode::ANALOG_OUT));
        text_return->concatf("%u: %s\n", (uint8_t) GPIOMode::ANALOG_IN,       getPinModeStr(GPIOMode::ANALOG_IN));
        break;
    }
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "val")) {
    text_return->concatf("GPIO %d ", arg0);
    switch (args->count()) {
      case 3:
        {
          int8_t ret0 = setPin(arg0, (0 != arg1));
          text_return->concatf("set to %s. Returns %d.\n", (0 != arg1) ? "high" : "low", ret0);
        }
        break;
      default:
        {
          int8_t ret0 = readPin(arg0);
          text_return->concatf("reads %s.\n", ret0 ? "high" : "low");
        }
        break;
    }
  }
  else {
    ret = -1;
  }
  return ret;
}


int callback_platform_info(StringBuilder* text_return, StringBuilder* args) {
  char* group = args->position_trimmed(0);
  switch (args->count()) {
    case 1:
      if (0 == StringBuilder::strcasecmp(group, "crypto")) {
        platformObj()->printCryptoOverview(text_return);
      }
      break;
    default:
      platformObj()->printDebug(text_return);
      break;
  }
  return 0;
}


int callback_reboot(StringBuilder* text_return, StringBuilder* args) {
  platformObj()->firmware_reset(args->position_as_int(0));
  return 0;
}


/**
* The application would optionally call this function with a console handler to
*   add the platform functions above. This overhead should be removed from the
*   binary if the application never adds these (occasionally) helpful commands.
*
* @param console is the application-created object for handling consoles.
* @return 0 always.
*/
int8_t AbstractPlatform::configureConsole(C3PConsole* console) {
  #if defined(CONFIG_C3P_CONSOLE_GPIO_TOOL)
    console->defineCommand("gpio",   '\0', "GPIO values", "[val|mode] [pin] [value]", 2, callback_gpio_value);
  #endif
  #if defined(CONFIG_C3P_CONSOLE_PFINFO_TOOL)
    console->defineCommand("pfinfo", '\0', "Platform information", "[types | crypto]", 0, callback_platform_info);
  #endif
  #if defined(CONFIG_C3P_CONSOLE_REBOOT_TOOL)
    console->defineCommand("reboot", '\0', "Reboot firmware", "[reason code]", 0, callback_reboot);
  #endif
  return 0;
}



/*******************************************************************************
* Basic logger support
*******************************************************************************/
/**
* String conversion function to render syslog-style severity codes for humans.
* This is a concealed implementation detail.
*
* @param severity is the syslog-style importance of a message.
* @return a constant string representing the severity code.
*/
const char* c3p_log_severity_string(const uint8_t severity) {
  switch (severity) {
    case LOG_LEV_EMERGENCY:   return "EMERGENCY ";
    case LOG_LEV_ALERT:       return "ALERT     ";
    case LOG_LEV_CRIT:        return "CRITICAL  ";
    case LOG_LEV_ERROR:       return "ERROR     ";
    case LOG_LEV_WARN:        return "WARNING   ";
    case LOG_LEV_NOTICE:      return "NOTICE    ";
    case LOG_LEV_INFO:        return "INFO      ";
    default: break;
  }
  return "DEBUG     ";  // All severity greater than INFO is DEBUG.
}


/**
* This function is a convenience wrapper around the StringBuilder variant of
*   c3p_log(), which is the root implementation given by platform.
* It converts variadic form into a discrete parameter list, uses the
*   StringBuilder API to render it, and (if successfully rendered) shunts it to
*   the root implementaion of c3p_log() that is optionally provided by the
*   application or platform.
*
* @param severity is the syslog-style importance of the message.
* @param tag is the free-form source of the message.
* @param fmt is the printf-style formatting string.
* @param ... are the optional variadics.
* @return 0 on log acceptance.
*/
void c3p_log(uint8_t severity, const char* tag, const char* fmt, ...) {
  StringBuilder msg;
  va_list args;
  va_start(args, fmt);
  int8_t ret = (0 <= msg.concatf(fmt, args)) ? 0:-1;
  va_end(args);
  if (0 == ret) {
    c3p_log(severity, tag, &msg);
  }
}


/**
* For systems that don't have logging faculties, this function will provide it.
*
* @param severity is the syslog-style importance of the message.
* @param tag is the free-form source of the message.
* @param msg contains the log content.
* @return 0 on log acceptance.
*/
int8_t C3PLogger::print(uint8_t severity, const char* tag, StringBuilder* msg) {
  int8_t ret = -1;
  if (severity <= _verb_limit) {
    StringBuilder line;
    if (printTime()) {       line.concatf("%10u ", millis());                 }
    if (printSeverity()) {   line.concat(c3p_log_severity_string(severity));  }
    if (printTag()) {
      const uint8_t TAG_LEN = (uint8_t) strlen(tag);
      if (_tag_ident < LOG_TAG_MAX_LEN) {
        _tag_ident = strict_min(_tag_ident, strict_min((uint8_t) LOG_TAG_MAX_LEN, TAG_LEN));
      }
      const uint32_t MAX_FMT_STR_SZ = 12;
      char ufmt_str[MAX_FMT_STR_SZ];
      memset(ufmt_str, 0, MAX_FMT_STR_SZ);
      sprintf(ufmt_str, "%%%ds ", _tag_ident);
      line.concatf(ufmt_str, tag);
    }
    line.concatHandoff(msg);
    line.concat("\n");
    line.string();   // Condense string to a single contiguous allocation.
    _store_or_forward(&line);
    ret = 0;
  }
  return ret;
}


/**
* Append formatted text to the log, buffering if necessary.
* If the BufferAccepter API is being used, this function will forward the buffer
*   onward to the sink, along with any accumulated log ahead of it, ensuring
*   order.
* If no BufferAccepter sink is available, or the sink rejects the buffer, the
*   log will be buffered internally until it is either retrieved by an external
*   call to fetchLog(), or accepted by the sink on a subsequent call to this
*   function.
* TODO: This would be an appropriate place to put constraints on log growth.
*
* @param log_line is the new item to append to the log.
*/
void C3PLogger::_store_or_forward(StringBuilder* log_line) {
  bool store_to_buffer = true;
  if (nullptr != _sink) {
    bool backlog_remaining = !_log.isEmpty();
    if (backlog_remaining) {
      backlog_remaining = (0 != _sink->pushBuffer(&_log));
    }
    if (!backlog_remaining && (0 == _sink->pushBuffer(log_line))) {
      // NOTE: Short-circuit evaluation above is important for ordering.
      store_to_buffer = false;
    }
  }
  if (store_to_buffer) {
    _log.concatHandoff(log_line);
  }
}


/**
* Relinquish to the caller any log buffer we've accumulated. Use-cases that
*   employ the BufferAccepter interface should not call this function.
*
* @param b is the container to receive any accumulated log text.
*/
void C3PLogger::fetchLog(StringBuilder* b) {
  if ((nullptr != b) && (!_log.isEmpty())) b->concatHandoff(&_log);
}
