This is a fork of GLdc optimised for the Dreamcast port of ClassiCube, and unfortunately is essentially useless for any other project

---

# GLdc

**Development of GLdc has moved to [Gitlab](https://gitlab.com/simulant/GLdc)**

This is a partial implementation of OpenGL 1.2 for the SEGA Dreamcast for use
with the KallistiOS SDK.

It began as a fork of libGL by Josh Pearson but has undergone a large refactor
which is essentially a rewrite.

The aim is to implement as much of OpenGL 1.2 as possible, and to add additional
features via extensions.

Things left to (re)implement:

 - Spotlights (Trivial)
 - Framebuffer extension (Trivial)
 - Texture Matrix (Trivial)
 
Things I'd like to do:

 - Use a clean "gl.h"
 - Define an extension for modifier volumes
 - Add support for point sprites
 - Optimise, add unit tests for correctness

# Compiling

GLdc uses CMake for its build system, it currently ships with two "backends":

 - kospvr - This is the hardware-accelerated Dreamcast backend
 - software - This is a stub software rasterizer used for testing testing and debugging
 
To compile a Dreamcast debug build, you'll want to do something like the following:

```
mkdir dcbuild
cd dcbuild
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchains/Dreamcast.cmake -G "Unix Makefiles" ..
make
```

For a release build, replace the cmake line with with the following:
```
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchains/Dreamcast.cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
```

You will need KallistiOS compiled and configured (e.g. the KOS_BASE environment
variable must be set)

To compile for PC:

```
mkdir pcbuild
cd pcbuild
cmake -G "Unix Makefiles" ..
make
```
 
# Special Thanks!

 - Massive shout out to Hayden Kowalchuk for diagnosing and fixing a large number of bugs while porting GL Quake to the Dreamcast. Absolute hero!  
