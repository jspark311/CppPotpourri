#include "../../CppPotpourri.h"
#include "../../AbstractPlatform.h"
#include "TripleAxisPipe.h"


#define TRIPAX_ORIENT_FLAG_PENDING_ZERO         0x00000001  // Pending re-zero from upstream.
#define TRIPAX_ORIENT_FLAG_VALUE_DIRTY          0x00000002  // Unread data is available.


/*******************************************************************************
* Overrides from TripleAxisPipe
*******************************************************************************/

/*
* Behavior: If sense parameter represents usable data, refreshes
*   this instance's state and calls callback (if defined) when the compass
*   changes. Marks the data as fresh if the callback is either absent, or
*   returns nonzero.
*
* @return 0 on acceptance, or -1 on sense mis-match.
*/
int8_t TripleAxisOrientation::pushVector(const SpatialSense s, Vector3f* data, Vector3f* error, uint32_t seq_num) {
  uint32_t tmp_millis = millis();
  int8_t ret = 0;
  switch (s) {
    case SpatialSense::ACC:        // A gravity vector.
      {
        if (nullptr != error) {
          _ERR_ACC.set(error);
        }
        if (_flags.value(TRIPAX_ORIENT_FLAG_PENDING_ZERO)) {
          _up.set(data);
          _flags.clear(TRIPAX_ORIENT_FLAG_PENDING_ZERO);
        }
        Vector3f plane_xz(0.0, 1.0, 0.0);  // Sagittal plane.
        Vector3f plane_yz(1.0, 0.0, 0.0);  // Coronal plane.
        Vector3f proj_up_xz   = _up - _up.projected(plane_xz);      // Project the up vector onto sagittal plane.
        Vector3f proj_up_yz   = _up - _up.projected(plane_yz);      // Project the up vector onto coronal plane.
        Vector3f proj_vect_xz = *data - data->projected(plane_xz);  // Project the vector onto sagittal plane.
        Vector3f proj_vect_yz = *data - data->projected(plane_yz);  // Project the vector onto coronal plane.
        //proj_up_xz.normalize(PI);
        //proj_up_yz.normalize(PI);
        //proj_vect_xz.normalize(PI);
        //proj_vect_yz.normalize(PI);

        _gravity.set(
          //Vector3<float>::angle(proj_up_yz, proj_vect_yz),  // Roll
          //Vector3<float>::angle(proj_up_xz, proj_vect_xz),  // Pitch
          //atanf((proj_up_yz % proj_vect_yz).length() / (proj_up_yz * proj_vect_yz)),  // Roll
          //atanf((proj_up_xz % proj_vect_xz).length() / (proj_up_xz * proj_vect_xz)),  // Pitch
          atan2(proj_vect_yz.y, proj_vect_yz.z) - atan2(proj_up_yz.y, proj_up_yz.z),  // Roll
          atan2(proj_vect_xz.x, proj_vect_xz.z) - atan2(proj_up_xz.x, proj_up_xz.z),  // Pitch
          0   // Unless we have a magnetometer or bearing, we can't track yaw.
        );
        _data_period = millis_since(_last_update);
        _last_update = tmp_millis;
        _update_count++;
        if (nullptr != _NXT) {
          // If the vector was pushed downstream, we consider it noted. If it
          //   was rejected, we leave the class marked dirty.
          //_filter_dirty = (0 > _NXT->pushVector(SpatialSense::EULER_ANG, &_gravity, &_ERR));
          ret = _NXT->pushVector(SpatialSense::EULER_ANG, &_gravity, &_ERR_ACC, seq_num);
        }
        if (!_flags.value(TRIPAX_ORIENT_FLAG_VALUE_DIRTY)) {
          _flags.set(TRIPAX_ORIENT_FLAG_VALUE_DIRTY, (0 > ret));
        }
      }
      break;

    case SpatialSense::MAG:        // A field vector.
      if (nullptr != error) {
        _ERR_MAG.set(error);
      }
      break;

    case SpatialSense::EULER_ANG:  // An orientation.
      break;

    case SpatialSense::GYR:        // Ignored by this class.
    case SpatialSense::UNITLESS:   // wat.
    default:
      ret = -1;
  }
  return ret;
}


/*******************************************************************************
* Functions to output things to the console
*******************************************************************************/

