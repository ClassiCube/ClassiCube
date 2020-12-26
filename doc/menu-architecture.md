### Introduction

The 2D gui works by showing multiple 'screens' or 'layers' on top of each other. Each 'screen' usually contains one or more widgets. (e.g. a 'ButtonWidget' for a 'Close' button)

Here's an example of multiple screens:

![image](https://user-images.githubusercontent.com/6509348/103143562-a11cee80-476c-11eb-9e24-a854b069704f.png)

The example above consists of three screens:
1. `PauseScreen` - Top layer which consists of button widgets. Also draws a background to darken the screens/layers underneath it.
2. `ChatScreen` - Text widgets in the bottom left area (Only one is currently used though)
3. `HUDScreen` - Text in the top left and hotbar at the bottom

### Screen interface

`Init` - Where you initialise stuff that lasts for the entire duration the screen is open, e.g. hooking into events.

`Free` - Where you undo the allocating/event hooking you did in Init.

`ContextRecreated` - Where you actually allocate stuff like textures of widgets and the screen's dynamic vertex buffer. It's where you should call stuff like ButtonWidget_SetConst, TextWidget_SetConst, etc.

`ContextLost` - Where you destroy what you allocated in `ContextRecreated`. You **MUST** destroy all the textures/vertex buffers that you allocated, as otherwise you will get an error later with 'recreating context failed' with the Direct3D9 backend when you next try to resize the window or go fullscreen.

`Layout` - Where you reposition all the elements (i.e. widgets + anything else) of this screen. (Will always be called when initially showing this screen and whenever window resizes)

`HandlesInputDown/HandlesInputUp` - Called whenever a keyboard or mouse button is pressed/released.

`HandlesKeyPress` - Called when a key character is entered on the keyboard. (e.g. you'd get '$' here if user pressed 4 while holding shift)

`HandlesTextChanged` - Called when the text changes in the on-screen keyboard. (i.e. KeyPress but for mobile text input)

`HandlesMouseScroll` - Called when the wheel is scrolled on the mouse.

`HandlesPointerDown/HandlesPointerUp/HandlesPointerMove` - Called whenever the left mouse or a touch finger is pressed/released/moved.

`BuildMesh` - Where you fill out the screen's dynamic vertex buffer with the vertices of all the widgets. This gets called just before `Render` whenever s->dirty is set to true. (pointer/input events automatically set s->dirty to true for the next frame)

`Update` - Called just before `Render` every frame and also provides the elapsed time since last render. Typically you'd use this to update simple stuff that depends on accumulated time. (e.g. whether the flashing caret should appear or not for input widgets)

`Render` - Called every frame and is where you actually draw the widgets and other stuff on-screen. Don't forget to call Gfx_SetTexturing(true) before rendering widgets and Gfx_SetTexturing(false) once you're done.

### Screen members

`VTABLE` - Set to a ScreenVTABLE instance which implements all of the `Screen interface` functions

`grabsInput` - Whether this screen grabs all input. If any screen grabs all input, then the mouse cursor becomes visible, W/A/S/D stops causing player movement, etc.

`blocksWorld` - Whether this screen completely and opaquely covers the game world behind it. (e.g. loading screen and disconnect screen)

`closable` - Whether this screen is automatically closed when pressing Escape. (Usually should be true for screens that are menus, e.g. pause screen)

`dirty` - Whether this screen will have `BuildMesh` called on the next frame

`maxVertices` - The maximum number of vertices that this screen's dynamic vertex buffer may use

`vb` - This screen's dynamic vertex buffer


### Screen notes

* When the screen is shown via `Gui_Add`, the following functions are called
  1) Init
  2) ContextRecreated
  3) Layout
  4) HandlesPointerMove

* When the screen is removed via `Gui_Remove`, the following functions are called
  1) ContextLost
  2) Free

* When the screen is refreshed via `Gui_Refresh`, the following functions are called
  1) ContextLost
  2) ContextRecreated
  3) Layout

Note: Whenever `default.png` (font texture) changes, `Gui_Refresh` is called for all screens. Therefore, fonts should usually be allocated/freed in `ContextRecreated/ContextLost` instead of `Init/Free` to ensure that the screen still looks correct after the texture pack changes.
