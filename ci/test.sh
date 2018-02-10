#!/bin/bash
set -ev

echo -e "Download und extract sourcemod\n"
wget "http://www.sourcemod.net/latest.php?version=$1&os=linux" -O sourcemod.tar.gz
tar -xzf sourcemod.tar.gz

echo -e "Give compiler rights for compile\n"
chmod +x addons/sourcemod/scripting/spcomp

echo -e "\nCompile plugins"
echo -e "Compiling addons/sourcemod/scripting/store.sp..."
addons/sourcemod/scripting/spcomp -O2 -v2 addons/sourcemod/scripting/store.sp
echo -e "\nCompiling addons/sourcemod/scripting/store-trade.sp..."
addons/sourcemod/scripting/spcomp -O2 -v2 addons/sourcemod/scripting/trade.sp
echo -e "\nCompiling addons/sourcemod/scripting/thirdperson.sp..."
addons/sourcemod/scripting/spcomp -O2 -v2 addons/sourcemod/scripting/thirdperson.sp