void TripleAxisOrientation::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  StringBuilder indent;
  for (uint8_t i = 0; i < stage; i++) {    indent.concat("    ");    }
  output->concatf("%s+-< 3AxisPipe: Orientation >----------------\n", (char*) indent.string());
  output->concatf("%s| Seq number:     %u\n", (char*) indent.string(), _update_count);
  output->concatf("%s| Last update:    %u\n", (char*) indent.string(), _last_update);
  if (_data_period) {
    output->concatf("%s| Data rate:      %.2f vectors/sec\n", indent.string(), (double) (1000.0 / _data_period));
  }
  output->concatf("%s| Up:    (%.4f, %.4f, %.4f)\n", (char*) indent.string(), (double) _up.x, (double) _up.y, (double) _up.z);
  output->concatf("%s| Pitch: %.4f\n", (char*) indent.string(), (double) _gravity.x);
  output->concatf("%s| Roll:  %.4f\n", (char*) indent.string(), (double) _gravity.y);
}



/*******************************************************************************
* Core class functions
*******************************************************************************/

void TripleAxisOrientation::markLevel() {  _flags.set(TRIPAX_ORIENT_FLAG_PENDING_ZERO);   };
bool TripleAxisOrientation::dirty() {      return _flags.value(TRIPAX_ORIENT_FLAG_VALUE_DIRTY);  };
Vector3f* TripleAxisOrientation::value() {
  _flags.clear(TRIPAX_ORIENT_FLAG_VALUE_DIRTY);
  return &_gravity;
};




/*******************************************************************************
* TripleAxisMadgwick
*
* Taken from
* https://github.com/kriswiner/LSM9DS1/blob/master/Teensy3.1/LSM9DS1-MS5637/quaternionFilters.ino
*
* There is a tradeoff in the beta parameter between accuracy and response speed.
* In the original Madgwick study, beta of 0.041 (corresponding to GyroMeasError of 2.7 degrees/s) was found to give optimal accuracy.
* However, with this value, the LSM9SD0 response time is about 10 seconds to a stable initial quaternion.
* Subsequent changes also require a longish lag time to a stable output, not fast enough for a quadcopter or robot car!
* By increasing beta (GyroMeasError) by about a factor of fifteen, the response time constant is reduced to ~2 sec
* I haven't noticed any reduction in solution accuracy. This is essentially the I coefficient in a PID control sense;
* the bigger the feedback coefficient, the faster the solution converges, usually at the expense of accuracy.
* In any case, this is the free parameter in the Madgwick filtering and fusion scheme.
*const float TripleAxisMadgwick::beta = ((float)sqrt(3.0f / 4.0f)) * TripleAxisMadgwick::GyroMeasError;   // compute beta
*******************************************************************************/

#if 0

/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

TripleAxisMadgwick::TripleAxisMadgwick() : _complete(CONFIG_INTEGRATOR_Q_DEPTH), _pending(CONFIG_INTEGRATOR_Q_DEPTH) {
  // Values for 9 DoF fusion and AHRS (Attitude and Heading Reference System)
  //GyroMeasError = PI * (40.0f / 180.0f);  // gyroscope measurement error in rads/s (shown as 3 deg/s)
  GyroMeasDrift = PI * (0.0f / 180.0f);   // gyroscope measurement drift in rad/s/s (shown as 0.0 deg/s/s)
  //beta = 0.866025404f * (PI * GyroMeasError);   // compute beta
  beta = 0.2f;
}


void TripleAxisMadgwick::reset() {
  delta_t   = 0;
  rangeBind(false);
  _grav.set(0.0f, 0.0f, 0.0f);
  grav_scalar = 0.0f;
}


