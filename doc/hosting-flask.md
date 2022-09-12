The page provides a complete example for how to integrate the webclient into a simple website

The example website will be structured like so:

```
websrv.py
templates/play.html
static/classisphere.js
static/default.zip
static/style.css
static/jquery.js
```

## Content
#### websrv.py
```Python
from flask import Flask, render_template, request
app = Flask(__name__)

@app.route("/")
def index():
    return '<html><h1>Welcome!</h1>Click <a href="/play">here</a> to play</h1></html>'

@app.route("/play")
@app.route("/play/")
def play():
    user = request.args.get('user') or 'Singleplayer'
    ver  = request.args.get('mppass') or ''
    addr = request.args.get('ip')
    port = request.args.get('port') or '25565'

    if addr:
        args = "['%s', '%s', '%s', '%s']" % (user, ver, addr, port)
    else:
        args = "['%s']" % user
    return render_template('play.html', game_args=args)

if __name__ == "__main__":
    app.run()
```

#### templates/play.html
```HTML
{% set mobile_mode = request.user_agent.platform in ('android', 'iphone', 'ipad') %}
<html>
  <head>
    <meta name="viewport" content="width=device-width">
    <link href="/static/style.css" rel="stylesheet">
    <script src="/static/jquery.js"></script>
  </head>
  <body>
{% if mobile_mode %}
    <style>
      #body {min-height: 0px;}
      .sec {padding: 0px;}
      .row {padding: 0px;}
    </style>
{% else %}
    <div id="header">
      <div class="row">
        <a href="/"><h1 class="columns">Home</h1></a>
        <a href="/play"><h1 class="columns">Play</h1></a>
      </div>
    </div>
{% endif %}
<div id="body">
<div class="sec">
  <div class="row">
    <canvas id="canvas" style="display:block; box-sizing:border-box; border-width:0px; padding:0; margin:0 auto; background-color: black; width:100%; height:auto;"
            oncontextmenu="event.preventDefault()" tabindex=-1 width="1000" height="562"></canvas>
    <span id="logmsg" style="font-size:18px;color:#F67;"></span>
  </div>
</div>
<script type='text/javascript'>
  function resizeGameCanvas() {
    var cc_canv = $('canvas#canvas');
    var dpi = window.devicePixelRatio;
    var aspect_ratio = 16/9;

    var viewport_w = cc_canv.parent().width();
    var viewport_h = viewport_w / aspect_ratio;

    var canv_w = Math.round(viewport_w);
    var canv_h = Math.round(viewport_h);

    if (canv_h % 2) { canv_h = canv_h - 1; }
    if (canv_w % 2) { canv_w = canv_w - 1; }

{% if mobile_mode %}
    var screen_h = Math.min(window.innerHeight, window.outerHeight || window.innerHeight);
    canv_h = screen_h;
{% endif %}
     cc_canv[0].width  = canv_w * dpi;
     cc_canv[0].height = canv_h * dpi;
  }

  var Module = {
    preRun: [ resizeGameCanvas ],
    postRun: [],
    arguments: {{game_args|safe}},
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
        </div>
    </body>
</html>
```

#### static/classisphere.js
Download `cs.classicube.net/c_client/latest/ClassiCube.js` for this

#### static/default.zip
Download `classicube.net/static/default.zip` for this

#### static/style.css
```CSS
body { margin: 0; }

.row {
    margin-left: auto;
    margin-right: auto;
    max-width: 62.5em;
}

a { text-decoration: none; }

.columns { display: inline-block; }

.sec {
    background:#f1ecfa;
    padding:10px 0 5px;
}

#header { background-color:#5870b0; }

#header h1 {
    color:#fff;
    margin:0px 10px 0px 10px;
    width: 200px;
}
```

#### static/jquery.js
Download some version of jQuery for this. Version 2.1.1 is known to work.

## Notes

* If you don't want the game to resize to fit different resolutions, remove the `resizeGameCanvas` code.

* mobile_mode is used to deliver a minified page for mobile/tablet devices

## Results

After all this setup, you need to install the flask package for python.

Then in command prompt/terminal enter: `python websrv.py`

Then navigate to `http://127.0.0.1:5000/play` in your web browser. If all goes well you should see the web client start in singleplayer.

To start in multiplayer instead, navigate to `http://127.0.0.1:5000/play?user=test&ip=127.0.0.1&port=25565`
