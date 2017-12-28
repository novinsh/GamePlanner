/**
 * @file AgentTask.cpp
 *
 * Result of task assignment module
 *
 * @author <a href="mailto:novinsha@gmail.com">Novin Shahroudi</a>
 * @date June 2017
 */

#include "AgentTask.h"
#include <iostream>
#include <fstream>
#include <cmath>

AgentTask::AgentTask()
{
	_currentVoronoiPose = Vector2f(-1000,0); //FIXME: Remove this after porting role/post assignment.
}

const Pose2f& AgentTask::converToPoint(const Pose2f& p) const
{
	if (!_cells.size())
		throw ("Tiles is not initialized...");
	return _cells[converToId(p)].pointer();
}

const VoronoiCell& AgentTask::converToCell(const Pose2f& p) const
{
	if (!_cells.size())
		throw("Title is not initialized...");
	return _cells[converToId(p)];
}

unsigned int AgentTask::converToId(const Pose2f& p, const unsigned& hysID) const
{
	if (!_cells.size())
		throw("Title is not initialized...");

	const int thre=200;
	//-- Initializing the counter
	float distance;

	if(hysID == 0)
		distance=(p - _cells[0].globalPose()).translation.norm() - thre;
	else
		distance=(p - _cells[0].globalPose()).translation.norm() + thre;

	unsigned id=0;

	for (unsigned i=0; i<_cells.size(); i++)
	{
		float d;

		if(hysID == i)
			d = (p - _cells[i].globalPose()).translation.norm() - thre;
		else
			d = (p - _cells[i].globalPose()).translation.norm() + thre;


		if (distance > d)
		{
			distance = d;
			id = i;
		}
	}

	return id;
}

bool AgentTask::load(const std::string& configAddress)
{
	class CFGReader
	{
	public:
		int id, x, y, cx, cy, num;
		std::string s;
		bool textMode;
		bool isOk;
		char varCounter;

		void pushAValue(char i, int value)
		{
			switch (i)
			{
			case 0:
				id = value;
				break;
			case 1:
				x = value;
				break;
			case 2:
				y = value;
				break;
			case 3:
				num = value;
				break;
			case 4:
				cx = value;
				break;
			case 5:
				cy = value;
				break;
			}
		}

		void processLine(const char* str)
		{
			if (!str) return;

			isOk = false;
			s = "";
			id = x = y = cx = cy = num = 0;
			textMode = false;

			varCounter=0;
			int varValue=0;
			bool neg = false;

			unsigned int i;

			for (i=0; str[i] != '\0'; i++)
			{
				const char c = str[i];

				if (c == '#' || c==';') //-- A Comment
					break;

				if (c == ' ')
					continue;

				if (c == ':')
				{
					textMode = true;
					break;
				}

				if (c == ',') //-- Push An other
				{
					if (varCounter < 6)
						pushAValue(varCounter, neg?(-1*varValue):varValue);
					varValue = 0;
					varCounter++;
					neg = false;
					continue;
				}

				if (c == '-')
				{
					neg = true;
					continue;
				}

				varValue = varValue*10 + (c - 48);
			}

			if (varValue != 0)
			{
				pushAValue(varCounter, neg?(-1*varValue):varValue);
			}

			if (textMode)
			{
				for (i++; str[i] != '\0'; i++)
				{
					s += str[i];
				}
			}

			if (varCounter < 4)
			{
				cx = x;
				cy = y;
			}

			if (varCounter < 2)
				isOk = false;
			else
				isOk = true;
		}
	} cfgReader;

	std::ifstream file(configAddress.c_str(), std::ios::in);
	if (!file)
	{
		std::cerr << "[Loading Grid Error] Config \'" << configAddress.c_str() << "\' File not found...\n";
		return false;
	}

	_cells.clear();

	char sz[255];
	while (!file.eof())
	{
		file.getline(sz, 255);
		cfgReader.processLine(sz);
		if (!cfgReader.isOk) continue;

		VoronoiCell p = VoronoiCell();

		p.set(Pose2f(cfgReader.x, cfgReader.y), Pose2f(cfgReader.cx, cfgReader.cy));
		p.setName(cfgReader.s);
		p.setRegionId(cfgReader.id);
		p.setNumOfSup(cfgReader.num);

		_cells.push_back(p);
	}
	return true;
}

void AgentTask::getTilesFromFile(const std::string& configAddress, std::vector<VoronoiCell>& tiles)
{
	class CFGReader
	{
	public:
		int id, x, y, cx, cy, num;
		std::string s;
		bool textMode;
		bool isOk;
		char varCounter;

		void pushAValue(char i, int value)
		{
			switch (i)
			{
			case 0:
				id = value;
				break;
			case 1:
				x = value;
				break;
			case 2:
				y = value;
				break;
			case 3:
				num = value;
				break;
			case 4:
				cx = value;
				break;
			case 5:
				cy = value;
				break;
			}
		}

		void processLine(const char* str)
		{
			if (!str) return;

			isOk = false;
			s = "";
			id = x = y = cx = cy = num = 0;
			textMode = false;

			varCounter=0;
			int varValue=0;
			bool neg = false;

			unsigned int i;

			for (i=0; str[i] != '\0'; i++)
			{
				const char c = str[i];

				if (c == '#' || c==';') //-- A Comment
					break;

				if (c == ' ')
					continue;

				if (c == ':')
				{
					textMode = true;
					break;
				}

				if (c == ',') //-- Push An other
				{
					if (varCounter < 6)
						pushAValue(varCounter, neg?(-1*varValue):varValue);
					varValue = 0;
					varCounter++;
					neg = false;
					continue;
				}

				if (c == '-')
				{
					neg = true;
					continue;
				}

				varValue = varValue*10 + (c - 48);
			}

			if (varValue != 0)
			{
				pushAValue(varCounter, neg?(-1*varValue):varValue);
			}

			if (textMode)
			{
				for (i++; str[i] != '\0'; i++)
				{
					s += str[i];
				}
			}

			if (varCounter < 4)
			{
				cx = x;
				cy = y;
			}

			if (varCounter < 2)
				isOk = false;
			else
				isOk = true;
		}
	} cfgReader;

	std::ifstream file(configAddress.c_str(), std::ios::in);
	if (!file)
	{
		std::cerr << "[Loading Grid Error] Config \'" << configAddress.c_str() << "\' File not found...\n";
		return;
	}

	tiles.clear();

	char sz[255];
	while (!file.eof())
	{
		file.getline(sz, 255);
		cfgReader.processLine(sz);
		if (!cfgReader.isOk) continue;

		VoronoiCell p = VoronoiCell();

		p.set(Pose2f(cfgReader.x, cfgReader.y), Pose2f(cfgReader.cx, cfgReader.cy));
		p.setName(cfgReader.s);
		p.setRegionId(cfgReader.id);
		p.setNumOfSup(cfgReader.num);

		tiles.push_back(p);
	}
}