/*
TODO: DEPRECATED Slow. Too complicated.
*/
//int8_t TripleAxisMadgwick::pushMeasurement(SampleType data_type, float x, float y, float z, float d_t) {
//  uint32_t read_time = micros();
//  switch (data_type) {
//    case SampleType::GYRO:
//      if (_ptr_gyr) _ptr_gyr->set(x, y, z);
//      //if (rangeBind()) {
//      //  last_value_gyr /= LSM9DS1_M::max_range_vect_gyr;
//      //}
//      *(_ptr_s_count_gyr) = *(_ptr_s_count_gyr)+1;
//      dirty_gyr = read_time;
//      delta_t = (d_t > delta_t) ? d_t : delta_t;   // We want the smaller of the two, since it sets the quat rate.
//      break;
//
//    case SampleType::ACCEL:
//      //if (_ptr_acc) _ptr_acc->set(x, y, z);
//      if (_ptr_acc && !nullifyGravity()) _ptr_acc->set(x, y, z);  // If we are nulling gravity, can't do this.
//      //if (rangeBind()) {
//      //  last_value_acc /=  LSM9DS1_AG::max_range_vect_acc;
//      //}
//      *(_ptr_s_count_acc) = *(_ptr_s_count_acc)+1;
//      dirty_acc = read_time;
//      delta_t = (d_t > delta_t) ? d_t : delta_t;   // We want the smaller of the two, since it sets the quat rate.
//      break;
//
//    case SampleType::MAG:
//      //if (rangeBind()) {
//      //  last_value_mag /=  LSM9DS1_AG::max_range_vect_mag;
//      //}
//      if (correctSphericalAbberation()) {
//        if (_ptr_mag) _ptr_mag->set(1.0f, 0.0f, 0.0f);
//        return 0;
//      }
//
//      if (nullifyBearing()) {
//        // Now we need to apply the rotation matrix so that we don't have to care what our bearing is at calibration time.
//        // First, rotate about y.
//        float sin_theta = sin((180.0f/M_PI) - offset_angle_y);
//        float cos_theta = cos((180.0f/M_PI) - offset_angle_y);
//        x = cos_theta * x + sin_theta * z;
//        z = sin_theta * -1.0f * x + cos_theta * z;
//
//        // Then rotate about z.
//        sin_theta = sin((180.0f/M_PI) - offset_angle_z);
//        cos_theta = cos((180.0f/M_PI) - offset_angle_z);
//        x = cos_theta * x + sin_theta * -1.0f * y;
//        y = sin_theta * x + cos_theta * y;
//      }
//
//      if (_ptr_mag) _ptr_mag->set(x, y, z);
//
//      *(_ptr_s_count_mag) = *(_ptr_s_count_mag)+1;
//      dirty_mag = read_time;
//      break;
//
//    case SampleType::GRAVITY:
//      _grav.set(x, y, z);
//      grav_scalar = _grav.length();
//      _grav.normalize();
//      // Now, we should reset the quaternion to reflect our idea of gravity with
//      // whatever our last mag read was as the bearing.
//      if (_ptr_quat) _ptr_quat->setDown(x, y, z);
//      break;
//
//    case SampleType::BEARING:
//      if (grav_scalar != 0.0f) {  // We need the gravity vector to do this trick.
//        Vector3<float> mag_original_bearing(x, y, z);
//        mag_original_bearing.normalize();
//        offset_angle_y = y;
//        offset_angle_z = z;
//        // Now, we should reset the quaternion to reflect our idea of gravity with
//        // whatever our last mag read was as the bearing.
//
//      }
//      break;
//
//    default:
//      if (verbosity > 3) {
//        local_log.concat("TripleAxisMadgwick::pushMeasurement(): \t Unhandled Measurement type.\n");
//        Kernel::log(&local_log);
//      }
//      break;
//  }
//
//  if (processQuats() && isQuatDirty()) {
//    if (0 == _pending.count()) {
//      // There was nothing in this queue, but there is about to be.
//    }
//    //SensorFrame* nu_measurement = fetchMeasurement();
//    ////nu_measurement->set(_ptr_gyr, ((!dirty_mag && cleanMagZero()) ? (Vector3<float>*)&ZERO_VECTOR : _ptr_mag), _ptr_acc, delta_t);
//    //if (dirty_mag) dirty_mag = 0;
//    //dirty_acc = 0;
//    //dirty_gyr = 0;
//    //_pending.insert(nu_measurement);
//  }
//
//  return 0;
//}

/**
* This is the function that the processing thread (if any) would call repeatedly.
*
* @return The number of SensorFrames completed.
*/
int8_t TripleAxisMadgwick::churn() {
  int8_t return_value = 0;
  if ((0 < _complete.vacancy()) && MadgwickQuaternionUpdate()) {
    return_value++;
  }
  return return_value;
}


bool TripleAxisMadgwick::enableProfiling(bool en) {
  if (enableProfiling() != en) {
    data_handling_flags = (en) ? (data_handling_flags | IIU_DATA_HANDLING_PROFILING) : (data_handling_flags & ~(IIU_DATA_HANDLING_PROFILING));
  }
  return enableProfiling();
}


