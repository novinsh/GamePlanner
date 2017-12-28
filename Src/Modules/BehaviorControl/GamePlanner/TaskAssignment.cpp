/**
 * @file TaskAssignment.cpp
 *
 * Formation selection and Post/Role assignment
 *
 * @author Novin Shahroudi
 * @author Mohammadreza Hasanzadeh
 *
 * @date Feb 2015
 * @date 1 Jun, 2016
 */

#include "TaskAssignment.h"
#include "Tools/Debugging/DebugDrawings.h"
#include "Platform/Time.h"
#include "Platform/File.h"
#include <dirent.h>
#include <iostream>
#include <regex.h>
#include <algorithm>

MAKE_MODULE(TaskAssignment, behaviorControl)

TaskAssignment::TaskAssignment() :
ballInOwnHalfThre(-1500, 300),
distanceToTargetThre(750, 100)
{
	using namespace std;
	DIR *dir;
	struct dirent *ent;

	regex_t regex;
	int ret;

	//filter cpp and python files with regex
	const char *pattern = "^.*\\.cfg$";
	ret = regcomp(&regex, pattern, 0);
	if(ret)
	{
		std::cerr << "could not compile the regex!" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::string path = std::string(File::getBHDir()) + "/Config/Formations/";
	if ((dir = opendir (path.c_str())) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			ret = regexec(&regex, ent->d_name, 0, NULL, 0);
			if(!ret)
			{
				//	  		printf ("%s\n", ent->d_name);
				std::vector<VoronoiCell> tmpTile;
				agentTask.getTilesFromFile((path + ent->d_name).c_str(), tmpTile);
				formations[ent->d_name] = tmpTile;
			}
		}
		closedir (dir);
	} else {
		/* could not open directory */
		perror ("could not open directory");
	}

	regfree(&regex);
}

void TaskAssignment::update(AgentTask& agentTask)
{
	ORIGIN("module:TaskAssignment", 0, 0, 0);
	DECLARE_DEBUG_DRAWING("module:TaskAssignment", "drawingOnField");

	// GoalKeeper or Penalized robots are not considered
	if(theRobotInfo.number == 1 || theRobotInfo.penalty != PENALTY_NONE)
	{
		this->agentTask.setRole(AgentTask::GoalKeeper);
		agentTask = this->agentTask;
		return;
	}

	updateBasicPlan();

	if(theGameInfo.state == STATE_READY || theGameInfo.state == STATE_SET ||
			theGameInfo.state == STATE_PLAYING)
	{
		updateFormation();
		updatePost();
	}

	if(theGameInfo.state == STATE_SET || theGameInfo.state == STATE_READY ||
			theGameInfo.state == STATE_PLAYING )
	{
		//		OUTPUT_TEXT("update role");
		updateRole();
	}
	else
	{
		agentTask.setRole(AgentTask::None);
		leaderID = -1;
	}

	agentTask = this->agentTask;

}

void TaskAssignment::calculateHasBallMoved()
{
	if (theGameInfo.state != STATE_PLAYING)
	{
		hasBallMoved = false;
		return;
	}

	//-- In order to reduce the computations
	if (hasBallMoved) return;
	// To prevent usage of wrong ball data.
	if (theFrameInfo.getTimeSince(theBallModel.timeWhenDisappeared) > 700) return;
	// [TODO] : check if looking at the center but the ball is not there...!
	if(theRobotPose.validity > 0.9f)
		hasBallMoved = hasBallMoved || (Transformation::robotToField(theRobotPose,
				theBallModel.estimate.position).norm() > ballMoveLimit);

#ifdef TARGET_SIM
	if(theTeamBallModel.position.norm() > 500)
		hasBallMoved = true;
#else
	const CirclePercept& circle =  theCirclePercept;
	if(theFrameInfo.getTimeSince(circle.lastSeen) < 5)
	{
		const Vector2f ballToCircle =
				Vector2f(circle.pos.x(), circle.pos.y()) - theBallModel.estimate.position;
		hasBallMoved = hasBallMoved || (ballToCircle.norm() > ballMoveLimit);
	}
#endif

}

