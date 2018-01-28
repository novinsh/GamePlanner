# Game Planner
----
This is a module developed by [MRL-SPL](http://mrl-spl.ir) team based on [B-Human](http://b-human.de) framework 2016.
This module is mainly inspired by the "Positioning to Win" method at [Robocup Simulation3D league](http://http://robocup.org/leagues/23).
It calculates optimal assignment of tasks for a team of robots in a distributed setting.   

[![Demo](https://j.gifs.com/qYLR5G.gif)](https://youtu.be/gA6PGvBHR9w)
> You can watch the video at [youtube](https://youtu.be/gA6PGvBHR9w).


## Requirements

You need to get [B-Human](http://github.com/bhuman/BHumanCodeRelease) code release and checkout to coderelease2016

    git clone git@github.com:bhuman/BHumanCodeRelease.git
    git checkout coderelease2016

## Installation

### Copy
1. Get Game Planner code

    git clone [path to this repo] [sub dir]

2. Copy GamePlanner files to B-Human code (Src and Config directories).

3. Add AgentTask as representation and TaskAssignment as provider to 
Config/Locations/Default/modules.cfg
    
      {representation = AgentTask; provider = TaskAssignment;}

### git submodule
1. Add submodule to your current B-Human project

    git submodule add [path to this repo] [sub dir]

2. Run the ```setup.sh``` inside the fetched submodule. This will create a symbolic
link to the submodule automatically.
 
3. Add AgentTask as representation and TaskAssignment as provider to 
Config/Locations/Default/modules.cfg
    
      {representation = AgentTask; provider = TaskAssignment;}

Learn more about git [submodule](https://github.com/NebuPookins/git-submodule-tutorial)

## Running

Make the code and run it as instructed in B-Human coderelease

Note: players list in ```Config/Locations/Default/taskAssignment.cfg``` file must sync with the number of players in the game

## Usage

A simple example has written in ```Src/BehaviorControl/BehaviorControl2015/BehaviorControl2015.cpp``` to demonstrate the usage of the data provided by task assignment module

To enable debug drawings of the module on worldState fieldview, enter following command in SimRobot's console:
    
    vfd worlState module:TaskAssignment on

Following screenshot demonstrates the assignments in 
the Playing state of the game with black dots (formation points) and the red 
lines representing the assigned position for each agent. LD: Leader and SUP: Supporter 

> Playing State Screenshot

![alt example](http://piry.site/github/playingState.jpg "Playing State ScreenShot")

    
For changing the formations, go to ```Config/Formations``` and run PlanEditor

[![Plan Editor](https://j.gifs.com/nr6QW4.gif)](https://youtu.be/lHQWDaZeDZI)
> Learn more about [PlanEditor](http://github.com/ArefMq/VoronoiGridEditor)


## Authors

* **[Novin Shahroudi](mailto:n.shahroudi@mrl-spl.ir)** 
* **[Ali Piry](mailto:a.piry@mrl-spl.ir)** 


## License

This project is licensed under the MIT License 

