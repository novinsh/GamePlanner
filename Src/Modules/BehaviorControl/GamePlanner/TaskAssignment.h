/**
 * @file TaskAssignment.h
 *
 * Formation selection and Post/Role assignment
 *
 * @author Novin Shahroudi
 * @author Mohammadreza Hasanzadeh
 *
 * @date Feb 2015
 * @date 1 Jun, 2016
 */

#pragma once

#include "Tools/Module/Module.h"
#include "Tools/DynBorder.h"
#include "Representations/Infrastructure/RobotInfo.h"
#include "Representations/Infrastructure/FrameInfo.h"
#include "Representations/Infrastructure/GameInfo.h"
#include "Representations/Infrastructure/TeamInfo.h"
#include "Representations/Sensing/FallDownState.h"
#include "Representations/Perception/FieldPercepts/CirclePercept.h"
#include "Representations/Modeling/RobotPose.h"
#include "Representations/Modeling/BallModel.h"
#include "Representations/Modeling/TeamBallModel.h"
#include "Representations/Communication/TeammateData.h"
#include "Representations/BehaviorControl/AgentTask.h"
#include <map>

MODULE(TaskAssignment,
{,
	REQUIRES(RobotInfo),
	REQUIRES(FrameInfo),
	REQUIRES(GameInfo),
	REQUIRES(OwnTeamInfo),
	REQUIRES(FallDownState),
	REQUIRES(CirclePercept),
	REQUIRES(RobotPose),
	REQUIRES(BallModel), // TODO: if not usable remove it totally
	REQUIRES(TeamBallModel),
	REQUIRES(TeammateData),
	PROVIDES(AgentTask), // TODO
	LOADS_PARAMETERS(
	{,
		(bool)(false) dynamicPostAssign,
		(bool)(true) dynamicRoleAssign,
		(int)(1)			formationVersion,
		(std::vector<int>)(4, 0)	players,
	}),
});

class TaskAssignment : public TaskAssignmentBase {
public:
	/*
	 * Default constructor
	 */
	TaskAssignment();

	/**
	 * Default destructor
	 */
	virtual ~TaskAssignment() {}

	/**
	 * Main update
	 */
  void update(AgentTask& AgentTask);

private:
	/**
	 * Applies general rules of the game
	 */
  void updateBasicPlan();

	/**
	 * Updates Formation
	 *
	 * It chooses a formation among set of predefined formations
	 */
	void updateFormation();

	/**
	 * Updates post of each agent
	 */
	void updatePost();

	/**
	 * Updates role of each agent
	 */
	void updateRole();

	/**
	 * Checks movement of the ball right after game play
	 */
	void calculateHasBallMoved();

	/**
	 * calculating cost based on rotation, velocity and distance
	 * @param x0
	 * @param v0
	 * @param xf
	 * @param vf
	 * @param maxA
	 * @param maxV
	 */
	float timeCost(float x0, float v0, float xf, float vf, float maxA, float maxV);

	/**
	 * Calculates cost of each robot to each post or role
	 * @param c cost matrix
	 * @param agent list of agents
	 */
	void costOfRobotToPost(std::vector<std::vector<float> > &c, const std::vector<int> &agent);

	/**
	 * Get Teammate data based on player number
	 */
	const Teammate& getAgentByPlayerNumber(const int& playerNumber);

	Vector2f voronoiPoseRelativeToBall(const Vector2f& VoronoiPose, const Vector2f& BallPosition, const Vector2f& radius);
	unsigned lastNumOfPlayers();
	bool hasGotBall();

	// TODO: Find a better workaround for toString -
	// last time I failed to use to_string!!
	template<typename T>
	inline std::string toString(T val) {
		std::stringstream tmp;
		tmp << val;
		return tmp.str();
	}

	/**
	 * pairCompare
	 *
	 * To find minimum in a container
	 */
	static bool pairCompare(const std::pair<unsigned,float>& firstElm, const std::pair<unsigned,float>& secondElm){
		return firstElm.second < secondElm.second;
	}

private:
	AgentTask agentTask; /*< output of the module */

	// update formation vars -----------------------------------------------------
	unsigned numOfPlayers; /*< number of players affecting formation & strategies */
	unsigned lastFrameNumOfPlayers = 0; /*< to determine changes since last frame */
	uint8_t gameState = 0; /*< hold game state to determine changes since last frame */
	bool kickoffus; /*< hold kickoffus state to determine changes since last frame */
	std::map<std::string, std::vector<VoronoiCell> > formations; /*< loaded formations from file */
	std::vector<VoronoiCell> lastSetFormation; // ?

	// vars used in post assignment ----------------------------------------------
	std::vector<int> bestPermutation; /*< result of the post assignment algorithm */
	std::vector<int> agents; /*< list of current agents */

	// vars used in role assignment ----------------------------------------------
	int leaderID = -1; /*< indicates id of the robot that's been leader in the last frame */
	int voronoiWithTheBall = 0; /*< id of the voronoi which contains the ball */
	DynBorder<float> ballInOwnHalfThre;
	DynBorder<float> distanceToTargetThre;

	//	char robotTranslationSpeed_x = 0;
	//	char robotTranslationSpeed_y = 0;
	const char robotTranslationSpeed = 75;	// TODO: fill it with non-constant value

	// ball related vars ---------------------------------------------------------
	Vector2f ballGlobal;
	bool setKickOffWait = true;
	unsigned kickOffWait = 0;
	float ballMoveLimit = 400;
	bool hasBallMoved = false;
	bool ballIsFree = false;
	int timeMustWait = 0;


	// --------
	int kickOffTimerPalang;			// hack for robocup 2017 - this could be limited to the palang only!
	bool palangExpired = false;

};
