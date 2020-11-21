/*
* Abstract shape to which platform implementations must conform.
* This is broadly modeled from Arduino to facilitate cross-porting.
*/
#include <inttypes.h>
#include <stdint.h>
#include "AbstractPlatform.h"


/**
* Issue a human-readable string representing the platform state.
*
* @return A string constant.
*/
const char* getPinModeStr(GPIOMode mode) {
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

const char* shutdownCauseStr(ShutdownCause cause) {
  switch (cause) {
    case ShutdownCause::FATAL_ERR:     return "FATAL_ERR";
    case ShutdownCause::USER:          return "USER";
    case ShutdownCause::CONF_RELOAD:   return "CONF_RELOAD";
    case ShutdownCause::REFLASH:       return "REFLASH";
    case ShutdownCause::TIMEOUT:       return "TIMEOUT";
    case ShutdownCause::WATCHDOG:      return "WATCHDOG";
    case ShutdownCause::BROWNOUT:      return "BROWNOUT";
    case ShutdownCause::THERMAL:       return "THERMAL";
    default:  break;
  }
  return "UNSPECIFIED";
}
