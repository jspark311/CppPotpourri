/*
* Abstract shape to which platform implementations must conform.
* This is broadly modeled from Arduino to facilitate cross-porting.
*/
#include <inttypes.h>
#include <stdint.h>
#include "AbstractPlatform.h"
#include "C3PLogger.h"
#include "ParsingConsole.h"

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


/*
* Do the boilerplate setup of the MCU that all applications will require.
*/
int8_t platform_init() {
  if (platformObj()) {
    return platformObj()->init();
  }
  return -127;
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
    uint16_t test = 0xAA55;
    if (0xAA == *((uint8_t*) &test)) {
      _alter_flags(true, ABSTRACT_PF_FLAG_BIG_ENDIAN);
    }
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
    #if defined(WITH_MBEDTLS) && defined(MBEDTLS_SSL_TLS_C)
      out->concat("-- Supported TLS ciphersuites:");
      const int* cs_list = mbedtls_ssl_list_ciphersuites();
      while (0 != *(cs_list)) {
        if (0 == idx++ % 2) out->concat("\n--\t");
        out->concatf("\t%-40s", mbedtls_ssl_get_ciphersuite_name(*(cs_list++)));
      }
    #endif

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
  #endif  // WITH_MBEDTLS
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
void __attribute__((weak)) c3p_log(uint8_t, const char*, const char*, ...) { }
void __attribute__((weak)) c3p_log(uint8_t, const char*, StringBuilder*) {   }
AbstractPlatform* __attribute__((weak)) platformObj() {     return nullptr;  }


/*******************************************************************************
* Console callbacks
*******************************************************************************/

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
      if (0 == StringBuilder::strcasecmp(group, "types")) {
        text_return->concatf("ParsingConsole   %u\t%u\n", sizeof(ParsingConsole), alignof(ParsingConsole));
        text_return->concatf("ConsoleCommand   %u\t%u\n", sizeof(ConsoleCommand), alignof(ConsoleCommand));
        text_return->concatf("StringBuilder    %u\t%u\n", sizeof(StringBuilder), alignof(StringBuilder));
      }
      #if defined(__HAS_CRYPT_WRAPPER)
      else if (0 == StringBuilder::strcasecmp(group, "crypto")) {
        platformObj()->crypto.printDebug(text_return);
      }
      #endif
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


const ConsoleCommand cmd00 = ConsoleCommand("gpio",   '\0', ParsingConsole::tcodes_str_3,  "GPIO values", "[val|mode] [pin] [value]", 2, callback_gpio_value);
const ConsoleCommand cmd01 = ConsoleCommand("pfinfo", '\0', ParsingConsole::tcodes_str_1,  "Platform information", "[subgroup]", 0, callback_platform_info);
const ConsoleCommand cmd02 = ConsoleCommand("reboot", '\0', ParsingConsole::tcodes_uint_1, "Reboot firmware", "[subgroup]", 0, callback_reboot);


int8_t AbstractPlatform::configureConsole(ParsingConsole* console) {
  console->defineCommand(&cmd00);
  console->defineCommand(&cmd01);
  console->defineCommand(&cmd02);
  return 0;
}



/*******************************************************************************
* Basic logger support
*******************************************************************************/
const char* severityStr(const uint8_t severity) {
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


/*
* Relinquish any log buffer we've built up to the caller. Use-cases that employ
*   the BufferAccepter interface should not call this function.
*/
void C3PLogger::fetchLog(StringBuilder* b) {
  if ((nullptr != b) && (!_log.isEmpty())) b->concatHandoff(&_log);
}


/**
* @return 0 on log acceptance.
*/
int8_t C3PLogger::print(uint8_t severity, const char* tag, const char* fmt, ...) {
  int8_t ret = -1;
  if (severity <= _verb_limit) {
    const int FMT_LEN = strlen(fmt);
    uint8_t f_codes = 0;
    StringBuilder msg;
    // Count how many format codes are in use...
    for (unsigned short i = 0; i < FMT_LEN; i++) {  if (*(fmt+i) == '%') f_codes++; }
    // Allocate (hopefully) more space than we will need....
    int est_len = FMT_LEN + 300 + (f_codes * 15);   // TODO: Iterate on failure of vsprintf().
    va_list args;
    char* temp = (char *) alloca(est_len);  // Allocate (hopefully) more space than we will need....
    memset(temp, 0, est_len);
    va_start(args, fmt);
    if (0 <= vsprintf(temp, fmt, args)) {
      msg.concat(temp);
      ret = 0;
    }
    va_end(args);

    if (0 == ret) {
      ret = this->print(severity, tag, &msg);
    }
  }
  return ret;
}


/**
* This is the ultimate destination for log taken by the variadic function.
*
* @return 0 on log acceptance.
*/
int8_t C3PLogger::print(uint8_t severity, const char* tag, StringBuilder* msg) {
  int8_t ret = -1;
  if (severity <= _verb_limit) {
    StringBuilder line;
    if (printTime()) {          line.concatf("%10u ", millis());       }
    if (printSeverity()) {      line.concat(severityStr(severity));    }
    if (printTag()) {
      const uint8_t TAG_LEN = (uint8_t) strlen(tag);
      if (_tag_ident > LOG_TAG_MAX_LEN) {
        _tag_ident = strict_min(_tag_ident, strict_min((uint8_t) LOG_TAG_MAX_LEN, TAG_LEN));
      }
      char* ufmt_str = (char *) alloca(12);
      ufmt_str[0] = '%';
      memset(ufmt_str+1, 0, 12);
      sprintf(ufmt_str+1, "%ds ", _tag_ident);
      line.concatf(ufmt_str, tag);
    }
    line.concatHandoff(msg);
    line.concat("\n");
    line.string();   // Condense string.
    _store_or_forward(&line);
    ret = 0;
  }
  return ret;
}


void C3PLogger::_store_or_forward(StringBuilder* log_line) {
  bool store_to_buffer = true;
  if (nullptr != _sink) {
    bool backlog_remaining = !_log.isEmpty();
    if (backlog_remaining) {
      backlog_remaining = (0 != _sink->provideBuffer(&_log));
    }
    if (!backlog_remaining && (0 == _sink->provideBuffer(log_line))) {
      // NOTE: Short-circuit evaluation above is important for ordering.
      store_to_buffer = false;
    }
  }
  if (store_to_buffer) {
    _log.concatHandoff(log_line);
  }
}