bool TripleAxisMadgwick::nullGyroError(bool en) {
  if (nullGyroError() != en) {
    data_handling_flags = (en) ? (data_handling_flags | IIU_DATA_HANDLING_NULL_GYRO_ERROR) : (data_handling_flags & ~(IIU_DATA_HANDLING_NULL_GYRO_ERROR));
  }
  return nullGyroError();
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void TripleAxisMadgwick::printPipe(StringBuilder* output, uint8_t stage, uint8_t verbosity) {
  output->concat("\n-------------------------------------------------------\n--- Integrator\n-------------------------------------------------------\n");
  output->concatf("-- Samples:\t %u\n-- Measurements\n\t_pending:   %u\n\t_complete:  %u\n", _frames_completed, _pending.count(), _complete.count());
  output->concatf("-- delta_t:\t %3fms\n", ((double) delta_t * 1000));
  if (verbosity > 2) {
    if (verbosity > 3) output->concatf("-- GyroMeasDrift:    %.4f\n",  (double) GyroMeasDrift);
    output->concatf("-- Gravity: %.4G (%.4f, %.4f, %.4f)\n", (double) (grav_scalar), (double)(_grav.x), (double)(_grav.y), (double)(_grav.z));
  }
  output->concat("\n");
}



/**
* This ought to be the only place where we promote vectors into the last_read position. Otherwise, there
*   shall be chaos as several different systems rely on that data member being synchronized WRT to the _ptr_quat->
*/
uint8_t TripleAxisMadgwick::MadgwickQuaternionUpdate() {
  SensorFrame* c_frame = _pending.get();
  if (c_frame) {
    float d_t = c_frame->time();

    #if defined(MANUVR_DEBUG)
    if (verbosity > 6) {
      local_log.concatf("At delta-t = %f: ", (double)d_t);
        //c_frame->printDebug(&local_log);
        //local_log.concat("\t");
        //c_frame->quats[set_i]->printDebug(&local_log);
      local_log.concat("\n");
      Kernel::log(&local_log);
    }
    #endif

    float q0;
    float q1;
    float q2;
    float q3;

    float norm;
    float s0, s1, s2, s3;
    float qDot1, qDot2, qDot3, qDot4;
    float hx, hy;
    float _2q0mx, _2q0my, _2q0mz, _2q1mx, _2bx, _2bz, _4bx, _4bz, _2q0, _2q1, _2q2, _2q3, q0q0, q0q1, q0q2, q0q3, q1q1, q1q2, q1q3, q2q2, q2q3, q3q3;

    // Normalise mag c_frame
    float mag_normal;

    // Now we'll start the float churn...
    for (uint8_t set_i = 0; set_i < LEGEND_DATASET_IIU_COUNT; set_i++) {
      q0 = c_frame->quat[set_i].w;
      q1 = c_frame->quat[set_i].x;
      q2 = c_frame->quat[set_i].y;
      q3 = c_frame->quat[set_i].z;
      mag_normal = (c_frame->m_data[set_i]).normalize();

      if (dropObviousBadMag() && (mag_normal >= mag_discard_threshold)) {
        // We defer to the algorithm that does not use the non-earth mag data.
        for (int i = 0; i < madgwick_iterations; i++) {
          MadgwickAHRSupdateIMU(c_frame);
        }
      }
      else if (0.0f == mag_normal) {
        // We defer to the algorithm that does not use the absent mag data.
        for (int i = 0; i < madgwick_iterations; i++) {
          MadgwickAHRSupdateIMU(c_frame);
        }
      }
      else {
        float gx = (c_frame->g_data[set_i]).x * IIU_DEG_TO_RAD_SCALAR;
        float gy = (c_frame->g_data[set_i]).y * IIU_DEG_TO_RAD_SCALAR;
        float gz = (c_frame->g_data[set_i]).z * IIU_DEG_TO_RAD_SCALAR;

        for (int i = 0; i < madgwick_iterations; i++) {
          // Rate of change of quaternion from gyroscope
          qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
          qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
          qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
          qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

          // Normalise accelerometer c_frame-> If vector is non-zero, integrate it...
          if (0.0f != (c_frame->a_data[set_i]).normalize()) {
            float mx = (c_frame->m_data[set_i]).x;
            float my = (c_frame->m_data[set_i]).y;
            float mz = (c_frame->m_data[set_i]).z;

            float ax = (c_frame->a_data[set_i]).x;
            float ay = (c_frame->a_data[set_i]).y;
            float az = (c_frame->a_data[set_i]).z;

            // Auxiliary variables to avoid repeated arithmetic
            _2q0mx = 2.0f * q0 * mx;
            _2q0my = 2.0f * q0 * my;
            _2q0mz = 2.0f * q0 * mz;
            _2q1mx = 2.0f * q1 * mx;
            _2q0 = 2.0f * q0;
            _2q1 = 2.0f * q1;
            _2q2 = 2.0f * q2;
            _2q3 = 2.0f * q3;
            q0q0 = q0 * q0;
            q0q1 = q0 * q1;
            q0q2 = q0 * q2;
            q0q3 = q0 * q3;
            q1q1 = q1 * q1;
            q1q2 = q1 * q2;
            q1q3 = q1 * q3;
            q2q2 = q2 * q2;
            q2q3 = q2 * q3;
            q3q3 = q3 * q3;

            // Reference direction of Earth's magnetic field
            hx = mx * q0q0 - _2q0my * q3 + _2q0mz * q2 + mx * q1q1 + _2q1 * my * q2 + _2q1 * mz * q3 - mx * q2q2 - mx * q3q3;
            hy = _2q0mx * q3 + my * q0q0 - _2q0mz * q1 + _2q1mx * q2 - my * q1q1 + my * q2q2 + _2q2 * mz * q3 - my * q3q3;
            _2bx = sqrt(hx * hx + hy * hy);
            _2bz = -_2q0mx * q2 + _2q0my * q1 + mz * q0q0 + _2q1mx * q3 - mz * q1q1 + _2q2 * my * q3 - mz * q2q2 + mz * q3q3;
            _4bx = 2.0f * _2bx;
            _4bz = 2.0f * _2bz;
            float _8bx = 2.0f * _4bx;
            float _8bz = 2.0f * _4bz;

            // Gradient decent algorithm corrective step
            s0 = -_2q2*(2*(q1q3 - q0q2) - ax) + _2q1*(2*(q0q1 + q2q3) - ay) +  -_4bz*q2*(_4bx*(0.5 - q2q2 - q3q3) + _4bz*(q1q3 - q0q2) - mx)   +   (-_4bx*q3+_4bz*q1)*(_4bx*(q1q2 - q0q3) + _4bz*(q0q1 + q2q3) - my)    +   _4bx*q2*(_4bx*(q0q2 + q1q3) + _4bz*(0.5 - q1q1 - q2q2) - mz);
            s1 = _2q3*(2*(q1q3 - q0q2) - ax)  + _2q0*(2*(q0q1 + q2q3) - ay) + -4*q1*(2*(0.5 - q1q1 - q2q2) - az)    +   _4bz*q3*(_4bx*(0.5 - q2q2 - q3q3) + _4bz*(q1q3 - q0q2) - mx)   + (_4bx*q2+_4bz*q0)*(_4bx*(q1q2 - q0q3) + _4bz*(q0q1 + q2q3) - my)   +   (_4bx*q3-_8bz*q1)*(_4bx*(q0q2 + q1q3) + _4bz*(0.5 - q1q1 - q2q2) - mz);
            s2 = -_2q0*(2*(q1q3 - q0q2) - ax) + _2q3*(2*(q0q1 + q2q3) - ay) + (-4*q2)*(2*(0.5 - q1q1 - q2q2) - az) +   (-_8bx*q2-_4bz*q0)*(_4bx*(0.5 - q2q2 - q3q3) + _4bz*(q1q3 - q0q2) - mx)+(_4bx*q1+_4bz*q3)*(_4bx*(q1q2 - q0q3) + _4bz*(q0q1 + q2q3) - my)+(_4bx*q0-_8bz*q2)*(_4bx*(q0q2 + q1q3) + _4bz*(0.5 - q1q1 - q2q2) - mz);
            s3 = _2q1*(2*(q1q3 - q0q2) - ax)  + _2q2*(2*(q0q1 + q2q3) - ay) + (-_8bx*q3+_4bz*q1)*(_4bx*(0.5 - q2q2 - q3q3) + _4bz*(q1q3 - q0q2) - mx)+(-_4bx*q0+_4bz*q2)*(_4bx*(q1q2 - q0q3) + _4bz*(q0q1 + q2q3) - my)+(_4bx*q1)*(_4bx*(q0q2 + q1q3) + _4bz*(0.5 - q1q1 - q2q2) - mz);

            norm = 1.0f / (float) sqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude

            // Apply feedback step
            qDot1 -= beta * (s0 * norm);
            qDot2 -= beta * (s1 * norm);
            qDot3 -= beta * (s2 * norm);
            qDot4 -= beta * (s3 * norm);

            // Integrate rate of change of quaternion to yield quaternion
            q0 += qDot1 * d_t;
            q1 += qDot2 * d_t;
            q2 += qDot3 * d_t;
            q3 += qDot4 * d_t;

            // Normalise quaternion
            norm = 1.0f / (float) sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q0 * q0);    // normalise quaternion
            q0 = q0 * norm;
            q1 = q1 * norm;
            q2 = q2 * norm;
            q3 = q3 * norm;

            c_frame->setO(set_i, q0, q1, q2, q3);
          }
        }
      }

      if (c_frame->accNullGravity(set_i)) {
        /* If we are going to cancel gravity, we should do so now. */
        _grav.x = (2 * (q1 * q3 - q0 * q2));
        _grav.y = (2 * (q0 * q1 + q2 * q3));
        _grav.z = (q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3);

        c_frame->setN(set_i,
          (c_frame->a_data[set_i]).x - _grav.x,
          (c_frame->a_data[set_i]).y - _grav.y,
          (c_frame->a_data[set_i]).z - _grav.z
        );

        if (c_frame->velocity(set_i)) {
          // Are we finding velocity?
          c_frame->setV(set_i,
            c_frame->n_data[set_i].x * d_t,
            c_frame->n_data[set_i].y * d_t,
            c_frame->n_data[set_i].z * d_t
          );

          if (c_frame->position(set_i)) {
            // Track position....
            c_frame->setP(set_i,
              c_frame->v_data[set_i].x * d_t,
              c_frame->v_data[set_i].y * d_t,
              c_frame->v_data[set_i].z * d_t
            );
          }
        }
      }
    }
    c_frame->markComplete();
    if (_complete.insert(c_frame)) {
      local_log.concat("Dropped a frame in the integrator. This is probably a leak.\n");
    }
    _frames_completed++;
  }

  if (local_log.length() > 0) Kernel::log(&local_log);
  return 1;
}

