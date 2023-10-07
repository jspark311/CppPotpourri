/*
File:   OrderedPair.h
Author: J. Ian Lindsay
Date:   2023.06.03

Copyright 2020 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef __C3P_DS_POINTS_H
#define __C3P_DS_POINTS_H

template <class T> class Point2 {
  public:
    T x, y;

    // trivial ctor
    Point2<T>(): x(0),y(0) {}

    // setting ctor
    Point2<T>(const T x0, const T y0): x(x0), y(y0) {}

    // setting ctor
    Point2<T>(const Point2<T>* p): x(p->x), y(p->y) {}

    // function call operator
    void operator ()(const T x0, const T y0)
    {  x= x0; y= y0;  }

    // setting fxn
    void set(Point2<T>* p) {
      x = (p->x);
      y = (p->y);
    }

    // test for equality
    bool operator==(const Point2<T> &p)
    {  return (x==p.x && y==p.y);  }

    // test for inequality
    bool operator!=(const Point2<T> &p)
    {  return (x!=p.x || y!=p.y);  }

    // addition
    Point2<T> operator +(const Point2<T> &p) const
    {   return Point2<T>(x+p.x, y+p.y);   }

    // addition
    Point2<T> &operator +=(const Point2<T> &p) {
      x+=p.x;  y+=p.y;
      return *this;
    }

    // subtraction
    Point2<T> operator -(const Point2<T> &p) const
    {   return Point2<T>(x-p.x, y-p.y);   }

    // subtraction
    Point2<T> &operator -=(const Point2<T> &p) {
      x-=p.x;  y-=p.y;
      return *this;
    }
};


template <class T> class Point3 {
  public:
    T x, y, z;

    // trivial ctor
    Point3<T>(): x(0),y(0),z(0) {}

    // setting ctor
    Point3<T>(const T x0, const T y0, const T z0): x(x0), y(y0), z(z0) {}

    // setting ctor
    Point3<T>(const Point2<T>* p): x(p->x), y(p->y), z(p->z) {}

    // function call operator
    void operator ()(const T x0, const T y0, const T z0)
    {  x= x0; y= y0; z= z0;  }

    // setting fxn
    void set(Point3<T>* p) {
      x = (p->x);
      y = (p->y);
      z = (p->z);
    }

    // test for equality
    bool operator==(const Point3<T> &p)
    {  return (x==p.x && y==p.y && z==p.z);  }

    // test for inequality
    bool operator!=(const Point3<T> &p)
    {  return (x!=p.x || y!=p.y || z!=p.z);  }

    // addition
    Point3<T> operator +(const Point3<T> &p) const
    {   return Point3<T>(x+p.x, y+p.y, z+p.z);   }

    // addition
    Point3<T> &operator +=(const Point3<T> &p) {
      x+=p.x;  y+=p.y;  z+=p.z;
      return *this;
    }

    // subtraction
    Point3<T> operator -(const Point3<T> &p) const
    {   return Point3<T>(x-p.x, y-p.y, z-p.z);   }

    // subtraction
    Point3<T> &operator -=(const Point3<T> &p) {
      x-=p.x;  y-=p.y;  z-=p.z;
      return *this;
    }
};

#endif // __C3P_DS_POINTS_H
