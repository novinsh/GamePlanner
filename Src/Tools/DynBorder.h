/**
 * @file DynBorder.h
 * Implementation of an Episodic Memory for behavior control input
 *
 * @author <a href="mailto: m.a@sharpsand.com">Mohammad Ali Sharpasand</a>
 * @date Oct 16, 2013
 */

#pragma once

#include "Tools/Streams/InOut.h"
#include "Tools/Streams/Streamable.h"

/**
 * @class DynBorder
 */
template <class T=float> class DynBorder : public Streamable
{
public:
  virtual void serialize(In* in, Out* out)
  {
    STREAM_REGISTER_BEGIN;
    STREAM(_position);
    STREAM(_dr);
    STREAM(_sign);
    STREAM_REGISTER_FINISH;
  }

  /**
   * @brief Constructor of the border.
   * @param position: The position of the border
   * @param dr: Deliberativeness Radius
   */
  DynBorder(T position, T dr) : _position(position), _dr(dr), _sign(0) {}
  DynBorder() {};

  friend bool operator <  (DynBorder<T>& first, const T& second) { return first.isAboveBorder(second); }
  friend bool operator <= (DynBorder<T>& first, const T& second) { return first.isAboveBorder(second); }
  friend bool operator >  (DynBorder<T>& first, const T& second) { return first.isBelowBorder(second); }
  friend bool operator >= (DynBorder<T>& first, const T& second) { return first.isBelowBorder(second); }
  friend bool operator <  (const T& first, DynBorder<T>& second) { return second.isBelowBorder(first); }
  friend bool operator <= (const T& first, DynBorder<T>& second) { return second.isBelowBorder(first); }
  friend bool operator >  (const T& first, DynBorder<T>& second) { return second.isAboveBorder(first); }
  friend bool operator >= (const T& first, DynBorder<T>& second) { return second.isAboveBorder(first); }

  friend bool operator <  (const DynBorder<T>& first, const DynBorder<T>& second) { return first._position <  second._position; }
  friend bool operator <= (const DynBorder<T>& first, const DynBorder<T>& second) { return first._position <= second._position; }
  friend bool operator >  (const DynBorder<T>& first, const DynBorder<T>& second) { return first._position >  second._position; }
  friend bool operator >= (const DynBorder<T>& first, const DynBorder<T>& second) { return first._position >= second._position; }

private:
  DynBorder(const DynBorder&);

  T _position;
  T _dr;
  char _sign;

  bool isAboveBorder(T x)
  {
    if (x > _position + _dr * _sign) // TODO: * deliberativeness_coefficient
    {
      _sign = -1;
      return true;
    }

    _sign = 1;
    return false;
  }

  bool isBelowBorder(T x)
  {
    if (x < _position + _dr * _sign)
    {
      _sign = 1;
      return true;
    }

    _sign = -1;
    return false;
  }

};