//---------------------------------------------------------------------------------------------------
// IMU algorithm update

void TripleAxisMadgwick::MadgwickAHRSupdateIMU(SensorFrame* c_frame) {
  float norm;
  float s0, s1, s2, s3;
  float qDot1, qDot2, qDot3, qDot4;
  float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2 ,_8q1, _8q2, q0q0, q1q1, q2q2, q3q3;

  float q0;
  float q1;
  float q2;
  float q3;

  float gx;
  float gy;
  float gz;

  for (int set_i = 0; set_i < LEGEND_DATASET_IIU_COUNT; set_i++) {
    q0 = c_frame->quat[set_i].w;
    q1 = c_frame->quat[set_i].x;
    q2 = c_frame->quat[set_i].y,
    q3 = c_frame->quat[set_i].z;

    gx = (c_frame->g_data[set_i]).x * IIU_DEG_TO_RAD_SCALAR;
    gy = (c_frame->g_data[set_i]).y * IIU_DEG_TO_RAD_SCALAR;
    gz = (c_frame->g_data[set_i]).z * IIU_DEG_TO_RAD_SCALAR;
    float d_t = c_frame->time();

    // Rate of change of quaternion from gyroscope
    qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
    qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
    qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

    // Normalise accelerometer c_frame-> If vector is non-zero, integrate it...
    if (0.0f != (c_frame->a_data[set_i]).normalize()) {
      float ax = (c_frame->a_data[set_i]).x;
      float ay = (c_frame->a_data[set_i]).y;
      float az = (c_frame->a_data[set_i]).z;
      // Auxiliary variables to avoid repeated arithmetic
      _2q0 = 2.0f * q0;
      _2q1 = 2.0f * q1;
      _2q2 = 2.0f * q2;
      _2q3 = 2.0f * q3;
      _4q0 = 4.0f * q0;
      _4q1 = 4.0f * q1;
      _4q2 = 4.0f * q2;
      _8q1 = 8.0f * q1;
      _8q2 = 8.0f * q2;
      q0q0 = q0 * q0;
      q1q1 = q1 * q1;
      q2q2 = q2 * q2;
      q3q3 = q3 * q3;

      // Gradient decent algorithm corrective step
      s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
      s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
      s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
      s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;

      norm = 1.0f / (float) sqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
      s0 *= norm;
      s1 *= norm;
      s2 *= norm;
      s3 *= norm;

      // Apply feedback step
      qDot1 -= beta * s0;
      qDot2 -= beta * s1;
      qDot3 -= beta * s2;
      qDot4 -= beta * s3;
    }

    // Integrate rate of change of quaternion to yield quaternion
    q0 += qDot1 * d_t;
    q1 += qDot2 * d_t;
    q2 += qDot3 * d_t;
    q3 += qDot4 * d_t;

    // Normalise quaternion
    norm = 1.0f / (float) sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q0 * q0);    // normalise quaternion
    q0 = q0 * norm;
    q1 = q1 * norm;
    q2 = q2 * norm;
    q3 = q3 * norm;

    c_frame->setO(set_i, q0, q1, q2, q3);
  }
}


