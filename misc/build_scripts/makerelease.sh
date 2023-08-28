#!/bin/bash

# paths, change these as needed
SOURCE_DIR=~/client/src
MISC_DIR=~/client/misc
BUILDS_DIR=~/client/release

# ./ClassiCube
make_nix_tar() {
  cp $SOURCE_DIR/$1 ClassiCube
  chmod +x ClassiCube
  tar -zcvf ClassiCube.tar.gz ClassiCube
  rm ClassiCube
}

# ./ClassiCube.app/Info.plist
# ./ClassiCube.app/Contents/MacOS/ClassiCube
# ./ClassiCube.app/Resources/ClassiCube.icns
make_mac_tar() {
  mkdir ClassiCube.app
  mkdir ClassiCube.app/Contents
  mkdir ClassiCube.app/Contents/MacOS
  mkdir ClassiCube.app/Contents/Resources
  
  cp $SOURCE_DIR/$1 ClassiCube.app/Contents/MacOS/ClassiCube
  chmod +x ClassiCube.app/Contents/MacOS/ClassiCube
  
  cp $MISC_DIR/info.plist ClassiCube.app/Contents/Info.plist
  cp $MISC_DIR/CCIcon.icns ClassiCube.app/Contents/Resources/ClassiCube.icns
  tar -zcvf ClassiCube.tar.gz ClassiCube.app
  rm -rf ClassiCube.app
}


mkdir -p $BUILDS_DIR
mkdir -p $BUILDS_DIR/win32 $BUILDS_DIR/win64 $BUILDS_DIR/osx32 $BUILDS_DIR/osx64
mkdir -p $BUILDS_DIR/nix32 $BUILDS_DIR/nix64 $BUILDS_DIR/mac32 $BUILDS_DIR/mac64 
mkdir -p $BUILDS_DIR/rpi32

# edge doesn't respect download attribute, and having ClassiCube.64.exe breaks plugins
cp $SOURCE_DIR/cc-w32-d3d.exe $BUILDS_DIR/win32/ClassiCube.exe
cp $SOURCE_DIR/cc-w64-d3d.exe $BUILDS_DIR/win64/ClassiCube.exe

# use tar to preserve chmod +x, so users don't have to manually do it
cd $BUILDS_DIR/osx32/
make_nix_tar cc-osx32
cd $BUILDS_DIR/osx64/
make_nix_tar cc-osx64
cd $BUILDS_DIR/nix32/
make_nix_tar cc-nix32
cd $BUILDS_DIR/nix64/
make_nix_tar cc-nix64
cd $BUILDS_DIR/rpi32/
make_nix_tar cc-rpi

# generate ClassiCube.app for mac users
cd $BUILDS_DIR/mac32/
make_mac_tar cc-osx32
cd $BUILDS_DIR/mac64/
make_mac_tar cc-osx64