void TaskAssignment::updateBasicPlan()
{
	using namespace std;

	if(dynamicPostAssign) // because numOfPlayers specifically used in updatePost
		numOfPlayers = lastNumOfPlayers();
	else
		numOfPlayers = (unsigned)players.size();

	// {{{ CONSIDERATIONS IN KICK OFF
	bool kickOffOpponent = theGameInfo.kickOffTeam != theOwnTeamInfo.teamNumber;
	calculateHasBallMoved();

	if(theGameInfo.state == STATE_PLAYING && !ballIsFree && kickOffOpponent)
	{
		if(setKickOffWait)
		{
			kickOffWait = Time::getCurrentSystemTime();

#ifdef TARGET_SIM
			timeMustWait = 10;
#else
			// FIXME: whistlewasblown non existing in bhuman's code!
//			if (theGameInfo.whistleWasBlown)
//				timeMustWait = 10;
//			else
				timeMustWait = 0;
#endif

			setKickOffWait = false;
		}

//		OUTPUT(idText, text, "%%%: " << Time::getTimeSince(kickOffWait)/1000  <<" hasBallMovoed= " << hasBallMoved );
		if(Time::getTimeSince(kickOffWait)/1000 >= timeMustWait || hasBallMoved)
			ballIsFree = true;
		else
			ballIsFree = false;
	}
	else if (theGameInfo.state <= STATE_SET && kickOffOpponent)
	{
		ballIsFree = false;
		setKickOffWait = true;
	}
	else if(theGameInfo.state <= STATE_SET && !kickOffOpponent)
		ballIsFree = true;

	agentTask.setBallIsFree(ballIsFree);
	// }}}


	//{{{ implement an expiration for palang role in order to avoid conflict with supporter role
	kickOffOpponent ? palangExpired = true : palangExpired = false;

	if(theGameInfo.state == STATE_SET && !kickOffOpponent)
	{
		kickOffTimerPalang = theFrameInfo.time;
		palangExpired = false;
	}

	if(theGameInfo.state == STATE_PLAYING && !kickOffOpponent)
	{
		theFrameInfo.getTimeSince(kickOffTimerPalang) > 15000 ?
				palangExpired = true : palangExpired = false;
	}

	//}}}
}

void TaskAssignment::updateFormation()
{
	using namespace std;

	// change game formation based on gameState and/or numberOfPlayers
	bool gameStateHasChanged = gameState xor theGameInfo.state;
	bool kickoffushaschanged =
			kickoffus xor (theGameInfo.kickOffTeam == theOwnTeamInfo.teamNumber);
	bool formationNeedToChange = gameStateHasChanged or
			(lastFrameNumOfPlayers xor numOfPlayers) or kickoffushaschanged;

	// change formation if necessary
	if(!formationNeedToChange)
		return;

	//	OUTPUT_TEXT("new formation!\t" << gameStateHasChanged << "\t" <<
	//			(lastFrameNumOfPlayers xor numOfPlayers) );

	lastFrameNumOfPlayers = numOfPlayers;
	gameState = theGameInfo.state;
	kickoffus = theGameInfo.kickOffTeam == theOwnTeamInfo.teamNumber;

	std::string formationToLoad;

	string gameState_str =
			(gameState == STATE_READY || gameState == STATE_SET) ? "_ready" : "_playing";
	string numOfPlayers_str = "_" + toString(numOfPlayers) + "player";
	string kickoff_str = kickoffus ? "_kickoffus" : "";
	string strategyVersion_str = "_" + toString(formationVersion);

	formationToLoad = std::string("formation") + gameState_str + numOfPlayers_str +
			kickoff_str + strategyVersion_str + std::string(".cfg");

	// mirror formation positions when a goal achieved, either by us or the opponent
	if((gameStateHasChanged && kickoffus &&
			theGameInfo.state == STATE_READY ) && dynamicPostAssign)
		for(std::map<std::string,
				std::vector<VoronoiCell> >::iterator ii =
						formations.begin(); ii != formations.end(); ++ii)
		{
			for(std::vector<VoronoiCell>::iterator jj =
					ii->second.begin(); jj != ii->second.end(); ++jj)
				(*jj).mirrorY();
		}

	std::map<std::string, std::vector<VoronoiCell> >::iterator res =
			formations.find(formationToLoad);

	if(res == formations.end())
	{
		std::cerr << "loading formation failed: " << formationToLoad << std::endl;
		ASSERT(false);
	}
	else
	{
		//		OUTPUT_WARNING(theRobotInfo.number << " :: loading formation: " << formationToLoad);
		agentTask.load(res->second);
		lastSetFormation = res->second;
	}
}

