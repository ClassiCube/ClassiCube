#ifndef CC_EVENTS_H
#define CC_EVENTS_H
#include "Event.h"
#include "Typedefs.h"
/* Contains the various events that are raised by the client.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Raised when an entity is spawned in the current world. */
Event_EntityID EntityEvents_Added;
/* Raised when an entity is despawned from the current world. */
Event_EntityID EntityEvents_Removed;

/* Raised when a tab list entry is created. */
Event_EntityID TabListEvents_Added;
/* Raised when a tab list entry is modified. */
Event_EntityID TabListEvents_Changed;
/* Raised when a tab list entry is removed. */
Event_EntityID TabListEvents_Removed;

/* Raised when the terrain atlas ("terrain.png") is changed. */
Event_Void TextureEvents_AtlasChanged;
/* Raised when the texture pack is changed. */
Event_Void TextureEvents_PackChanged;
/* Raised when a file in a texture pack is changed. (such as "terrain.png", "rain.png") */
Event_Stream TextureEvents_FileChanged;

/* Raised when the user changed their view/fog distance. */
Event_Void GfxEvents_ViewDistanceChanged;
/* Raised when the projection matrix changes. */
Event_Void GfxEvents_ProjectionChanged;
/* Event raised when a context is destroyed after having been previously lost. */
Event_Void GfxEvents_ContextLost;
/* Event raised when a context is recreated after having been previously lost. */
Event_Void GfxEvents_ContextRecreated;

/* Raised when the user changes a block. */
Event_Block UserEvents_BlockChanged;
/* Raised when when the hack permissions of the player changes. */
Event_Void UserEvents_HackPermissionsChanged;
/* Raised when when the held block in hotbar changes. */
Event_Void UserEvents_HeldBlockChanged;

/* Raised when the block permissions(can place or delete a block) for the player changes. */
Event_Void BlockEvents_PermissionsChanged;
/* Raised when a block definition is changed. */
Event_Void BlockEvents_BlockDefChanged;

/* Raised when the player joins and begins loading a new world. */
Event_Void WorldEvents_NewMap;
/* Raised when a portion of the world is read and decompressed, or generated.
The floating point argument is progress (from 0 to 1). */
Event_Real32 WorldEvents_MapLoading;
/* Raised when new world has finished loading and the player can now interact with it. */
Event_Void WorldEvents_MapLoaded;
/* Raised when an environment variable of the world is changed by the user, CPE, or WoM config. */
Event_Int32 WorldEvents_EnvVarChanged;

/* Environment variable identifiers*/
typedef Int32 EnvVar;
#define EnvVar_EdgeBlock 0
#define EnvVar_SidesBlock 1
#define EnvVar_EdgeHeight 2
#define EnvVar_SidesOffset 3
#define EnvVar_CloudsHeight 4
#define EnvVar_CloudsSpeed 5

#define EnvVar_WeatherSpeed 6
#define EnvVar_WeatherFade 7
#define EnvVar_Weather 8
#define EnvVar_ExpFog 9
#define EnvVar_SkyboxHorSpeed 10
#define EnvVar_SkyboxVerSpeed 11

#define EnvVar_SkyCol 12
#define EnvVar_CloudsCol 13
#define EnvVar_FogCol 14
#define EnvVar_SunCol 15
#define EnvVar_ShadowCol 16




/*
   The Open Toolkit Library License
  
   Copyright (c) 2006 - 2009 the Open Toolkit library.
  
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
   the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:
  
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
  
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.
   */
   
/* Raised when the window is moved. */
Event_Void WindowEvents_OnMove;
/* Raised whene the window is resized. */
Event_Void WindowEvents_OnResize;
/* Raised when the window is about to close. */
Event_Void WindowEvents_OnClosing;
/* Raised after the window has closed. */
Event_Void WindowEvents_OnClosed;
/* Raised the visibility of the window changes. */
Event_Void WindowEvents_OnVisibleChanged;
/* Raised when the focus of the window changes. */
Event_Void WindowEvents_OnFocusedChanged;
/* Raised when the WindowState of the window changes. */
Event_Void WindowEvents_OnWindowStateChanged;
/* Raised whenever the mouse cursor leaves the bounds of the window. */
Event_Void WindowEvents_OnMouseLeave;
/* Raised whenever the mouse cursor enters the bounds of the window. */
Event_Void WindowEvents_OnMouseEnter;

/* Raised when a character is typed. Arg is a character. */
Event_Int32 KeyEvents_KeyPress;
/* Raised when a key is pressed. Arg is a member of Key enumeration. */
Event_Int32 KeyEvents_KeyDown;
/* Raised when a key is released. Arg is a member of Key enumeration. */
Event_Int32 KeyEvents_KeyUp;

/*Raised when mouse position is changed. Arg is delta from last position. */
Event_MouseMove MouseEvents_Move;
/* Raised when a button is pressed. Arg is a member of MouseButton enum. */
Event_Int32 MouseEvents_ButtonDown;
/* Raised when a button is released. Arg is a member of MouseButton enum. */
Event_Int32 MouseEvents_ButtonUp;
/* Raised when mouse wheel is moved / scrolled. Arg is wheel delta. */
Event_Real32 MouseEvents_WheelChanged;
#endif