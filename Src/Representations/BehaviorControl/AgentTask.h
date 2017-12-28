/**
 * @file AgentTask.h
 *
 * Result of task assignment module
 *
 * @author <a href="mailto:novinsha@gmail.com">Novin Shahroudi</a>
 * @date June 2017
 */

#pragma once

#include "Tools/Streams/Streamable.h"
#include "Tools/Math/Eigen.h"
#include "Tools/VoronoiCell.h"
#include "Tools/Streams/Enum.h"
#include <vector>

class AgentTask : public Streamable
{
public:
	AgentTask();

	ENUM(Role,
	{,
		Leader,
		Supporter,
		Defender,			// just for debugging (LED) - not really a role in our lit.
		GoalKeeper,		// just for debugging (LED) - not really a role in our lit.
		None,
	});

	// -- setters
	inline void setCells(const std::vector<VoronoiCell>& Tiles) { _cells = Tiles; }
	inline void setCell(unsigned int id, const VoronoiCell& value) { _cells[id] = value; }
	inline void setCurrentAgentVoronoiID(int id) { _currentVoronoiID = id; }
	inline void setRole(Role r) { _role = r; }
	inline void setBallIsFree(bool b) { _ballIsFree = b; }
	inline void setCurrentVoronoiPose(const Vector2f& p) { _currentVoronoiPose = p; }

	// -- getters
	inline const std::vector<VoronoiCell> cells() const { return _cells; }
	inline const VoronoiCell cell(unsigned int id) const { return _cells[id]; }
	inline const int 	getCurrentAgentVoronoiID() const { return _currentVoronoiID; }
	inline Role 			getRole() const { return _role; }
	inline bool 			getBallIsFree() const { return _ballIsFree; }
	inline Vector2f 	getCurrentVoronoiPose() const { return _currentVoronoiPose; }

	// -- ops
	const Pose2f& 			converToPoint(const Pose2f& p) const;
	const VoronoiCell& 	converToCell(const Pose2f& p) const;
	unsigned int 				converToId(const Pose2f& p, const unsigned& hysID=0) const;

	bool load(const std::string& configAddress);
	void load(const std::vector<VoronoiCell>& tiles) { setCells(tiles); };
	void getTilesFromFile(const std::string& configAddress, std::vector<VoronoiCell>& tiles);

private:
	std::vector<VoronoiCell> _cells;
	Role 			_role;
	int 			_currentVoronoiID;
	Vector2f 	_currentVoronoiPose;
	bool 			_ballIsFree;

	virtual void serialize(In* in, Out* out)
	{
		STREAM_REGISTER_BEGIN;
		STREAM(_cells);
		STREAM(_currentVoronoiID);
		STREAM(_currentVoronoiPose);
		STREAM(_ballIsFree);
		STREAM(_role);
		STREAM_REGISTER_FINISH;
	}
};
