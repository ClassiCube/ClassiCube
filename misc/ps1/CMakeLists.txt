cmake_minimum_required(VERSION 3.21)

project(
	ClassiCube
	LANGUAGES    C ASM
	VERSION      1.0.0
	DESCRIPTION  "ClassiCube PS1 port"
	HOMEPAGE_URL "https://classicube.net"
)

add_definitions(-DPLAT_PS1)
file(GLOB _sources ../../src/*.c)

psn00bsdk_add_executable(ClassiCube-PS1 GPREL ${_sources})

psn00bsdk_add_cd_image(
	iso        # Target name
	ClassiCube-PS1 # Output file name (= ClassiCube-PS1.bin + ClassiCube-PS1.cue)
	iso.xml    # Path to config file
	DEPENDS ClassiCube-PS1 system.cnf
)