const Teammate& TaskAssignment::getAgentByPlayerNumber(const int& playerNumber)
{
	for(auto& teammate : theTeammateData.teammates)
		if(teammate.number == playerNumber) {
			return teammate;
		}
		else
			continue;

	throw "no teammate found by the given index";
}

void TaskAssignment::costOfRobotToPost(std::vector<std::vector<float> > &c,
		const std::vector<int> &agent)
{
	using namespace std;
	Vector2f target;
	//	float xP = 0, xR = 0, yP = 0, yR = 0, t = 0;

	float td = 0, th = 0, t = 0, d =0, h = 0;

	vector<VoronoiCell> position = agentTask.cells();

	// calculating cost based on time cost
	for (size_t i = 0; i < c.size() ; i++)
	{
		for ( size_t j=0 ; j< c.size() ; j++)
		{
			int standToWalkCost = 0;

			if (theRobotInfo.number == agent [i])
			{
				target = Transformation::fieldToRobot(theRobotPose,
						position[j].globalPose().translation);
				// TODO: uncomment following when next TODO has been done
				//				if(theMotionInfo.motion == MotionInfo::walk)
				//					robotTranslationSpeed = theMotionInfo.walkRequest.speed.translation.norm();
				//				else if (theMotionInfo.motion == MotionInfo::stand && target.norm() > distanceToTargetThre)
				//					standToWalkCost = 2;
			}
			else
			{
				try {
					target =
							Transformation::fieldToRobot(getAgentByPlayerNumber(agent[i]).pose,
									position[j].globalPose().translation);
				} catch (std::string error) {
					cerr << error << endl;
				}

				// TODO: we need some motion data to be communicated to incorporate following cost
				//				robotTranslationSpeed_x = theTeamMateData.motionRequest[agent[i]].walkRequest.speed.translation.x;
				//				robotTranslationSpeed_y = theTeamMateData.motionRequest[agent[i]].walkRequest.speed.translation.y;
				//				robotTranslationSpeed = sqrt((pow(robotTranslationSpeed_x,2)) + (pow(robotTranslationSpeed_y,2)));

				//				if (theTeamMateData.motionRequest[i].motion == MotionInfo::stand && target.abs() > distanceToTargetThre)
				//					standToWalkCost = 2;
			}
			d = target.norm();
			h = target.angle();
			th = timeCost(h,0,0,0,0.2f,0.25f);
			td = timeCost(d,robotTranslationSpeed,0,0,16,220);
			t = th + td + standToWalkCost;
			c[i][j] = t;

			CIRCLE("module:TaskAssignment",
					position[j].globalPose().translation.x(), position[j].globalPose().translation.y(),
					50, 10, Drawings::solidPen, ColorRGBA::yellow, Drawings::solidPen, ColorRGBA::black);
		}
	}

}

