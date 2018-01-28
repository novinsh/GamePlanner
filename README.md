# Game Planner
----
This is a module for [B-Human](http://b-human.de) framework developed by [MRL-SPL](http://mrl-spl.ir) team.
This module is mainly inspired by the "Positioning to Win" method at [Robocup Simulation3D league](http://http://robocup.org/leagues/23).
Having a general framework for multi-agent coordination is an essential part of good soccer playing team of robots. 

[![Demo](https://j.gifs.com/qYLR5G.gif)](https://youtu.be/gA6PGvBHR9w)
> You can watch the video at [youtube](https://youtu.be/gA6PGvBHR9w).


## Requirements

For using the code, you need to get [B-Human](http://github.com/bhuman/BHumanCodeRelease) code release and checkout to coderelease2016

    git clone git@github.com:bhuman/BHumanCodeRelease.git
    git checkout coderelease2016

### Installation

Get Game Planner code

    git clone git@github.com:novinsh/GamePlanner.git

Copy GamePlanner files to B-Human code (Src and Config directories).

Open Config/Locations/Default/modules.cfg
Add AgentTask as representation and TaskAssignment as provider to modules.cfg
    
      {representation = AgentTask; provider = TaskAssignment;}

## Running

Make the code and run it as instructed in B-Human coderelease
Note: players list in Config/Locations/Default/taskAssignment.cfg file must sync with the number of players in the game

## Usage

A simple example has written in Src/BehaviorControl/BehaviorControl2015/BehaviorControl2015.cpp for understanding how to use the datas that provided by task assignment module

For showing task assignment module debug on SimRobot, just enter following command in SimRobot's command line
    
    vfd worlState module:TaskAssignment on

> Ready State Screenshot 

![alt example](http://piry.site/github/readyState.jpg "Ready State ScreenShot")

> Playing State Screenshot

![alt example](http://piry.site/github/playingState.jpg "Playing State ScreenShot")

    
For changing the formations, go to Config/Formations and run PlanEditor

[![Plan Editor](https://j.gifs.com/nr6QW4.gif)](https://youtu.be/lHQWDaZeDZI)
> Learn more at [github](http://github.com/ArefMq/VoronoiGridEditor)


## Authors

* **[Novin Shahroudi](mailto:novin@ut.ee)** 
* **[Ali Piry](mailto:a.piry@mrl-spl.ir)** 

## Contributing

1. Fork it!
2. Create your feature branch: `git checkout -b my-new-feature`
3. Commit your changes: `git commit -am 'Add some feature'`
4. Push to the branch: `git push origin my-new-feature`
5. Submit a pull request :P

## License

This project is licensed under the MIT License 

