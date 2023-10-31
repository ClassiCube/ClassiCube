The overall source code is structured where each .c represents a particular module. These modules are:

TODO: explain multiple backends for some Modules

## 2D modules
|Module|Functionality|
|--------|-------|
|Bitmap|Represents a 2D array of pixels (and encoding/decoding to PNG)
|Drawer2D|Contains a variety of drawing operations on bitmaps (including text and fonts)
|PackedCol|32 bit RGBA color in a format suitable for using as color component of a vertex
|SystemFonts|Drawing, measuring, and retrieving the list of platform/system specific fonts

## Audio modules
|Module|Functionality|
|--------|-------|
|Audio|Playing music and dig/place/step sounds, and abstracts a PCM audio playing API
|Vorbis| Decodes [ogg vorbis](https://xiph.org/vorbis/) audio into PCM audio samples

## Entity modules
|Module|Functionality|
|--------|-------|
|Entity|Represents an in-game entity, and manages updating and rendering all entities
|EntityComponents|Various components that can be used by entities (e.g. tilt, animation, hacks state)
|Model|Contains the list of entity models, and provides relevant methods for entity models
|Particle|Represents particle effects, and manages rendering and spawning particles

## Game modules
|File|Functionality|
|--------|-------|
|Block|Stores properties and data for blocks (e.g. collide type, draw type, sound type)
|BlockPhysics|Implements simple block physics for singleplayer
|Camera|Represents a camera (can be first or third person)
|Chat|Manages sending, adding, logging and handling chat
|Game|Manages the overall game loop, state, and variables (e.g. renders a frame, runs scheduled tasks)
|Input|Manages keyboard, mouse, and touch state and events, and implements base handlers for them
|Inventory|Manages inventory hotbar, and ordering of blocks in the inventory menu

## Game gui modules
|File|Functionality|
|--------|-------|
|Gui|Describes and manages the 2D GUI elements on screen
|IsometricDrawer|Draws 2D isometric blocks for the hotbar and inventory UIs
|Menus|Contains all 2D non-menu screens (e.g. inventory, HUD, loading, chat)
|Screens|Contains all 2D menu screens (e.g. pause menu, keys list menu, font list menu)
|Widgets|Contains individual GUI widgets (e.g. button, label)

## Graphics modules
|Module|Functionality|
|--------|-------|
|Builder|Converts a 16x16x16 chunk into a mesh of vertices
|Drawer|Draws the vertices for a cuboid region
|Graphics|Abstracts a 3D graphics rendering API

## I/O modules
|Module|Functionality|
|--------|-------|
|Deflate|Decodes and encodes data compressed using DEFLATE (in addition to GZIP/ZLIB headers)
|Stream|Abstract reading and writing data to/from various sources in a streaming manner

## Launcher modules 
|Module|Functionality|
|--------|-------|
|Launcher|Manages the overall launcher loop, state, and variables (e.g. resets pixels in areas, marks areas as needing to be redrawn)
|LBackend|Handles the rendering of widgets and forwarding input events to screens/menus
|LScreens|Contains all the menus in the launcher (e.g. servers list, updates menu, main menu)
|LWeb|Responsible for launcher related web requests (e.g. signing in, fetching servers list)
|LWidgets|Contains individual launcher GUI widgets (e.g. button, label, input textbox)
|Resources|Responsible for checking, downloading, and creating the default assets (e.g. default.zip, sounds)

## Map modules 
|Module|Description|
|--------|-------|
|Formats|Imports/exports a world from/to several map file formats (e.g. .cw, .dat, .lvl)
|Generator|Generates a new world in either a flatgrass or Minecraft Classic style
|Lighting|Gets lighting colors at coordinates in the world
|World|Manages fixed size 3D array of blocks and associated environment metadata

## Math/Physics modules 
|Module|Description|
|--------|-------|
|ExtMath|Math functions, math constants, and a Random Number Generator
|Physics|AABBs and geometry intersection
|Picking|Performs raytracing to e.g. determine the picked/selected block in the world
|Vectors|Contains vector,matrix,and frustum culling

## Network modules 
|Module|Description|
|--------|-------|
|Http|Performs GET and POST requests in the background
|Protocol|Implements Minecraft Classic, CPE, and WoM environment protocols
|Server|Manages a connection to a singleplayer or multiplayer server
|SSL|Performs SSL/TLS encryption and decryption

## Platform modules
|Module|Description|
|--------|-------|
|Logger|Manages logging to client.log, and dumping state in both intentional and unhandled crashes
|Platform|Abstracts platform specific functionality. (e.g. opening a file, allocating memory, starting a thread)
|Program|Parses command line arguments, and then starts either the Game or Launcher
|Window|Abstracts creating and managing a window (e.g. setting titlebar text, entering fullscreen)

## Rendering modules
|Module|Description|
|--------|-------|
|AxisLinesRenderer|Renders 3 lines showing direction of each axis
|EnvRenderer|Renders environment of the world (clouds, sky, skybox, world sides/edges, etc)
|HeldBlockRenderer|Renders the block currently being held in bottom right corner
|MapRenderer|Renders the blocks of the world by diving it into chunks, and manages sorting/updating these chunks
|PickedPosRenderer|Renders an outline around the block currently being looked at
|SelectionBox|Renders and stores selection boxes

## Texture pack modules
|Module|Functionality|
|--------|-------|
|Animations|Everything relating to texture animations (including default water/lava ones)
|TexturePack|Everything relating to texture packs (e.g. extracting .zip, terrain atlas, etc)

## Utility modules
|Module|Functionality|
|--------|-------|
|Event|Contains all events and provies helper methods for using events
|Options|Retrieves options from and sets options in options.txt
|String|Implements operations for a string with a buffer, length, and capacity
|Utils|Various general utility functions