void TaskAssignment::updatePost()
{
	using namespace std;

	// static assignment  ---------------------------------------------------------------
	if (!dynamicPostAssign)
	{
		unsigned int agentVoronoi;
		for (agentVoronoi=0; agentVoronoi < players.size() ; agentVoronoi++ )
		{
			if (theRobotInfo.number == players.at(agentVoronoi))
				break;
		}
		ASSERT(agentVoronoi < players.size());

		try
		{
			agentTask.setCurrentAgentVoronoiID(agentVoronoi);
			agentTask.setCurrentVoronoiPose(agentTask.cell(agentVoronoi).globalPose().translation);
		}
		catch(std::out_of_range&)
		{
			std::cout << "agentVoronoi: " << agentVoronoi << std::endl;
			throw(std::out_of_range("agentVoronoi out of range in TaskAssignment::updatePost()"));
		}

		LINE("module:TaskAssignment",
				theRobotPose.translation.x(), theRobotPose.translation.y(),
				agentTask.cells()[agentVoronoi].globalPose().translation.x(),
				agentTask.cells()[agentVoronoi].globalPose().translation.y(),
				60, Drawings::solidBrush, ColorRGBA::red);
		return;
	}
	else // dynamic assignment --------------------------------------------------
	{

		/**
		 *  find out which voronoi leader is in (actually last frame's leader)
		 *  if a leader exists then:
		 *  Give the post that leader is in (cost to voronoi that leader's in = 0)
		 *
		 *  // FIXME: this may cause a paradox between role and post results ->
		 *  better to merge ball postiion and the closest post!
		 */
		int postForLeader = -1;
		if(leaderID > -1) // only if any leader exists at all
		{
			if(leaderID == theRobotInfo.number)
			{
				int tmpv = agentTask.converToId(theRobotPose);
				if(agentTask.cell(tmpv).name() != "DF")
					postForLeader = tmpv;
			}
			else
			{
				for(auto &teammate : theTeammateData.teammates)
				{
					if(teammate.number == leaderID)
					{
						int tmpv = agentTask.converToId(teammate.pose);
						if(agentTask.cell(tmpv).name() != "DF")
							postForLeader = tmpv;
					}
				}
			}
		}

		vector<int> post(numOfPlayers);
		//{{{ add present agents counted for post/role assignment
		agents.clear();
		for(auto& teammate : theTeammateData.teammates)
		{
			if(!teammate.isGoalkeeper && teammate.status != Teammate::PENALIZED)
				agents.push_back(teammate.number);
		}
		agents.push_back(theRobotInfo.number); // add me to agent list
		//}}}


		// FIXME: this is a quick hack, djavo's comming - continue of post assignment is not possible
		// numOfPlayers is calculated based on numOfActiveTeamMates but because of hysteresis buffer it may have lag
		// we also calculate size of agent buffer based on numOfActiveTeamMates which is feed of numOfPLayers provided in selfDataProvider
		// this will have inconsistency with itself!
		if(agents.size() != (size_t)numOfPlayers) {
			cerr << "this should never happen, something's wrong!" << __LINE__ << endl;
			return;
		}

		// FIXME: sync with voronoi ids in formation files
		for(size_t i = 0 ; i < post.size(); i++)
			post[i] = (int)i;

		// FIXME: redundant indexes is used in costMatrix!!!
		vector<vector<float> > costMatrix;
		costMatrix.resize(numOfPlayers);
		for(size_t i = 0; i < costMatrix.size(); i++)
			costMatrix[i].resize(numOfPlayers);

		costOfRobotToPost(costMatrix, agents);

		// find leader's position in the agent matrix
		long idxOfLeaderInAgentMatrix;
		if(postForLeader > -1) // only if any leader exists at all
		{
			vector<int>::const_iterator ifoundLeader =
					find(agents.begin(), agents.end(), leaderID);
			if(ifoundLeader != agents.end())
				idxOfLeaderInAgentMatrix = ifoundLeader - agents.begin();
			else
				postForLeader = -1;   // there's going to be a change in leader in current frame (not yet happened) so ignore!
		}

		// make leader's cost to its voronoi zero (0)
		if(postForLeader > -1) // only if any leader exists at all
			costMatrix[idxOfLeaderInAgentMatrix][postForLeader] = 0;

		sort(post.begin(),post.end());	// TODO: seams to be redundant

		//	std::cout << "numOfPlayers: " << numOfPlayers << std::endl;
		bestPermutation.clear();
		bestPermutation.resize(numOfPlayers);

		float globalMin = INFINITY;

		do
		{
			/* avoid permutations that is not our desire
			 * in other words: exclude perms. which leader is not assigned where it
			 * is meant to. in another words: perform post assignment with rest of the
			 * available points (one is assigned explicitly to the leader)
			 */
			if(postForLeader > -1)
				if( post[idxOfLeaderInAgentMatrix] != postForLeader)
					continue;

			float localMin=0;
			for(unsigned i=0; i<numOfPlayers; i++)
			{
				localMin += costMatrix[i][post[i]];
				// cout << setprecision(2) << costMatrix[i][post[i]] << "(a" << agent[i] << " -> p" << post[i] << ")" << endl;
			}
			// cout << " sum: " << localMin << endl << "------------------" << endl;

			if(localMin < globalMin)
			{
				for (size_t i=0 ; i<post.size(); i++ )
					bestPermutation[i] = post[i];
				globalMin = localMin;
			}
		} while(next_permutation(post.begin(), post.end()));

		vector<int>::iterator ifound = find(agents.begin(), agents.end(), theRobotInfo.number);
		long idx = ifound - agents.begin();

		// FIXME: !!!! output of the module
		//	theWorld.mutableSelf().mutableMyState().mutableVote().setBestPermutation(bestPermutation);

		int vID; // voronoi ID
		vID = bestPermutation[idx];
		//OUTPUT_TEXT(theRobotInfo.number << " :: before: " << vID);


		////OUTPUT_TEXT(theRobotInfo.number << " :: after: " << vID);
		// FIXME: the second condition of the following if statement is not working in fast game due to
		// ball is valid flag issues. fix that then fix this so we can only manipulate voronoi points when we've got the ball!
		if(theGameInfo.state == STATE_PLAYING) // && theWorld.ball().providedLevel() >= MRLBody::Combined)
		{
			// TODO: fill in the radius from the gui's input

			//			Vector2f p = voronoiPoseRelativeToBall(lastSetFormation[vID].globalPose().translation, ballGlobal, Vector2f(750, 500));
			agentTask.setCurrentVoronoiPose(lastSetFormation[vID].globalPose().translation);
			agentTask.setCurrentAgentVoronoiID(vID);
			//			formation.setCurrentVoronoiPose(p);

			//			CIRCLE("module:TaskAssignment",
			//					p.x(), p.y(),
			//					50, 10, Drawings::solidBrush, ColorRGBA::black, Drawings::solidBrush, ColorRGBA::black);
		}
		else
		{
			agentTask.setCurrentAgentVoronoiID(vID);
			agentTask.setCurrentVoronoiPose(lastSetFormation[vID].globalPose().translation);
		}

		LINE("module:TaskAssignment",
				theRobotPose.translation.x(), theRobotPose.translation.y(),
				agentTask.cells()[bestPermutation[idx]].globalPose().translation.x(),
				agentTask.cells()[bestPermutation[idx]].globalPose().translation.y(),
				60, Drawings::solidBrush, ColorRGBA::red);

		// cout << "\nglobal minimum: " << globalMin << endl;
#ifndef RELEASE
		std::vector<VoronoiCell> position = agentTask.cells();
		for(size_t i=0; i<agents.size(); i++)
		{
			DRAWTEXT("module:TaskAssignment",
					position[bestPermutation[i]].globalPose().translation.x(),
					position[bestPermutation[i]].globalPose().translation.y(), 100, ColorRGBA::white, agents[i]);
			// cout << "a" << agent[i]+2 << "->p" << bestPermutation[i]+1 << "\t";
		}
#endif
	}
}

