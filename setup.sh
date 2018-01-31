#!/bin/bash

echo "Create symbolic link to the content of the submodule.."
cd "$(dirname "$0")" # switch to the submodule directory
cp -rfs $PWD/Config $PWD/../
echo "symbolic link to Config/ done."
cp -rfs $PWD/Src $PWD/../
echo "symbolic link to Src/ done."
echo "alles gut."