int8_t TripleAxisMadgwick::calibrate_from_data_ag() {
  //// Average vectors....
  //Vector3<int32_t> avg;
  //for (int i = 0; i < 32; i++) {
  //  avg.x += sample_backlog_acc[i].x;
  //  avg.y += sample_backlog_acc[i].y;
  //  avg.z += sample_backlog_acc[i].z;
  //}
  //avg /= 32;   // This is our idea of gravity.
  //noise_floor_acc.x = (int16_t) avg.x;
  //noise_floor_acc.y = (int16_t) avg.y;
  //noise_floor_acc.z = (int16_t) avg.z;

  //for (int i = 0; i < 32; i++) {
  //  avg.x += sample_backlog_gyr[i].x;
  //  avg.y += sample_backlog_gyr[i].y;
  //  avg.z += sample_backlog_gyr[i].z;
  //}
  //avg /= 32;   // This is our idea of gravity.
  //noise_floor_gyr.x = (int16_t) avg.x;
  //noise_floor_gyr.y = (int16_t) avg.y;
  //noise_floor_gyr.z = (int16_t) avg.z;

  //float scaler = error_map_acc[scale_acc].per_lsb;
  //float x = ((noise_floor_acc.x) * scaler);
  //float y = ((noise_floor_acc.y) * scaler);
  //float z = ((noise_floor_acc.z) * scaler);

  //// This is our idea of magentic North. Write it to the offset registers.
  ////for (int i = 0; i < 8; i++) {
  ////  *(register_pool + 14 + i) = *(register_pool + 3 + i);
  ////}
  ////writeRegister(RegID::AG_INT_THS_L_M, (register_pool + 12),   8, true);
  //scaler = error_map_gyr[scale_gyr].per_lsb;
  //Vector3<int16_t> reflection_vector_gyr(ManuManager::reflection_gyr.x, ManuManager::reflection_gyr.y, ManuManager::reflection_gyr.z);

  //x = ((int16_t) regValue(RegID::G_DATA_X)) * scaler * reflection_vector_gyr.x;
  //y = ((int16_t) regValue(RegID::G_DATA_Y)) * scaler * reflection_vector_gyr.y;
  //z = ((int16_t) regValue(RegID::G_DATA_Z)) * scaler * reflection_vector_gyr.z;
  ////Vector3<float> proj_xy(x, y, 0);
  ////Vector3<float> proj_xz(x, 0, z);
  ////Vector3<float> x_axis(1.0f, 0.0f, 0.0f);
  ////
  ////float offset_angle_y = Vector3<float>::angle(&proj_xy, &x_axis);
  ////float offset_angle_z = Vector3<float>::angle(&proj_xz, &x_axis);
  ////integrator->pushMeasurement(IMU_FLAG_BEARING_DATA, 0, offset_angle_y, offset_angle_z, 0);
  ////integrator->pushMeasurement(IMU_FLAG_BEARING_DATA, x, y, z, 0);
  return 0;
}



int8_t TripleAxisMadgwick::calibrate_from_data_mag() {
  //// Is reading stable?
  //// Average vectors....
  //Vector3<int32_t> avg;
  //for (int i = 0; i < 32; i++) {
  //  avg.x += sample_backlog_mag[i].x;
  //  avg.y += sample_backlog_mag[i].y;
  //  avg.z += sample_backlog_mag[i].z;
  //}
  //avg /= 32;
  //noise_floor_mag.x = (int16_t) avg.x;
  //noise_floor_mag.y = (int16_t) avg.y;
  //noise_floor_mag.z = (int16_t) avg.z;
  return 0;
}
#endif
