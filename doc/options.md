Listed below are all of the options supported in options.txt

# Global options

### HTTP options
|Name|Default|Description|
|--|--|--|
`http-no-https`|`false`|Whether `https://` support is disabled<br>**Disabling means your account password is transmitted in plaintext**
`https-verify`|`false`|Whether to validate 'https://' certificates returned by webservers<br>**Disabling this is a bad idea, but is still less bad than `http-no-https`**

### Text drawing options
|Name|Default|Description|
|--|--|--|
`gui-arialchatfont`|`false`|Whether system fonts are used instead of default.png for drawing most text
`gui-blacktextshadows`|`false`|Whether text shadow color is pure black instead of faded
`gui-fontname`||Name of preferred system font to use for text rendering

### Window options
|Name|Default|Description|
|--|--|--|
`landscape-mode`|`false`|Whether to force landscape orientation<br>**Only supported on Android/iOS**

# Launcher options

### General options
|Name|Default|Description|
|--|--|--|
`autocloselauncher`|`false`|Whether to automatically close the launcher after the game is started
`launcher-cc-username`||Account username
`launcher-cc-password`||Encrypted account password
`nostalgia-classicbg`|false`|Whether to draw classic minecraft.net like background

### Web options
|Name|Default|Description|
|--|--|--|
`server-services`|`https://www.classicube.net/api`|URL for account services (login and server list)
`launcher-session`||Encrypted session cookie<br>Valid session cookies avoids MFA/2FA check

### Theme colors
|Name|Description|
|--|--|
`launcher-back-col`|Background/base color
`launcher-btn-border-col`|Color of border arround buttons
`launcher-btn-fore-active-col`|Color of button when mouse is hovered over
`launcher-btn-fore-inactive-col`|Normal color of buttons
`launcher-btn-highlight-inactive-col`|Color of line at top of buttons

### Direct connect
|Name|Description|
|--|--|
`launcher-dc-username`|Username to use when connecting
`launcher-dc-ip`|IP address or hostname to connect to
`launcher-dc-port`|Port to connect on
`launcher-dc-mppass`|Encrypted mppass to use when connecting

### Resume
|Name|Description|
|--|--|
`launcherserver`|Name of server to connect to<br>Hint for user, not actually used for connecting
`launcher-username`|Username to use when connecting
`launcher-ip`|IP address or hostname to connect to
`launcher-port`|Port to connect on
`launcher-mppass`|Encrypted mppass to use when connecting

# Game options

### Audio options
|Name|Default|Description|
|--|--|--|
`soundsvolume`|`0` for webclient<br>`100` elsewhere|Volume of game sounds (e.g. break/walk sounds)<br>Volume must be between 0 and 100
`musicvolume`|`0` for webclient<br>`100` elsewhere|Volume of game background music<br>Volume must be between 0 and 100
`music-mindelay`|`120` (2 minutes)|Minimum delay before next music track is played <br>Delay must be between 0 and 3600
`music-maxdelay`|`420` (7 minutes)|Maximum delay before next music track is played <br>Delay must be between 0 and 3600

### Block physics options
|Name|Default|Description|
|--|--|--|
`singleplayerphysics`|`true`|Whether block physics are enabled in singleplayer

### Chat options
|Name|Default|Description|
|--|--|--|
`chat-logging`|`false` for mobile/web<br>`true` elsewhere|Whether to log chat messages to disc

### HTTP options
|Name|Default|Description|
|--|--|--|
`http-skinserver`|`http://classicube.s3.amazonaws.com/skin`|URL where player skins are downloaded from

### Map rendering options
|Name|Default|Description|
|--|--|--|
`gfx-smoothlighting`|`false`|Whether smooth/advanced lighting is enabled
`gfx-maxchunkupdates`|`30`|Max number of chunks built in one frame<br>Must be between 4 and 1024

### Camera options
|Name|Default|Description|
|--|--|--|
`mousesensitivity`|`40` for Windows<br>`30` elsewhere|How sensitive camera rotation is to mouse movement<br>Sensitivity must be between 1 and 200
`hacks-fov`|`70`|Field of view<br>Must be between 1 and 179
`hacks-cameraclipping`|`true`|Whether third person camera is clipped to not go inside blocks
`invertmouse`|`true`|Whether vertical mouse movement direction is inverted
`camera-smooth`|`false`|Whether smooth camera mode is enabled
`cameramass`|`20`|How smooth the smooth camera is<br>Value must be between 1 and 100

### Game options
|Name|Default|Description|
|--|--|--|
./Game.c:       Game_ClassicMode       = Options_GetBool(OPT_CLASSIC_MODE, false);
./Game.c:       Game_ClassicHacks      = Options_GetBool(OPT_CLASSIC_HACKS, false);
./Game.c:       Game_AllowCustomBlocks = Options_GetBool(OPT_CUSTOM_BLOCKS, true);
./Game.c:       Game_UseCPE            = Options_GetBool(OPT_CPE, true);
./Game.c:       Game_SimpleArmsAnim    = Options_GetBool(OPT_SIMPLE_ARMS_ANIM, false);
./Game.c:       Game_ViewBobbing       = Options_GetBool(OPT_VIEW_BOBBING, true);
./Game.c:       Game_ViewDistance     = Options_GetInt(OPT_VIEW_DISTANCE, 8, 4096, 512);
./Game.c:       Game_BreakableLiquids = !Game_ClassicMode && Options_GetBool(OPT_MODIFIABLE_LIQUIDS, false);
./Game.c:       Game_AllowServerTextures = Options_GetBool(OPT_SERVER_TEXTURES, true);

