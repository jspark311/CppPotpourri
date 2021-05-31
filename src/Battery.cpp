/*
File:   Battery.cpp
Author: J. Ian Lindsay
Date:   2021.05.31  (but assembled from much older ideas)

This is a collection of definitions for managing batteries in firmware.
*/

#include "Battery.h"


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

/**
* Debug and logging support.
*
* @return a const char* containing a human-readable representation of a fault code.
*/
const char* BatteryOpts::batteryStateStr(const ChargeState e) {
  switch (e) {
    case ChargeState::UNKNOWN:   return "UNKNOWN";
    case ChargeState::FULL:      return "FULL";
    case ChargeState::CHARGING:  return "CHARGING";
    case ChargeState::DRAINING:  return "DRAINING";
    case ChargeState::ERROR:     return "ERROR";
    default:                     return "<UNDEF>";
  }
}
