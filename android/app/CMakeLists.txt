#
# Copyright (C) The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.4.1)

# build native_app_glue as a static lib
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Werror")

add_library(classicube SHARED
        ../../src/main.c
        ../../src/IsometricDrawer.c
        ../../src/Builder.c
        ../../src/ExtMath.c
        ../../src/_ftbitmap.c
        ../../src/Utils.c
        ../../src/Camera.c
        ../../src/Game.c
        ../../src/GameVersion.c
        ../../src/Window_Android.c
        ../../src/_ftbase.c
        ../../src/Graphics_GL2.c
        ../../src/Deflate.c
        ../../src/_cff.c
        ../../src/_ftsynth.c
        ../../src/String.c
        ../../src/LWidgets.c
        ../../src/Options.c
        ../../src/Drawer2D.c
        ../../src/Server.c
        ../../src/Entity.c
        ../../src/Drawer.c
        ../../src/Vorbis.c
        ../../src/Protocol.c
        ../../src/World.c
        ../../src/SelOutlineRenderer.c
        ../../src/Platform_Posix.c
        ../../src/Platform_Android.c
        ../../src/LScreens.c
        ../../src/_truetype.c
        ../../src/_ftglyph.c
        ../../src/Model.c
        ../../src/_autofit.c
        ../../src/Vectors.c
        ../../src/HeldBlockRenderer.c
        ../../src/Inventory.c
        ../../src/Launcher.c
        ../../src/Block.c
        ../../src/LWeb.c
        ../../src/Stream.c
        ../../src/Lighting.c
        ../../src/Resources.c
        ../../src/PackedCol.c
        ../../src/Screens.c
        ../../src/Formats.c
        ../../src/_sfnt.c
        ../../src/Bitmap.c
        ../../src/EntityComponents.c
        ../../src/_pshinter.c
        ../../src/Http_Worker.c
        ../../src/MapRenderer.c
        ../../src/Audio.c
        ../../src/_ftinit.c
        ../../src/Event.c
        ../../src/Logger.c
        ../../src/Widgets.c
        ../../src/TexturePack.c
        ../../src/Menus.c
        ../../src/BlockPhysics.c
        ../../src/_psmodule.c
        ../../src/Chat.c
        ../../src/Gui.c
        ../../src/AxisLinesRenderer.c
        ../../src/Picking.c
        ../../src/_type1.c
        ../../src/_smooth.c
        ../../src/_psaux.c
        ../../src/Generator.c
        ../../src/Input.c
        ../../src/Particle.c
        ../../src/Physics.c
        ../../src/SelectionBox.c
        ../../src/EnvRenderer.c
        ../../src/Animations.c
        ../../src/LBackend.c
        ../../src/SystemFonts.c
        ../../src/Commands.c
        ../../src/EntityRenderers.c
        ../../src/AudioBackend.c
        ../../src/TouchUI.c
        ../../src/LBackend_Android.c
        ../../src/InputHandler.c
        ../../src/MenuOptions.c
        ../../src/FancyLighting.c
        ../../src/Queue.c
        )

# add lib dependencies
target_link_libraries(classicube
    android
    EGL
    GLESv2
    log
    OpenSLES
    jnigraphics)
