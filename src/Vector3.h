/*
Further modification by J. Ian Lindsay 2014.07.01
Original commentary, license, and credits preserved below.
Thank you, Bill and Michael!
*/

// Copyright 2010 Michael Smith, all rights reserved.

//  This library is free software; you can redistribute it and / or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.

// Derived closely from:
/****************************************
 * 3D Vector Classes
 * By Bill Perone (billperone@yahoo.com)
 * Original: 9-16-2002
 * Revised: 19-11-2003
 *          11-12-2003
 *          18-12-2003
 *          06-06-2004
 *
 * Copyright 2003, This code is provided "as is" and you can use it freely as long as
 * credit is given to Bill Perone in the application it is used in
 *
 * Notes:
 * if a*b = 0 then a & b are orthogonal
 * a%b = -b%a
 * a*(b%c) = (a%b)*c
 * a%b = a(cast to matrix)*b
 * (a%b).length() = area of parallelogram formed by a & b
 * (a%b).length() = a.length()*b.length() * sin(angle between a & b)
 * (a%b).length() = 0 if angle between a & b = 0 or a.length() = 0 or b.length() = 0
 * a * (b%c) = volume of parallelpiped formed by a, b, c
 * vector triple product: a%(b%c) = b*(a*c) - c*(a*b)
 * scalar triple product: a*(b%c) = c*(a%b) = b*(c%a)
 * vector quadruple product: (a%b)*(c%d) = (a*c)*(b*d) - (a*d)*(b*c)
 * if a is unit vector along b then a%b = -b%a = -b(cast to matrix)*a = 0
 * vectors a1...an are linearly dependant if there exists a vector of scalars (b) where a1*b1 + ... + an*bn = 0
 *           or if the matrix (A) * b = 0
 *
 ****************************************/

#ifndef __MANUVR_DS_VECTOR3_H
#define __MANUVR_DS_VECTOR3_H

#include <inttypes.h>
#include <stdint.h>
#include <math.h>


/*
* If used in the context of gravity, means: "Which axis is up?"
* If used in the context of magnetism, means: "Which axis is North?"
*/
enum class GnomonType : uint8_t {
  UNDEFINED      = 0b00000000,   //
  RH_POS_X       = 0b00000001,   //
  RH_POS_Y       = 0b00000010,   //
  RH_POS_Z       = 0b00000011,   //
  RH_NEG_X       = 0b00000101,   //
  RH_NEG_Y       = 0b00000110,   //
  RH_NEG_Z       = 0b00000111,   //
  LH_POS_X       = 0b00001001,   //
  LH_POS_Y       = 0b00001010,   //
  LH_POS_Z       = 0b00001011,   //
  LH_NEG_X       = 0b00001101,   //
  LH_NEG_Y       = 0b00001110,   //
  LH_NEG_Z       = 0b00001111    //
};


