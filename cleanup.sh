#!/bin/bash

# TODO: put this script in setup.sh as an argument --cleanup

dirs="Config/ Src/"
cd "$(dirname "$0")" # switch to the submodule directory
config_files="$(find $dirs -type f)"

#echo "${config_files}"
cd ..
rm -rvf ${PWD}/${config_files}
git checkout -- $PWD/Config/Locations/Default/modules.cfg
git checkout -- $PWD/Src/Modules/BehaviorControl/BehaviorControl2015/BehaviorControl2015.cpp
git checkout -- $PWD/Src/Modules/BehaviorControl/BehaviorControl2015/BehaviorControl2015.h
git checkout -- $PWD/Src/Modules/BehaviorControl/LEDHandler/LEDHandler.cpp
git checkout -- $PWD/Src/Representations/BehaviorControl/Role.h

