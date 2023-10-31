## Introduction

The 2D GUI works by showing multiple 'screens' or 'layers' on top of each other. Each 'screen' usually contains one or more widgets. (e.g. a 'ButtonWidget' for a 'Close' button)

Here's an example of multiple screens:

![image](https://user-images.githubusercontent.com/6509348/103143562-a11cee80-476c-11eb-9e24-a854b069704f.png)

The example above consists of three screens:
1. `PauseScreen` - Top layer which consists of button widgets. Also draws a background to darken the screens/layers underneath it.
2. `ChatScreen` - Text widgets in the bottom left area (Only one is currently used though)
3. `HUDScreen` - Text in the top left and hotbar at the bottom

TODO: Explain event processing architecture

TODO: Explain pointer IDs. maybe a separate input-architecture.md file?

TODO: Explain dynamic vertex buffer

## Screen interface

#### `void Init(void* screen)`

Where you initialise state and objects that lasts for the entire duration the screen is open

E.g. Hooking into events

#### `void Free(void* screen)` 

Where you undo the allocating/event hooking you did in `Init`.

#### `void ContextRecreated(void* screen)` 

Where you allocate resources used for rendering, such as fonts, textures of widgets, and the screen's dynamic vertex buffer. 

E.g. where you should call `ButtonWidget_SetConst`, `TextWidget_SetConst`, etc.

#### `void ContextLost(void* screen)`

Where you destroy/release all that resources that were allocated in `ContextRecreated`. 

Note: You **MUST** destroy all the textures/vertex buffers that you allocated, as otherwise you will get an error later with 'recreating context failed' with the Direct3D9 backend when you next try to resize the window or go fullscreen. TODO: rewrite this note

#### `void Layout(void* screen)`

Where you reposition all the elements (i.e. widgets + anything else) of this screen. 

Note: This is called when initially showing this screen, and whenever the window is resized

#### `int HandlesInputDown(void* screen, int button)` 

Called whenever a input button (keyboard/mouse/gamepad) is pressed.

#### `void OnInputUp(void* screen, int button)` 

Called whenever a input button (keyboard/mouse/gamepad) is released.

#### `int HandlesKeyPress(void* screen, char keyChar)` 

Called when a key character is entered on the keyboard. 

E.g. you'd get '$' here if user pressed 4 while holding shift)

#### `int HandlesTextChanged(void* screen, const cc_string* str)`

Called when the text changes in the on-screen keyboard. (i.e. KeyPress but for mobile text input)

#### `int HandlesMouseScroll(void* screen, float wheelDelta)`

Called when the wheel is scrolled on the mouse.

#### `int HandlesPointerDown(void* screen, int pointerID, int x, int y)`

Called whenever the left mouse or a touch finger is pressed.

#### `void OnPointerUp(void* screen, int pointerID, int x, int y)`

Called whenever the left mouse or a touch finger is released.

#### `int HandlesPointerMove(void* screen, int pointerID, int x, int y)`

Called whenever the left mouse or a touch finger is moved.

#### `void BuildMesh(void* screen)` 

Where you fill out the screen's dynamic vertex buffer with the vertices of all the widgets. 

Note: This gets called just before `Render()` whenever the `dirty` field is set to `true`. 

Note: Pointer/Input events automatically set the `dirty` field to `true` for the next frame

#### `void Update(void* screen, double delta)` 

Called just before `Render()` every frame and also provides the elapsed time since last render. 

Typically you'd use this to update simple stuff that depends on accumulated time. 
(E.g. whether the flashing caret should appear or not for input widgets)

#### `void Render(void* screen)` 

Called every frame and is where you actually draw the widgets and other stuff on-screen.

## Screen members

#### `struct ScreenVTABLE* VTABLE` 

Set to a `ScreenVTABLE` instance which implements all of the `Screen interface` functions

#### `cc_bool grabsInput` 

Whether this screen grabs all input. 

Note: If any screen grabs all input, then the mouse cursor becomes visible, W/A/S/D stops causing player movement, etc.

#### `cc_bool blocksWorld` 

Whether this screen completely and opaquely covers the game world behind it. 

E.g. loading screen and disconnect screen

#### `cc_bool closable` 

Whether this screen is automatically closed when pressing Escape. 

Note: Usually should be true for screens that are menus (e.g. pause screen)

#### `cc_bool dirty` 

Whether this screen will have `BuildMesh()` called on the next frame

Note: This should be set to `true` whenever any event/actions that might alter the screen's appearance occurs TODO explain when automatically called?

#### `int maxVertices` 

The maximum number of vertices that this screen's dynamic vertex buffer may use

#### `GfxResourceID vb` 

This screen's dynamic vertex buffer

#### `struct Widget** widgets;` 

Pointer to the array of pointers to widgets that are contained in this screen

#### `int numWidgets;` 

The number of widgets that are contained in this screen


## Screen notes

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

TODO: Move this note earlier?

# Putting it altogether

TODO Add example of screen

Simple menu example that grabs all input

More complicated example too