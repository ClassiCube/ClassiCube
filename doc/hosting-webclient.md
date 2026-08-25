Hosting your own version of the ClassiCube webclient is relatively straightforward.

### Example setup

At minimum, a deployment needs 3 things:
1) A web page to initialise the game .js and display the game
2) The game .js file
3) The default texture pack

When building from source, `make web dist` provides all three. You can also rehost the prebuilt JS client from ClassiCube.net. For example, let's assume your website is setup like this:
* `example.com/play.html`
* `example.com/static/classisphere.js`
* `example.com/static/default.zip`

For simplicity,
1) Download `cs.classicube.net/client/latest/ClassiCube.js`, then upload it to `static/classisphere.js` on the webserver
2) Download `classicube.net/static/default.zip`, then upload it to `static/default.zip` on the webserver

The play.html page is the trickiest part, because how to implement this is website-specific. (depends on how the website is styled, what webserver is used, what programming language is used to generate the html, etc)

#### Changing where the game downloads the texture pack from

There should be this piece of code somewhere in the .JS file: `function _interop_AsyncDownloadTexturePack(rawPath) {`

A bit below that, there should be `var url = '/static/default.zip';` - change that to the desired URL.

#### Embedding the game in play.html

The following HTML code is required to be somewhere in the webpage:
```HTML
<!-- the canvas *must not* have any border or padding, or mouse coords will be wrong -->
<canvas id="canvas" style="display:block; border:0; padding:0; background-color: black;" 
		oncontextmenu="event.preventDefault()" tabindex=-1></canvas>
<span id="logmsg"></span>

<script type='text/javascript'>
  var Module = {
    preRun: [],
    postRun: [],
    arguments: [ {username}, {mppass}, {server ip}, {server port} ],
    print: function(text) {
      if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
      console.log(text);
    },
    printErr: function(text) {
      if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
      console.error(text);
    },
    canvas: (function() { return document.getElementById('canvas'); })(),
    setStatus: function(text) {
      console.log(text);
      document.getElementById('logmsg').innerHTML = text;
    },
    totalDependencies: 0,
    monitorRunDependencies: function(left) {
      this.totalDependencies = Math.max(this.totalDependencies, left);
      Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
    }
  };
  Module.setStatus('Downloading...');
  window.onerror = function(msg) {
    // TODO: do not warn on ok events like simulating an infinite loop or exitStatus
    Module.setStatus('Exception thrown, see JavaScript console (' + msg + ')');
    Module.setStatus = function(text) {
      if (text) Module.printErr('[post-exception status] ' + text);
    };
  };
</script>
<script async type="text/javascript" src="/static/classisphere.js"></script>
```

The links below show how to integrate the webclient into a simple website
* [Flask (python webserver)](hosting-flask.md)

### Building the webclient from source

Run `make web dist` to package the compiled webclient into a `build/web/dist` folder, which can then be uploaded to a website.

- By default the game is compiled to WebAssembly (`ClassiCube.js` and `ClassiCube.wasm`). Add `RELEASE=1` for optimised output.
- Add `LEGACY=1` to produce a build equivalent to the webclient deployed to ClassiCube.net, a single optimised `ClassiCube.js` file (either asm.js or wasm2js depending on your emscripten version) that's slower but compatible with older browsers.
- Make sure to run `make web clean` first when rebuilding with different flags/options.

The folder contains a basic singleplayer webpage (`index.html`), the compiled game, and the texture pack (if `static/default.zip` exists). The game loads the texture pack from `/static/default.zip` at the website root, so upload the folder contents to the root (or see above for changing the url).

#### Testing the local webclient build

After compiling the webclient do `make web run`. This serves the compiled webclient on a local web server using emscripten's `emrun`, and then opens it in your browser.

The webclient downloads the default texture pack from `/static/default.zip`, so you need to provide that file too. For example:
```
mkdir -p static
curl -o static/default.zip https://classicube.net/static/default.zip
```

The generated ClassiCube.html page starts the game in singleplayer with the default username. To test multiplayer, you will need a webpage that provides server connection arguments.

### Authentication

The webclient has no login system built-in. The hosting webpage supplies the player's identity through `arguments`. To add authentication, sign the user in with your website's own account system, then generate the embed HTML with `arguments` filled in.

The supported `arguments` forms are:
- No arguments - opens singleplayer
- 1 argument (username) - opens singleplayer as that player
- 1 argument in the form `mc://[ip]:[port]/[username]/[mppass]` - connects to that server
- 4 arguments (username, mppass, ip, port) - connects to that server (port is usually `'25565'`)

If the server verifies names, the webpage must also calculate the correct [mppass](https://wiki.vg/Classic_Protocol#User_Authentication) for the user. Otherwise pass `''` for mppass.

### Custom skin server

Skins are downloaded from `http://cdn.classicube.net/skin/[player name].png` by default.
Set the `http-skinserver` option to download from `[skin server]/[player name].png` instead.
Multiplayer servers can also provide absolute URLs for player skins.

NOTE: In a browser, skins only download successfully when the skin server:
- sends CORS headers (e.g. `Access-Control-Allow-Origin: *`)
- supports https:// when the webpage is https://

### Page hooks

The game js calls these functions on the webpage, when they are defined:
- `forceTouchLayout()` - called on startup when the browser identifies as an Android or iOS device, so the webpage can switch to a mobile layout
- `resizeGameCanvas()` - called when the window is resized or fullscreen is exited, so the webpage can adjust the canvas size

### Storage

Options and singleplayer maps are saved to the browser's IndexedDB storage, separately per website. Clearing the website's site data erases them.

Players can also import their own map files into singleplayer through the Load level screen's Upload button, and export them as `.cw` downloads through the Save level screen.

### iOS / Android support

The webclient is compatible with Android / iOS devices and will show a touch based UI to these devices.

However, due to the limited screen size available on such devices, you should consider serving a webpage consisting of just the `<canvas>` to these devices - no header, footer or anything else.

Additionally, you will likely want to ensure zooming is disabled, viewport width is same as the device's width, and that 'add to device homescreen' is fully supported. You can accomplish that by adding these three HTML tags to the page:
```HTML
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="mobile-web-app-capable" content="yes">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0">
```