So you want to host your own version of the webclient? It's pretty easy to do, you need 3 files:
1) A web page to initialise the .js and display the game
2) The game .js file
3) The default texture pack

### Example setup

For example, let's assume our site is setup like this:
* `example.com/play.html`
* `example.com/static/classisphere.js`
* `example.com/static/default.zip`

For simplicitly,
1) Download `cs.classicube.net/c_client/latest/ClassiCube.js`, then upload it to `static/classisphere.js` on the webserver
2) Download `classicube.net/static/default.zip`, then upload it to `static/default.zip` on the webserver

The play.html page is the trickiest part, because how to implement this is website-specific. (depends on how your website is styled, what webserver you use, what programming language is used to generate the html, etc)

#### Embedding the game in play.html

You are required to have this HTML code somewhere in the page:
```HTML
<style>
/* the canvas *must not* have any border or padding, or mouse coords will be wrong */
#GameCanvas { display:block; border:0; padding:0; background-color: black; }
</style>
<canvas id="GameCanvas" oncontextmenu="event.preventDefault()" tabindex=-1></canvas>
<span id="logmsg"></span>

<script type='text/javascript'>
  // need to load IndexedDB before running the game
  function preloadIndexedDB() {
    addRunDependency('load-idb');
    FS.mkdir('/classicube');
    FS.mount(IDBFS, {}, '/classicube');
    FS.syncfs(true, function(err) {
        if (err) window.cc_idbErr = err;
        removeRunDependency('load-idb');
    })
  }
  
  var Module = {
    preRun: [ preloadIndexedDB ],
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
    canvas: (function() { return document.getElementById('GameCanvas'); })(),
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
  window.onerror = function(event) {
    // TODO: do not warn on ok events like simulating an infinite loop or exitStatus
    Module.setStatus('Exception thrown, see JavaScript console');
    Module.setStatus = function(text) {
      if (text) Module.printErr('[post-exception status] ' + text);
    };
  };
</script>
<script async type="text/javascript" src="/static/classisphere.js"></script>
```
**To start in singleplayer instead, change `arguments` to just `arguments: [ {username} ],`**

##### Variables
* {username} - the player's username
* {mppass} - if server verifies names, [mppass](https://wiki.vg/Classic_Protocol#User_Authentication). Otherwise leave as `''`.
* {server ip} - the IP address of the server to connect to
* {server port} - the port on the server to connect on (usually `'25565'`)

### Complete example

The links below show implementing a simple website that hosts the web client
* [Flask (python webserver)](hosting-flask.md)