void TaskAssignment::updateRole()
{
	bool hasGotTheBall = hasGotBall();

	// fix plan  ---------------------------------------------------------------
	if(!dynamicRoleAssign)
	{
		bool leader;

		if(theGameInfo.state == STATE_READY)
		{
			voronoiWithTheBall = agentTask.converToId(Pose2f(0, 0, 0));
			leader = (agentTask.getCurrentAgentVoronoiID() == voronoiWithTheBall);
		}
		else
		{
			ballGlobal =
						Transformation::robotToField(theRobotPose, theBallModel.estimate.position);

			voronoiWithTheBall = agentTask.converToId(ballGlobal, voronoiWithTheBall);
			leader = (agentTask.getCurrentAgentVoronoiID() == voronoiWithTheBall
					or theBallModel.estimate.position.norm() < distanceToTargetThre) and hasGotTheBall;
		}

		//		OUTPUT(idText, text, formation.converToId(theWorld.ball().globalPose()));

		if(leader)
		{
			//			std::cout << "leader " << std::endl;
			agentTask.setRole(AgentTask::Leader);

			DRAWTEXT("module:TaskAssignment",
					theRobotPose.translation.x(),
					theRobotPose.translation.y(),
					300,ColorRGBA::black, "LD");
		}
		else
			agentTask.setRole(AgentTask::None);
	}
	else	// dynamic assignment  ------------------------------------------------
	{
		// role assignment without ball's meaningless
		if(!hasGotTheBall)
		{
			agentTask.setRole(AgentTask::None);
			return;
		}

		std::vector<std::pair<int,float> > robotsToBallCost;

		Vector2f target;
		float th, td;

		if(theRobotInfo.number != 1 &&
				theFallDownState.state == theFallDownState.upright)
		{
			if(theGameInfo.state == STATE_READY || theGameInfo.state == STATE_SET)
				target = Transformation::fieldToRobot(theRobotPose, Vector2f(0,0));
			else if(theGameInfo.state == STATE_PLAYING)
				target = Transformation::fieldToRobot(theRobotPose, theTeamBallModel.position);

			th = timeCost(target.angle(), 0, 0, 0, 0.2f, 0.25f);
			td = timeCost(target.norm(), 0, 0, 0, 16, 220);

			float lowerLastFrameLeaderCost = 0;

			if(leaderID != -1 && theRobotInfo.number == leaderID)
			{
				if(theGameInfo.state == STATE_READY)
					lowerLastFrameLeaderCost -= 1;	// 1 secs
				else
					lowerLastFrameLeaderCost -= 2;	// 1 secs
			}

			robotsToBallCost.push_back(std::make_pair(theRobotInfo.number, th+td+lowerLastFrameLeaderCost));
		}

		for(auto teammate : theTeammateData.teammates)
		{
			if(theGameInfo.state == STATE_READY || theGameInfo.state == STATE_SET)
				target = Transformation::fieldToRobot(teammate.pose, Vector2f(0,0));
			else if(theGameInfo.state == STATE_PLAYING)
				target = Transformation::fieldToRobot(teammate.pose, theTeamBallModel.position);

			if(!teammate.isGoalkeeper)
			{
				th = timeCost(target.angle(), 0, 0, 0, 0.2f, 0.25f);
				td = timeCost(target.norm(), 0, 0, 0, 16, 220);

				float lowerLastFrameLeaderCost = 0;

				if(leaderID != -1 && leaderID == teammate.number)
				{
					if(theGameInfo.state == STATE_READY)
						lowerLastFrameLeaderCost -= 1;	// 1 secs
					else
						lowerLastFrameLeaderCost -= 2;	// 1 secs
				}

				if(teammate.status == Teammate::PLAYING)
				{
					robotsToBallCost.push_back(std::make_pair(teammate.number, th+td+lowerLastFrameLeaderCost));

					// FIXME: consider start walking from lull
					/* if(teammate.motionRequest.motion == MotionRequest::stand && target.abs() > distanceToTargetThre)
							costToBall[i] += 2;
							if(theRunswiftMotionInfo.actiontype == ActionCommand::Body::STAND && target.norm() > distanceToTargetThre)
							costToBall[theRobotInfo.number] += 2;
					 */
				}
				else
				{
					// TODO: more logical value for the fallen robot cost to ball
					robotsToBallCost.push_back(std::make_pair(teammate.number, 1000));
				}
			}
		}

		// There are times that robotsToBallCost is not filled such as the very
		// beginning of the simulation time hence this code should not be run from
		// this point on because it depends on the robotsToBallCost data.
		if(!robotsToBallCost.size())
		{
			agentTask.setRole(AgentTask::None);
			return;
		}

		sort(robotsToBallCost.begin(), robotsToBallCost.end(), TaskAssignment::pairCompare);

		//		for(std::vector<std::pair<int, float> >::iterator it = robotsToBallCost.begin(); it != robotsToBallCost.end(); ++it)
		//			OUTPUT(idText, text, (*it).first << "\t" << (*it).second);
		//		OUTPUT_TEXT("\n------------\n");


		//		OUTPUT_TEXT(theRobotInfo.number << ": " << robotsToBallCost[0].second << "\t" << robotsToBallCost[1].second );
		leaderID = robotsToBallCost[0].first; // leader
		//		std::cout << "leaderID: " << leaderID << std::endl;

		bool amIDefender = false, hasSupporter = false;
		// This "if" is a hack to be able to have static_post_assign with dyn_role_assign
		if(dynamicPostAssign) // FIXME: refactor in order to be independnt of agents container
		{
			std::vector<int>::iterator ifound =
					std::find(agents.begin(), agents.end(), robotsToBallCost[0].first);

			if(ifound == agents.end())
			{
				std::cerr << " :: couldn't find agent in  std::vector<int> agent " << std::endl;
				goto DF_SUP_BAK_PLAN;	// it's been a long time ago I used this feature, though not proud :D
			}

			long idx = ifound - agents.begin();

			// fill-out amIDefender
			if(agentTask.getCurrentAgentVoronoiID() >= 0 && theGameInfo.state == STATE_PLAYING)
				agentTask.cell(agentTask.getCurrentAgentVoronoiID()).name().compare("DF") == 0 ? amIDefender = true : amIDefender = false;

			// fill-out hasSupporter
			if(bestPermutation.size()) {
				hasSupporter = agentTask.cell(bestPermutation[idx]).numOfSup() ? true : false;
			}
		}
		else	// only role assignment is dynamic
		{
DF_SUP_BAK_PLAN:
//			OUTPUT_TEXT("role assign - supporter/defender");
			if(agentTask.getCurrentAgentVoronoiID() >= 0 && theGameInfo.state == STATE_PLAYING)
				agentTask.cell(agentTask.getCurrentAgentVoronoiID()).name().compare("DF") == 0 ? amIDefender = true : amIDefender = false;

			if(!amIDefender)
				hasSupporter = true;
		}

		float LeaderPoseX;
		bool isleaderPoseValid = false;
		for(auto& teammate : theTeammateData.teammates) {
			if(teammate.number == leaderID) {
				LeaderPoseX = teammate.pose.translation.x();
				isleaderPoseValid = true;
			}
		}

		bool amIPalanag =
				agentTask.cell(agentTask.getCurrentAgentVoronoiID()).name() == "palang";

		// TODO: Move drawings to the representation

		///{{{ Drawings for debug
		if (robotsToBallCost[0].first == theRobotInfo.number)
		{
			agentTask.setRole(AgentTask::Leader);

			DRAWTEXT("module:TaskAssignment",
					theRobotPose.translation.x(),
					theRobotPose.translation.y(),
					300,ColorRGBA::black, "LD");
		}
		else if(robotsToBallCost[1].first == theRobotInfo.number && hasSupporter)
		{
			if(palangExpired && !amIDefender)	// hack: for robocup 2017
			{
				agentTask.setRole(AgentTask::Supporter);

				DRAWTEXT("module:TaskAssignment",
						theRobotPose.translation.x(),
						theRobotPose.translation.y(),
						300,ColorRGBA::white, "SP");
			}
			else
			{
				agentTask.setRole(AgentTask::None);
			}
		}
		else if(robotsToBallCost[1].first == theRobotInfo.number && amIDefender &&
				theTeamBallModel.position.x() < ballInOwnHalfThre &&
				isleaderPoseValid && LeaderPoseX > theTeamBallModel.position.x())	// in case our leader fell behind the opponent (?)
		{
			agentTask.setRole(AgentTask::Leader);

			DRAWTEXT("module:TaskAssignment",
					theRobotPose.translation.x(),
					theRobotPose.translation.y(),
					300,ColorRGBA::white, "LD2");
		}
		else
			agentTask.setRole(AgentTask::None);
		///}}} end of drawings


	} // end of dynamic role assignment

}