### Hacks options
|Name|Default|Description|
|--|--|--|
`hacks-hacksenabled`|`true`|Whether hacks are enabled at all<br>Has no effect in 'classic only' game mode
`hacks-speedmultiplier`|`10.0`|Speed multiplier/factor when speedhacks are active<br>Multiplier must be between 0.1 and 50.0
`hacks-pushbackplacing`|`false`|Whether to enable pushback placing mode
`hacks-noclipslide`|`false`|Whether to still slide for a bit after a movement button is released in noclip mode
`hacks-womstylehacks`|`false`|Whether to enable WoM client style hacks (e.g. ludicrous triple jump)
`hacks-fullblockstep`|`false`|Whether to automatically climb up 1.0 (default is only 0.5) tall blocks
`hacks-jumpvelocity`|`0.42`|Initial vertical velocity when you start a jump<br>Velocity must be between 0.0 and 52.0
`hacks-perm-msgs`|`true`|Whether to show a message in chat if you attempt to use a currently disabled hack

## General rendering options
|Name|Default|Description|
|--|--|--|
`gfx-mipmaps`|`false`|Whether to use mipmaps to reduce faraway texture noise
`fpslimit`|`LimitVSync`|Strategy used to limit FPS<br>Strategies: LimitVSync, Limit30FPS, Limit60FPS, Limit120FPS, Limit144FPS, LimitNone
`normal`|`normal`|Environmental effects render mode<br>Modes: normal, normalfast, legacy, legacyfast<br>- legacy improves appearance on some older GPUs<br>- fast disables clouds, fog and overhead sky

## Other rendering options
|Name|Default|Description|
|--|--|--|
`nostalgia-classicarm`|Classic mode|Whether to render your own arm in classic or modern minecraft style
`gui-blockinhand`|`true`|Whether to show block currently being held in bottom right corner
`namesmode`|`Hovered`|Entity nametag rendering mode<br>None, Hovered, All, AllHovered, AllUnscaled
`entityshadow`|`None`|Entity shadow rendering mode<br>None, SnapToBlock, Circle, CircleAll

### Texture pack options
|Name|Default|Description|
|--|--|--|
`defaulttexpack`|`default.zip`|Filename of default texture pack

### Window options
|Name|Default|Description|
|--|--|--|
`window-width`|`854`|Width of game window<br>Width must be between 0 and display width
`window-height`|`480`|Height of game window<br>Height must be between 0 and display height
`win-grab-cursor`|`false`|Whether to grab exclusive control over the cursor<br>**Only supported on Linux/BSD**

./Gui.c:        Gui.Chatlines       = Options_GetInt(OPT_CHATLINES, 0, 30, Gui.DefaultLines);
./Gui.c:        Gui.ClickableChat   = !Game_ClassicMode && Options_GetBool(OPT_CLICKABLE_CHAT,   !Input_TouchMode);
./Gui.c:        Gui.TabAutocomplete = !Game_ClassicMode && Options_GetBool(OPT_TAB_AUTOCOMPLETE, true);
./Gui.c:        Gui.ClassicTexture = Options_GetBool(OPT_CLASSIC_GUI, true)      || Game_ClassicMode;
./Gui.c:        Gui.ClassicTabList = Options_GetBool(OPT_CLASSIC_TABLIST, false) || Game_ClassicMode;
./Gui.c:        Gui.ClassicMenu    = Options_GetBool(OPT_CLASSIC_OPTIONS, false) || Game_ClassicMode;
./Gui.c:        Gui.ClassicChat    = Options_GetBool(OPT_CLASSIC_CHAT, false)    || Game_PureClassic;
./Gui.c:        Gui.ShowFPS        = Options_GetBool(OPT_SHOW_FPS, true);
./Gui.c:        Gui.RawInventoryScale = Options_GetFloat(OPT_INVENTORY_SCALE, 0.25f, 5.0f, 1.0f);
./Gui.c:        Gui.RawHotbarScale    = Options_GetFloat(OPT_HOTBAR_SCALE,    0.25f, 5.0f, 1.0f);
./Gui.c:        Gui.RawChatScale      = Options_GetFloat(OPT_CHAT_SCALE,      0.25f, 5.0f, 1.0f);
./Gui.c:        Gui.RawTouchScale     = Options_GetFloat(OPT_TOUCH_SCALE,     0.25f, 5.0f, 1.0f);
./Gui.c:        Gui._onscreenButtons = Options_GetInt(OPT_TOUCH_BUTTONS, 0, Int32_MaxValue,
./Input.c:              mapping = Options_GetEnum(name.buffer, KeyBind_Defaults[i], Input_Names, INPUT_COUNT);
