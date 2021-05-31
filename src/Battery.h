/*
File:   Battery.h
Author: J. Ian Lindsay
Date:   2021.05.31  (but assembled from much older ideas)

This is a collection of definitions for managing batteries in firmware.
*/

#include <inttypes.h>
#include <stdint.h>

#ifndef __BATTERIES_INCLUDED_H__
#define __BATTERIES_INCLUDED_H__

/*
* Battery chemistries...
* Different battery chemistries have different discharge curves. This struct is
*   a point on a discharge curve.
* We should build a series of constants of this type for each chemistry we want
*   to support. Since these curves amount to physical constants, they shouldn't
*   ever consume prescious RAM. Keep them isolated to flash.
*/
typedef struct v_cap_point {
  float percent_of_max_voltage;     // The percentage of the battery's stated maximum voltage.
  float capacity_derate;            // The percentage by which to multiply the battery's stated capacity (if you want AH remaining).
} V_Cap_Point;


/*
* An enum for battery chemistries. Many physical parameters of the battery can
*   be determined from accurate information about the chemistry, and some
*   drivers (charging circuits) might require it to function safely.
*/
enum class BatteryChemistry : uint8_t {
  UNDEFINED        = 0,  // Some drivers might refuse to work without a defined chemistry.
  SUPERCAP         = 1,  // A spring so large, we can pretend it is a battery.
  NON_RECHARGABLE  = 2,  // This means alkaline, heavy-duty, lithium coin cells, etc...
  LEAD_ACID        = 3,  // The old standby.
  LEAD_ACID_SEALED = 4,  // If the battery is sealed, extra care needs to be taken to prevent outgassing.
  LEAD_ACID_AGM    = 5,  // Advanced Glass Mat varient of the Pb-H2SO4 chemistry.
  LIPO             = 6,  // Lithium polymer (also Lithium ion)
  LI_PHOSPHATE     = 7,  // Lithium phosphate
  NICAD            = 8   // Nickel cadmium
};


/*
* These are supply-side modes.
*/
enum class ChargeState : uint8_t {
  UNKNOWN,   // Driver is unsure about the state of the battery.
  FULL,      // Implies we are connected to a charging source.
  CHARGING,  // The battery is not full, but it is charging.
  DRAINING,  // No charging source. We are burning fuel.
  ERROR      // Charge error. This means a hardware fault. Absent battery?
};


/* Callback definition for battery state notifications from PMU drivers. */
typedef int8_t (*BatteryStateCallback)(ChargeState);


/*
* A class to identify battery parameters in a device.
*/
class BatteryOpts {
  // TODO: Include cell count parameter and arrive at voltages from first-principles?
  public:
    const BatteryChemistry chemistry;
    const uint16_t capacity;       // The capacity of the battery in mAh.
    const float    voltage_min;    // Voltage where battery is considered dead.
    const float    voltage_weak;   // Voltage where battery is considered weak.
    const float    voltage_float;  // Voltage where battery is considered charged.
    const float    voltage_max;    // Maximum safe voltage.

    BatteryOpts(const BatteryOpts* o) :
      chemistry(o->chemistry),
      capacity(o->capacity),
      voltage_min(o->voltage_min),
      voltage_weak(o->voltage_weak),
      voltage_float(o->voltage_float),
      voltage_max(o->voltage_max) {};

    BatteryOpts(BatteryChemistry chem, uint16_t cap, float v_min, float v_w, float v_f, float v_max) :
      chemistry(chem),
      capacity(cap),
      voltage_min(v_min),
      voltage_weak(v_w),
      voltage_float(v_f),
      voltage_max(v_max) {};

    static const char* batteryStateStr(const ChargeState);
};


#endif  // __BATTERIES_INCLUDED_H__