float TaskAssignment::timeCost(float x0, float v0, float xf, float vf, float maxA, float maxV)
{
	const float dxMin = (vf*vf - v0*v0) / (2.f*maxA*sgn(vf-v0));

	/*
	 *                            ⎧ ∆X < ∆Xmin => a<0 (1.1)
	 *          ⎧ ∆X>0 =========> ⎨                          (Special-1)
	 * (1) ∆V>0 ⎨ ∆X<0 => a<0     ⎩ ∆X > ∆Xmin => a>0 (1.2)
	 *          ⎩ ∆X=0 => a<0
	 *
	 *          ⎧ ∆X>0 => a>0     ⎧ ∆X < ∆Xmin => a<0 (2.1)
	 * (2) ∆V<0 ⎨ ∆X<0 =========> ⎨                          (Special-2)
	 *          ⎩ ∆X=0 => a>0     ⎩ ∆X > ∆Xmin => a>0 (2.2)
	 *
	 *          ⎧ ∆X>0 => a>0
	 * (3) ∆V=0 ⎨ ∆X<0 => a<0
	 *          ⎩ ∆X=0 => a=0 (*)
	 */
	const float a = vf>v0?   //-- (1)
			xf>x0? //-- Special-1
					xf-x0<dxMin?
							-maxA: //-- (1.1)
							+maxA: //-- (1.2)
							-maxA:
							vf<v0?   //-- (2)
									xf<x0? //-- Special-2
											xf-x0<dxMin?
													-maxA: //-- (2.1)
													+maxA: //-- (2.2)
													+maxA:
													sgn(xf-x0)*maxA;  //-- (3)

	if (a == 0) return 0; //-- No need to moving

	const float T1 = ((sgn(a)*maxV)/* <=> vMax*/ - v0)/a;

	/*
	 * k1 = -a*T1 - v0 + vf;
	 * k2 = (a/2) * T1*T1 - x0 + xf;
	 * k3 = -k1 / a;
	 * k4 = a * T1 + v0;
	 * k5 = (-a/2) * k3*k3 + a * T1 * k3 + v0 * k3 - k2;
	 * <=>
	 */

	const float k3 = T1 + (v0 + vf)/a;
	const float T2 = -((-a/2) * k3*k3 + a * T1 * k3 + v0 * k3 - (a/2) * T1*T1 + x0 - xf) / (a * T1 + v0);

	if(T2 > T1)
		return T2 + k3;
	else
	{
		/*
		 * c1 = vf - v0;
		 * c2 = xf - x0;
		 * c3 = -c1 / a;
		 * c4 = 2* v0 / a;
		 * c5 = - 0.5 * c3*c3 + (v0 * c3) / a - c2 / a;
		 * delta = c4*c4 - 4 * c5;
		 * <=>
		 */
		const float c3 = (v0 - vf) / a;
		const float c4 = 2* v0 / a;
		const float delta = c4*c4 + 2.0f*c3*c3 - 4.0f*((v0*c3)-(xf-x0))/a;

		if(delta > 0)
			return c3-c4 + (float) sqrt(delta);

		// [TODO] : check this comments, to see if there are any place that T12 need to be used.
		//      const float T11 = -c4/2 + sqrt(delta)/2;
		//      const float T12 = -c4/2 - sqrt(delta)/2;
		//      const float TF1 = 2*T11+c3;
		//      const float TF2 = 2*T12+c3;
		//      const float VF1 = -a*TF1 + 2*a*T1 + v0;
		//      const float VF2 = -a*TF2 + 2*a*T1 + v0;
		//      if (sgn(VF1) == sgn(vf))
		//        return TF1;
		//      else
		//        return TF2;
		else if (delta == 0)
			return c3-c4;
		else
			return 0;
	}

	return 0;
}

