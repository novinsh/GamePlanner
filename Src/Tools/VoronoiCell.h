/**
 * @file VoronoiTile.h
 * Voronoi typed tile
 *
 * @author <a href="mailto:aref.moqadam@gmail.com">Aref Moqadam</a>
 * @date Feb 2014
 */

#pragma once

#include "Tools/Streams/Streamable.h"
#include "Tools/Math/Eigen.h"
#include "Tools/Math/Geometry.h"
#include "Tools/Math/BHMath.h"
#include "Tools/Math/Transformation.h"
#include "Platform/BHAssert.h"

class VoronoiCell : public Streamable
{
public:
	VoronoiCell() : _point(Pose2f()),
		_pointer(Pose2f()),
		_name(""),
		_regionId(0),
		_numOfSup(0)
	{}

	// -- setters
  inline const Pose2f& 			globalPose() const { return _point; }
  inline const Pose2f& 			pointer() const { return _pointer; }
  inline const std::string& name() const { return _name; }
  inline int	 							regionId() const { return _regionId; }
  inline unsigned 					numOfSup() const { return _numOfSup; }

  // -- getters
  inline void set(const Pose2f& point, const Pose2f& pointer) { _point = point; _pointer = pointer; }
  inline void setRegionId(int id) { _regionId = id; }
  inline void setName(const std::string& name) { _name = name; }
  inline void setNumOfSup(const unsigned num) { _numOfSup = num; }

  // -- ops
  inline void mirrorY() { _pointer.translation.y() = -_pointer.translation.y(); _point.translation.y() = -_point.translation.y(); }

private:
  Pose2f 			_point;
  Pose2f 			_pointer;
  std::string _name;
  int 				_regionId;
  unsigned 		_numOfSup;

  virtual void serialize(In* in, Out* out)
  {
    STREAM_REGISTER_BEGIN;
    STREAM(_point);
    STREAM(_pointer);
    STREAM(_name);
    STREAM(_regionId);
    STREAM(_numOfSup);
    STREAM_REGISTER_FINISH;
  }
};