template <typename T> class Vector3 {
  public:
    T x, y, z;

    // trivial ctor
    Vector3<T>(): x(0),y(0),z(0) {}

    // setting ctor
    Vector3<T>(const T x0, const T y0, const T z0): x(x0), y(y0), z(z0) {}

    // setting ctor
    Vector3<T>(const Vector3<T>* existing): x(existing->x), y(existing->y), z(existing->z) {}

    // function call operator
    void operator ()(const T x0, const T y0, const T z0)
    {  x= x0; y= y0; z= z0;  }

    // setting fxn
    void set(Vector3<T>* existing) {
      x = (existing->x);
      y = (existing->y);
      z = (existing->z);
    }

    // test for equality
    bool operator==(const Vector3<T> &v)
    {  return (x==v.x && y==v.y && z==v.z);  }

    // test for inequality
    bool operator!=(const Vector3<T> &v)
    {  return (x!=v.x || y!=v.y || z!=v.z);  }

    // negation
    Vector3<T> operator -(void) const
    {  return Vector3<T>(-x,-y,-z);  }

    // addition
    Vector3<T> operator +(const Vector3<T> &v) const
    {   return Vector3<T>(x+v.x, y+v.y, z+v.z);   }

    // subtraction
    Vector3<T> operator -(const Vector3<T> &v) const
    {   return Vector3<T>(x-v.x, y-v.y, z-v.z);   }

    // uniform scaling
    Vector3<T> operator *(const T num) const {
      Vector3<T> temp(*this);
      return temp*=num;
    }

    // uniform scaling
    Vector3<T> operator /(const T num) const {
      Vector3<T> temp(*this);
      return temp/=num;
    }

    // addition
    Vector3<T> &operator +=(const Vector3<T> &v) {
      x+=v.x;  y+=v.y;  z+=v.z;
      return *this;
    }

    // subtraction
    Vector3<T> &operator -=(const Vector3<T> &v) {
      x-=v.x;  y-=v.y;  z-=v.z;
      return *this;
    }

    // uniform scaling
    Vector3<T> &operator *=(const T num) {
      x*=num; y*=num; z*=num;
      return *this;
    }

    // uniform scaling
    Vector3<T> &operator /=(const T num) {
      x/=num; y/=num; z/=num;
      return *this;
    }

    // dot product
    T operator *(const Vector3<T> &v) const {  return x*v.x + y*v.y + z*v.z;  }

    // cross product
    Vector3<T> operator %(const Vector3<T> &v) const {
      Vector3<T> temp(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
      return temp;
    }

    // gets the length of this vector squared
    T length_squared() const {  return (T)(*this * *this);       }

    // gets the length of this vector
    float length() const {      return (T)sqrt(*this * *this);   }

    // normalizes this vector
    float normalize(){
      float stacked_len = length();
      if (stacked_len) {
        if (stacked_len != 1.0f) {
          // Only do more math if anything will change. May already be normalized.
          // Multiplying 3 times is typically worth saving two divisions.
          // I picked up this trick from Sebastian Madgwick's AHRS code.
          stacked_len = 1.0f/stacked_len;
          *this *=stacked_len;
        }
      }
      return stacked_len;
    }

    // Set function with components broken out.
    void set(T _x, T _y, T _z) {
      x = _x;
      y = _y;
      z = _z;
    }

    // Normalize against the supplied length, for code that caches this information.
    void normalize(float len) {
      if (len) *this/=len;
    }

    // returns the normalized version of this vector
    Vector3<T> normalized() const {   return  *this/length();  }

    // reflects this vector about n
    void reflect(const Vector3<T> &n) {
      Vector3<T> orig(*this);
      project(n);
      *this= *this*2 - orig;
    }

    // projects this vector onto v
    void project(const Vector3<T> &v) {  *this= v * (*this * v)/(v*v);  }

    // returns this vector projected onto v
    Vector3<T> projected(const Vector3<T> &v) {  return v * (*this * v)/(v*v); }

    // computes the angle between 2 arbitrary vectors
    static inline T angle(const Vector3<T> &v1, const Vector3<T> &v2)
    {   return (T)acosf((v1*v2) / (v1.length()*v2.length()));  }

    // computes the angle between 2 arbitrary normalized vectors
    // NOTE: The minimum check is to prevent floating point rounding from
    //   causing a domain error in acosf().
    static inline T angle_normalized(const Vector3<T> &v1, const Vector3<T> &v2)
    {   return (T)acosf(strict_min(1.0, v1*v2));  }

};

typedef Vector3<int32_t>       Vector3i32;
typedef Vector3<int16_t>       Vector3i16;
typedef Vector3<int8_t>        Vector3i8;
typedef Vector3<uint32_t>      Vector3ui32;
typedef Vector3<uint16_t>      Vector3ui16;
typedef Vector3<uint8_t>       Vector3ui8;
typedef Vector3<float>         Vector3f;
typedef Vector3<double>        Vector3f64;

#endif // __MANUVR_DS_VECTOR3_H