Vector2f TaskAssignment::voronoiPoseRelativeToBall(const Vector2f& VoronoiPose, const Vector2f& BallPosition, const Vector2f& radius)
{
	// change voronoi center position relative to ball position in the field
	// ball differential in the field range is scaled in voronoi range for that specific post/voronoi position
	// relation => r_p_g(t) = [(r_p_g(t0) - c) * (1 - ∆r_b_g(t,t0))] + [(r_p_g(t0) + c) * ∆r_b_g(t,t0)]
	// simplified => r_p_g(t) = r_p_g(t0) - c + [2c * ∆r_b_g(t, t0)]
	// r_p_g: global voronoi position
	// c: raidus that voronoi position can change
	// ∆r_b_g: global ball position variation while it is [0, 1]
	// t0: the initial position, i.e. loaded from the formation in case of voronoi positions

	const Vector2f& posi_voronoi_g = VoronoiPose;
	const Vector2f& posi_ball_g = BallPosition;
	const Vector2f& pose_radius_g = radius;

	// TODO: to be loaded from field dimensions
	float fieldXUpperBound = 4500;
	float fieldXLowerBound = -4500;
	float fieldYUpperBound = 3000;
	float fieldYLowerBound = -3000;

	//		float poseXLowerBound = posi_voronoi_g.x() - std::fabs(pose_radius_g.x());
	//		float poseXUpperBound = posi_voronoi_g.x() + std::fabs(pose_radius_g.x());
	//		float poseYLowerBound = posi_voronoi_g.y() - std::fabs(pose_radius_g.y());
	//		float poseYUpperBound = posi_voronoi_g.y() + std::fabs(pose_radius_g.y());

	// ratio of ball in absolute bound of its lower/upper limits
	float ballXRatioInAbsBound = (posi_ball_g.x() - fieldXLowerBound)/(fieldXUpperBound-fieldXLowerBound);
	float ballYRatioInAbsBound = (posi_ball_g.y() - fieldYLowerBound)/(fieldYUpperBound-fieldYLowerBound);

	return Vector2f(posi_voronoi_g.x() - pose_radius_g.x() + 2 * pose_radius_g.x() * ballXRatioInAbsBound,
			posi_voronoi_g.y() - pose_radius_g.y() + 2 * pose_radius_g.y() * ballYRatioInAbsBound);
}

unsigned TaskAssignment::lastNumOfPlayers()
{
	unsigned num_of_players = 0;
	for(auto& teammate: theTeammateData.teammates) {
		if(teammate.status != Teammate::PENALIZED && !teammate.isGoalkeeper)
			num_of_players++;
	}

	return num_of_players + 1; // other actives plus me!
}

bool TaskAssignment::hasGotBall()
{
	return (theTeamBallModel.isValid || theFrameInfo.getTimeSince(theTeamBallModel.timeWhenLastValid) < 3000);	// TODO: double check workability of this flag
	//	return (theFrameInfo.getTimeSince(theBallModel.timeWhenLastSeen)) < 2000;
}
