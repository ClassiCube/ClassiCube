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

psn00bsdk_add_executable(ClassiCube GPREL ${_sources})

psn00bsdk_add_cd_image(
	iso        # Target name
	ClassiCube # Output file name (= ClassiCube.bin + ClassiCube.cue)
	iso.xml    # Path to config file
	DEPENDS ClassiCube system.cnf
)
