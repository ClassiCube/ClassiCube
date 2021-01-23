This folder contains addtitional information and resources for the game

## Icons

CCicon.ico is the basis icon for the other icon files

mac_icon_gen.cs/linux_icon_gen.cs use CCIcon.ico to generate icon files for macOS/Linux

TODO: Explain how to compile your own icon for all the platforms

## Build scripts

|File|Description|
|--------|-------|
|buildbot.sh | Compiles the game to optimised executables (with icons) |
|buildbot_plugin.sh | Compiles specified plugin for various platforms |
|buildtestplugin.sh | Example script for how to use buildbot_plugin.sh  | 
|makerelease.sh | Packages the executables to produce files for a release |
|notify.py | Notifies a user on Discord if buildbot fails |

## Other files

Info.plist is the Info.plist you would use when creating an Application Bundle for macOS.