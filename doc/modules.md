The overall source code is structured where each .c represents a particular module. These modules are:

## 2D modules
|File|Functionality|
|--------|-------|
|Bitmap.c|Represents a 2D array of pixels (and encoding/decoding to PNG)
|Drawer2D.c|Contains a variety of drawing operations on bitmaps (including text and fonts)
|PackedCol.c|32 bit RGBA color in a format suitable for using as color component of a vertex

## Audio modules
|File|Functionality|
|--------|-------|
|Audio.c|Playing music and dig/place/step sounds, and abstracts a PCM audio playing API
|Vorbis.c| Decodes [ogg vorbis](https://xiph.org/vorbis/) audio into PCM audio samples

## Entity modules
|File|Functionality|
|--------|-------|
|Entity.c|Represents an in-game entity, and manages updating and rendering all entities
|EntityComponents.c|Various components that can be used by entities (e.g. tilt, animation, hacks state)
|Model.c|Contains the list of entity models, and provides relevant methods for entity models
|Particle.c|Represents particle effects, and manages rendering and spawning particles

## Game modules
|File|Functionality|
|--------|-------|
|Block.c|Stores properties and data for blocks (e.g. collide type, draw type, sound type)
|BlockPhysics.c|Implements simple block physics for singleplayer
|Camera.c|Represents a camera (can be first or third person)
|Chat.c|Manages sending, adding, logging and handling chat
|Game.c|Manages the overall game loop, state, and variables (e.g. renders a frame, runs scheduled tasks)
|Input.c|Manages keyboard, mouse, and touch state and events, and implements base handlers for them
|Inventory.c|Manages inventory hotbar, and ordering of blocks in the inventory menu

## Game gui modules
|File|Functionality|
|--------|-------|
|Gui.c|Describes and manages the 2D GUI elements on screen
|IsometricDrawer.c|Draws 2D isometric blocks for the hotbar and inventory UIs
|Menus.c|Contains all 2D non-menu screens (e.g. inventory, HUD, loading, chat)
|Screens.c|Contains all 2D menu screens (e.g. pause menu, keys list menu, font list menu)
|Widgets.c|Contains individual GUI widgets (e.g. button, label)

## Graphics modules
|File|Functionality|
|--------|-------|
|Builder.c|Converts a 16x16x16 chunk into a mesh of vertices
|Drawer.c|Draws the vertices for a cuboid region
|Graphics.c|Abstracts a 3D graphics rendering API

## I/O modules
|File|Functionality|
|--------|-------|
|Deflate.c|Decodes and encodes data compressed using DEFLATE (in addition to GZIP/ZLIB headers)
|Stream.c|Abstract reading and writing data to/from various sources in a streaming manner

## Launcher modules 
|File|Functionality|
|--------|-------|
|Launcher.c|Manages the overall launcher loop, state, and variables (e.g. resets pixels in areas, marks areas as needing to be redrawn)
|LScreens.c|Contains all the menus in the launcher (e.g. servers list, updates menu, main menu)
|LWeb.c|Responsible for launcher related web requests (e.g. signing in, fetching servers list)
|LWidgets.c|Contains individual launcher GUI widgets (e.g. button, label, input textbox)
|Resources.c|Responsible for checking, downloading, and creating the default assets (e.g. default.zip, sounds)

## Map modules 
|File|Description|
|--------|-------|
|Formats.c|Imports/exports a world from/to several map file formats (e.g. .cw, .dat, .lvl)
|Generator.c|Generates a new world in either a flatgrass or Minecraft Classic style
|Lighting.c|Gets lighting colors at coordinates in the world
|World.c|Manages fixed size 3D array of blocks and associated environment metadata

## Math/Physics modules 
|File|Description|
|--------|-------|
|ExtMath.c|Math functions, math constants, and a Random Number Generator
|Physics.c|AABBs and geometry intersection
|Picking.c|Performs raytracing to e.g. determine the picked/selected block in the world
|Vectors.c|Contains vector,matrix,and frustum culling

## Network modules 
|File|Description|
|--------|-------|
|Http.c|Performs GET and POST requests in the background
|Protocol.c|Implements Minecraft Classic, CPE, and WoM environment protocols
|Server.c|Manages a connection to a singleplayer or multiplayer server

## Platform modules
|File|Description|
|--------|-------|
|Logger.c|Manages logging to client.log, and dumping state in both intentional and unhandled crashes
|Platform.c|Abstracts platform specific functionality. (e.g. opening a file, allocating memory, starting a thread)
|Program.c|Parses command line arguments, and then starts either the Game or Launcher
|Window.c|Abstracts creating and managing a window (e.g. setting titlebar text, entering fullscreen)

## Rendering modules
|File|Description|
|--------|-------|
|AxisLinesRenderer.c|Renders 3 lines showing direction of each axis
|EnvRenderer.c|Renders environment of the world (clouds, sky, skybox, world sides/edges, etc)
|HeldBlockRenderer.c|Renders the block currently being held in bottom right corner
|MapRenderer.c|Renders the blocks of the world by diving it into chunks, and manages sorting/updating these chunks
|PickedPosRenderer.c|Renders an outline around the block currently being looked at
|SelectionBox.c|Renders and stores selection boxes

## Texture pack modules
|File|Functionality|
|--------|-------|
|Animations.c|Everything relating to texture animations (including default water/lava ones)
|TexturePack.c|Everything relating to texture packs (e.g. extracting .zip, terrain atlas, etc)

## Utility modules
|File|Functionality|
|--------|-------|
|Event.c|Contains all events and provies helper methods for using events
|Options.c|Retrieves options from and sets options in options.txt
|String.c|Implements operations for a string with a buffer, length, and capacity
|Utils.c|Various general utility functions
