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

psn00bsdk_add_executable(template GPREL ${_sources})

psn00bsdk_add_cd_image(
	iso      # Target name
	template # Output file name (= template.bin + template.cue)
	iso.xml  # Path to config file
	DEPENDS template system.cnf
)
