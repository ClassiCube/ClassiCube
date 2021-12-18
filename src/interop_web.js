// Copyright 2010 The Emscripten Authors. All rights reserved.
// Emscripten is available under two separate licenses, 
//  the MIT license and the University of Illinois/NCSA Open Source License. 
// Both these licenses can be found in the LICENSE file.

mergeInto(LibraryManager.library, {
  
  interop_SaveBlob: function(blob, name) {
    if (window.navigator.msSaveBlob) {
      window.navigator.msSaveBlob(blob, name); return;
    }
    var url  = window.URL.createObjectURL(blob);
    var elem = document.createElement('a');

    elem.href     = url;
    elem.download = name;
    elem.style.display = 'none';

    document.body.appendChild(elem);
    elem.click();
    document.body.removeChild(elem);
    window.URL.revokeObjectURL(url);
  },
  interop_InitModule: function() {
    // these are required for older versions of emscripten, but the compiler removes
    // this by default as no syscalls are used by the C platform code anymore
    window.ERRNO_CODES={EPERM:1,ENOENT:2,EIO:5,ENXIO:6,EBADF:9,EAGAIN:11,EWOULDBLOCK:11,ENOMEM:12,EEXIST:17,ENODEV:19,ENOTDIR:20,EISDIR:21,EINVAL:22,EBADFD:77,ENOTEMPTY:39};
  },
  interop_InitModule__deps: ['interop_SaveBlob'],
  interop_TakeScreenshot: function(path) {
    var name   = UTF8ToString(path);
    var canvas = Module['canvas'];
    if (canvas.toBlob) {
      canvas.toBlob(function(blob) { _interop_SaveBlob(blob, name); });
    } else if (canvas.msToBlob) {
      _interop_SaveBlob(canvas.msToBlob(), name);
    }
  },
  
  
//########################################################################################################################
//-----------------------------------------------------------Http---------------------------------------------------------
//########################################################################################################################
  interop_DownloadAsync: function(urlStr, method, reqID) {
    // onFinished = FUNC(data, len, status)
    // onProgress = FUNC(read, total)
    var url        = UTF8ToString(urlStr);
    var reqMethod  = method == 1 ? 'HEAD' : 'GET';  
    var onFinished = Module["_Http_OnFinishedAsync"];
    var onProgress = Module["_Http_OnUpdateProgress"];

    var xhr = new XMLHttpRequest();
    try {
      xhr.open(reqMethod, url);
    } catch (e) {
      // DOMException gets thrown when invalid URL provided. Test cases:
      //   http://%7https://www.example.com/test.zip
      //   http://example:app/test.zip
      console.log(e);
      return 1;
    }  
    xhr.responseType = 'arraybuffer';

    var getContentLength = function(e) {
      if (e.total) return e.total;

      try {
        var len = xhr.getResponseHeader('Content-Length');
        return parseInt(len, 10);
      } catch (ex) { return 0; }
    };
    
    xhr.onload = function(e) {
      var src  = new Uint8Array(xhr.response);
      var len  = src.byteLength;
      var data = _malloc(len);
      HEAPU8.set(src, data);
      onFinished(reqID, data, len || getContentLength(e), xhr.status);
    };
    xhr.onerror    = function(e) { onFinished(reqID, 0, 0, xhr.status);  };
    xhr.ontimeout  = function(e) { onFinished(reqID, 0, 0, xhr.status);  };
    xhr.onprogress = function(e) { onProgress(reqID, e.loaded, e.total); };

    try { xhr.send(); } catch (e) { onFinished(reqID, 0, 0, 0); }
    return 0;
  },
  interop_IsHttpsOnly : function() {
    // If this webpage is https://, browsers deny any http:// downloading
    return location.protocol === 'https:'; 
  },


//########################################################################################################################
//-----------------------------------------------------------Menu---------------------------------------------------------
//########################################################################################################################
  interop_DownloadMap: function(path, filename) {
    try {
      var name = UTF8ToString(path);
      var data = FS.readFile(name);
      var blob = new Blob([data], { type: 'application/octet-stream' });
      _interop_SaveBlob(blob, UTF8ToString(filename));
      FS.unlink(name);
      return 0;
    } catch (e) {
      if (!(e instanceof FS.ErrnoError)) abort(e);
      return -e.errno;
    }
  },
  interop_DownloadMap__deps: ['interop_SaveBlob'],
  interop_UploadTexPack: function(path) {
    // Move from temp into texpacks folder
    // TODO: This is pretty awful and should be rewritten
    var name = UTF8ToString(path);
    var data = FS.readFile(name);
    FS.writeFile('/texpacks/' + name.substring(1), data);
  },


//########################################################################################################################
//---------------------------------------------------------Platform-------------------------------------------------------
//########################################################################################################################
  interop_OpenTab: function(url) {
    window.open(UTF8ToString(url));
    return 0;
  },
  interop_Log: function(msg, len) {
    Module.print(UTF8ArrayToString(HEAPU8, msg, len));
  },
  interop_GetLocalTime: function(time) {
    var date = new Date();
    HEAP32[(time|0 +  0)>>2] = date.getFullYear();
    HEAP32[(time|0 +  4)>>2] = date.getMonth() + 1|0;
    HEAP32[(time|0 +  8)>>2] = date.getDate();
    HEAP32[(time|0 + 12)>>2] = date.getHours();
    HEAP32[(time|0 + 16)>>2] = date.getMinutes();
    HEAP32[(time|0 + 20)>>2] = date.getSeconds();
  },
  interop_DirectorySetWorking: function (raw) {
    var path = UTF8ToString(raw);
    try {
      FS.chdir(path);
      return 0;
    } catch (e) {
      if (typeof FS === 'undefined' || !(e instanceof FS.ErrnoError)) abort(e);
      return -e.errno;
    }
  },
  interop_DirectoryCreate: function(raw, mode) {
    var path = UTF8ToString(raw);
    try {
      FS.mkdir(path, mode, 0);
      _interop_SaveNode(path);
      return 0;
    } catch (e) {
      if (typeof FS === 'undefined' || !(e instanceof FS.ErrnoError)) abort(e);
      return -e.errno;
    }
  },
  interop_DirectoryCreate__deps: ['interop_SaveNode'],
  interop_DirectoryIter: function(raw) {
    var path = UTF8ToString(raw);
    try {
      var entries = FS.readdir(path);	  
      for (var i = 0; i < entries.length; i++) {
        ccall('Directory_IterCallback', 'void', ['string'], [entries[i]]);
      }
      return 0;
    } catch (e) {
      if (typeof FS === 'undefined' || !(e instanceof FS.ErrnoError)) abort(e);
      return -e.errno;
    }
  },
  interop_FileExists: function (raw) {
    var path = UTF8ToString(raw);
    try {
      var lookup = FS.lookupPath(path, { follow: true });
      if (!lookup.node) return false;
    } catch (e) {
      if (typeof FS === 'undefined' || !(e instanceof FS.ErrnoError)) abort(e);
      return false;
    }
    return true;
  },
  interop_FileCreate: function(raw, flags, mode) {
    var path = UTF8ToString(raw);
    try {
      var stream = FS.open(path, flags, mode);
      return stream.fd|0;
    } catch (e) {
      if (typeof FS === 'undefined' || !(e instanceof FS.ErrnoError)) abort(e);
      return -e.errno;
    }
  },
  interop_FileRead: function(fd, dst, count) {
    try {
      var stream = FS.getStream(fd);
      return FS.read(stream, HEAP8, dst, count)|0;
    } catch (e) {
      if (typeof FS === 'undefined' || !(e instanceof FS.ErrnoError)) abort(e);
      return -e.errno;
    }
  },
  interop_FileWrite: function(fd, src, count) {
    try {
      var stream = FS.getStream(fd);
      return FS.write(stream, HEAP8, src, count)|0;
    } catch (e) {
      if (typeof FS === 'undefined' || !(e instanceof FS.ErrnoError)) abort(e);
      return -e.errno;
    }
  },
  interop_FileSeek: function(fd, offset, whence) {
    try {
      var stream = FS.getStream(fd);
      return FS.llseek(stream, offset, whence)|0;
    } catch (e) {
      if (typeof FS === 'undefined' || !(e instanceof FS.ErrnoError)) abort(e);
      return -e.errno;
    }
  },
  interop_FileLength: function(fd) {
    try {
      var stream = FS.getStream(fd);
      var attrs  = stream.node.node_ops.getattr(stream.node);
      return attrs.size|0;
    } catch (e) {
      if (typeof FS === 'undefined' || !(e instanceof FS.ErrnoError)) abort(e);
      return -e.errno;
    }
  },
  interop_FileClose: function(fd) {
    try {
      var stream = FS.getStream(fd);
      FS.close(stream);
      // save writable files to IndexedDB (check for O_RDWR)
      if ((stream.flags & 3) == 2) _interop_SaveNode(stream.path);
      return 0;
    } catch (e) {
      if (typeof FS === 'undefined' || !(e instanceof FS.ErrnoError)) abort(e);
      return -e.errno;
    }
  },
  interop_FileClose__deps: ['interop_SaveNode'],
  
  
//########################################################################################################################
//--------------------------------------------------------Filesystem------------------------------------------------------
//########################################################################################################################
  interop_InitFilesystem: function(buffer) {
    if (!window.cc_idbErr) return;
    var msg = 'Error preloading IndexedDB:' + window.cc_idbErr + '\n\nPreviously saved settings/maps will be lost';
    ccall('Platform_LogError', 'void', ['string'], [msg]);
  },
  interop_LoadIndexedDB: function() {
    try {
      FS.lookupPath('/classicube');
      return;
      // FS.lookupPath throws exception if path doesn't exist
    } catch (e) { }
    
    addRunDependency('load-idb');
    FS.mkdir('/classicube');
    FS.mount(IDBFS, {}, '/classicube');
    FS.syncfs(true, function(err) { 
      if (err) window.cc_idbErr = err;
      removeRunDependency('load-idb');
    });
  },
  interop_InitFilesystem__deps: ['interop_SaveNode'],
  interop_SaveNode: function(path) {
    var callback = function(err) { 
      if (!err) return;
      console.log(err);
      ccall('Platform_LogError', 'void', ['string'], ['&cError saving ' + path]);
      ccall('Platform_LogError', 'void', ['string'], ['   &c' + err]);
    }; 
    
    var stat, node, entry;
    try {
      var lookup = FS.lookupPath(path);
      path = lookup.path;
      node = lookup.node;
      stat = node.node_ops.getattr(node);
    
      if (FS.isDir(stat.mode)) {
        entry = { timestamp: stat.mtime, mode: stat.mode };
      } else {
        // Performance consideration: storing a normal JavaScript array to a IndexedDB is much slower than storing a typed array.
        // Therefore always convert the file contents to a typed array first before writing the data to IndexedDB.
        node.contents = MEMFS.getFileDataAsTypedArray(node);
        entry = { timestamp: stat.mtime, mode: stat.mode, contents: node.contents };
      }
    } catch (err) {
      return callback(err);
    }
    
    IDBFS.getDB('/classicube', function(err, db) {
      if (err) return callback(err);
      var transaction, store;
      
      // can still throw errors here
      try {
        transaction = db.transaction([IDBFS.DB_STORE_NAME], 'readwrite');
        store = transaction.objectStore(IDBFS.DB_STORE_NAME);
      } catch (err) {
        return callback(err);
      }
      
      transaction.onerror = function(e) {
        callback(this.error);
        e.preventDefault();
      };
      
      var req = store.put(entry, path);
      req.onsuccess = function()  { callback(null); };
      req.onerror   = function(e) {
        callback(this.error);
        e.preventDefault();
      };
    });
  },


//########################################################################################################################
//---------------------------------------------------------Sockets--------------------------------------------------------
//########################################################################################################################
  interop_InitSockets: function() {
    window.SOCKETS = {
      EBADF:-8,EISCONN:-30,ENOTCONN:-53,EAGAIN:-6,EHOSTUNREACH:-23,EINPROGRESS:-26,EALREADY:-7,ECONNRESET:-15,EINVAL:-28,ECONNREFUSED:-14,
      sockets: [],
    };
  },
  interop_SocketCreate: function() {
    var sock = {
      error: null, // Used by interop_SocketGetError
      recv_queue: [],
      socket: null,
    };

    SOCKETS.sockets.push(sock);
    return (SOCKETS.sockets.length - 1) | 0;
  },
  interop_SocketConnect: function(sockFD, raw, port) {
    var addr = UTF8ToString(raw);
    var sock = SOCKETS.sockets[sockFD];
    if (!sock) return SOCKETS.EBADF;

    // already connecting or connected
    var ws = sock.socket;
    if (ws) {
      if (ws.readyState === ws.CONNECTING) return SOCKETS.EALREADY;
      return SOCKETS.EISCONN;
    }

    // create the actual websocket object and connect
    try {
      var parts = addr.split('/');
      var url = 'ws://' + parts[0] + ":" + port + "/" + parts.slice(1).join('/');
      ws = new WebSocket(url, 'ClassiCube');
      ws.binaryType = 'arraybuffer';
    } catch (e) {
      return SOCKETS.EHOSTUNREACH;
    }
    sock.socket = ws;

    ws.onopen  = function() {};
    ws.onclose = function() {};
    ws.onmessage = function(event) {
      var data = event.data;
      if (typeof data === 'string') {
        var encoder = new TextEncoder(); // should be utf-8
        data = encoder.encode(data); // make a typed array from the string
      } else {
        assert(data.byteLength !== undefined); // must receive an ArrayBuffer
        if (data.byteLength == 0) {
          // An empty ArrayBuffer will emit a pseudo disconnect event
          // as recv/recvmsg will return zero which indicates that a socket
          // has performed a shutdown although the connection has not been disconnected yet.
          return;
        } else {
          data = new Uint8Array(data); // make a typed array view on the array buffer
        }
      }
      sock.recv_queue.push(data);
    };
    ws.onerror = function(error) {
      // The WebSocket spec only allows a 'simple event' to be thrown on error,
      // so we only really know as much as ECONNREFUSED.
      sock.error = -SOCKETS.ECONNREFUSED; // Used by interop_SocketGetError
    };
    // always "fail" in non-blocking mode
    return SOCKETS.EINPROGRESS;
  },
  interop_SocketClose: function(sockFD) {
    var sock = SOCKETS.sockets[sockFD];
    if (!sock) return SOCKETS.EBADF;

    try {
      sock.socket.close();
    } catch (e) {
    }
    delete sock.socket;
    return 0;
  },
  interop_SocketSend: function(sockFD, src, length) {
    var sock = SOCKETS.sockets[sockFD];
    if (!sock) return SOCKETS.EBADF;

    var ws = sock.socket;
    if (!ws || ws.readyState === ws.CLOSING || ws.readyState === ws.CLOSED) {
      return SOCKETS.ENOTCONN;
    } else if (ws.readyState === ws.CONNECTING) {
      return SOCKETS.EAGAIN;
    }

    // var data = HEAP8.slice(src, src + length); unsupported in IE11
    var data = new Uint8Array(length);
    for (var i = 0; i < length; i++) {  
      data[i] = HEAP8[src + i];
    }

    try {
      ws.send(data);
      return length;
    } catch (e) {
      return SOCKETS.EINVAL;
    }
  },
  interop_SocketRecv: function(sockFD, dst, length) {
    var sock = SOCKETS.sockets[sockFD];
    if (!sock) return SOCKETS.EBADF;

    var packet = sock.recv_queue.shift();
    if (!packet) {
      var ws = sock.socket;

      if (!ws || ws.readyState == ws.CLOSING || ws.readyState == ws.CLOSED) {
        return SOCKETS.ENOTCONN;
      } else {
        // socket is in a valid state but truly has nothing available
        return SOCKETS.EAGAIN;
      }
    }

    // packet will be an ArrayBuffer if it's unadulterated, but if it's
    // requeued TCP data it'll be an ArrayBufferView
    var packetLength = packet.byteLength || packet.length;
    var packetOffset = packet.byteOffset || 0;
    var packetBuffer = packet.buffer || packet;
    var bytesRead = Math.min(length, packetLength);
    var msg = new Uint8Array(packetBuffer, packetOffset, bytesRead);

    // push back any unread data for TCP connections
    if (bytesRead < packetLength) {
      var bytesRemaining = packetLength - bytesRead;
      packet = new Uint8Array(packetBuffer, packetOffset + bytesRead, bytesRemaining);
      sock.recv_queue.unshift(packet);
    }

    HEAPU8.set(msg, dst);
    return msg.byteLength;
  },
  interop_SocketGetPending: function(sockFD) {
    var sock = SOCKETS.sockets[sockFD];
    if (!sock) return SOCKETS.EBADF;

    return sock.recv_queue.length;
  },
  interop_SocketGetError: function(sockFD) {
    var sock = SOCKETS.sockets[sockFD];
    if (!sock) return SOCKETS.EBADF;

    return sock.error || 0;
  },
  interop_SocketPoll: function(sockFD) {
    var sock = SOCKETS.sockets[sockFD];
    if (!sock) return SOCKETS.EBADF;

    var ws   = sock.socket;
    if (!ws) return 0;
    var mask = 0;

    if (sock.recv_queue.length || (ws.readyState === ws.CLOSING || ws.readyState === ws.CLOSED)) mask |= 1;
    if (ws.readyState === ws.OPEN) mask |= 2;
    return mask;
  },


//########################################################################################################################
//----------------------------------------------------------Window--------------------------------------------------------
//########################################################################################################################
  interop_CanvasWidth: function()  { return Module['canvas'].width;  },
  interop_CanvasHeight: function() { return Module['canvas'].height; },
  interop_ScreenWidth: function()  { return screen.width;  },
  interop_ScreenHeight: function() { return screen.height; },

  interop_IsAndroid: function() {
    return /Android/i.test(navigator.userAgent);
  },
  interop_IsIOS: function() {
    // iOS 13 on iPad doesn't identify itself as iPad by default anymore
    //  https://stackoverflow.com/questions/57765958/how-to-detect-ipad-and-ipad-os-version-in-ios-13-and-up
    return /iPhone|iPad|iPod/i.test(navigator.userAgent) || 
      (navigator.platform === 'MacIntel' && navigator.maxTouchPoints && navigator.maxTouchPoints > 2);
  },
  interop_InitContainer: function() {
    // Create wrapper div if necessary (so input textbox shows in fullscreen on android)
    var agent  = navigator.userAgent;  
    var canvas = Module['canvas'];
    window.cc_container = document.body;

    if (/Android/i.test(agent)) {
      var wrapper = document.createElement("div");
      wrapper.id  = 'canvas_wrapper';

      canvas.parentNode.insertBefore(wrapper, canvas);
      wrapper.appendChild(canvas);
      window.cc_container = wrapper;
    }
  },
  interop_GetContainerID: function() {
    // For chrome on android, need to make container div fullscreen instead
    return document.getElementById('canvas_wrapper') ? 1 : 0;
  },
  interop_ForceTouchPageLayout: function() {
    if (typeof(forceTouchLayout) === 'function') forceTouchLayout();
  },
  interop_SetPageTitle : function(title) {
    document.title = UTF8ToString(title);
  },
  interop_AddClipboardListeners: function() {
    // Copy text, but only if user isn't selecting something else on the webpage
    // (don't check window.clipboardData here, that's handled in interop_TrySetClipboardText instead)
    window.addEventListener('copy', 
    function(e) {
      if (window.getSelection && window.getSelection().toString()) return;
      ccall('Window_RequestClipboardText', 'void');
      if (!window.cc_copyText) return;

      if (e.clipboardData) {
        e.clipboardData.setData('text/plain', window.cc_copyText);
        e.preventDefault();
      }
      window.cc_copyText = null;
    });

    // Paste text (window.clipboardData is handled in interop_TryGetClipboardText instead)
    window.addEventListener('paste',
    function(e) {
      if (e.clipboardData) {
        var contents = e.clipboardData.getData('text/plain');
        ccall('Window_GotClipboardText', 'void', ['string'], [contents]);
      }
    });
  },
  interop_TryGetClipboardText: function() {
    // For IE11, use window.clipboardData to get the clipboard
    if (window.clipboardData) {
      var contents = window.clipboardData.getData('Text');
      ccall('Window_StoreClipboardText', 'void', ['string'], [contents]);
    } 
  },
  interop_TrySetClipboardText: function(text) {
    // For IE11, use window.clipboardData to set the clipboard */
    // For other browsers, instead use the window.copy events */
    if (window.clipboardData) {
      if (window.getSelection && window.getSelection().toString()) return;
      window.clipboardData.setData('Text', UTF8ToString(text));
    } else {
      window.cc_copyText = UTF8ToString(text);
    }
  },
  interop_EnterFullscreen: function() {
    // emscripten sets css size to screen's base width/height,
    //  except that becomes wrong when device rotates.
    // Better to just set CSS width/height to always be 100%
    var canvas = Module['canvas'];
    canvas.style.width  = '100%';
    canvas.style.height = '100%';
    
    // By default, pressing Escape will immediately exit fullscreen - which is
    //   quite annoying given that it is also the Menu key. Some browsers allow
    //   'locking' the Escape key, so that you have to hold down Escape to exit.
    // NOTE: This ONLY works when the webpage is a https:// one
    try { navigator.keyboard.lock(["Escape"]); } catch (ex) { }
  },

  // Adjust from document coordinates to element coordinates
  interop_AdjustXY: function(x, y) {
    var canvasRect = Module['canvas'].getBoundingClientRect();
    HEAP32[x >> 2] = HEAP32[x >> 2] - canvasRect.left;
    HEAP32[y >> 2] = HEAP32[y >> 2] - canvasRect.top;
  },
  interop_RequestCanvasResize: function() {
    if (typeof(resizeGameCanvas) === 'function') resizeGameCanvas();
  },
  interop_SetCursorVisible: function(visible) {
    Module['canvas'].style['cursor'] = visible ? 'default' : 'none';
  },
  interop_ShowDialog: function(title, msg) {
    alert(UTF8ToString(title) + "\n\n" + UTF8ToString(msg));
  },
  interop_OpenKeyboard: function(text, type, placeholder) {
    var elem  = window.cc_inputElem;
    var shown = true;
    if (!elem) {
      if (type == 1) { // KEYBOARD_TYPE_NUMBER
        elem = document.createElement('input');
        elem.setAttribute('type', 'text')
        elem.setAttribute('inputmode', 'decimal');
      } else if (type == 3) { // KEYBOARD_TYPE_INTEGER
        elem = document.createElement('input');
        elem.setAttribute('type', 'text')
        elem.setAttribute('inputmode', 'numeric');
        // Fix for older iOS safari where inputmode is unsupported
        //  https://news.ycombinator.com/item?id=22433654
        //  https://technology.blog.gov.uk/2020/02/24/why-the-gov-uk-design-system-team-changed-the-input-type-for-numbers/
        elem.setAttribute('pattern', '[0-9]*');
      } else {
        elem = document.createElement('textarea');
      }
      shown = false;
    }

    elem.setAttribute('style', 'position:absolute; left:0; bottom:0; margin: 0px; width: 100%');
    elem.setAttribute('placeholder', UTF8ToString(placeholder));
    elem.value = UTF8ToString(text);
  
    if (!shown) {
      // stop event propagation, because we don't want the game trying to handle these events
      elem.addEventListener('touchstart', function(ev) { ev.stopPropagation(); }, false);
      elem.addEventListener('touchmove',  function(ev) { ev.stopPropagation(); }, false);
      elem.addEventListener('mousedown',  function(ev) { ev.stopPropagation(); }, false);
      elem.addEventListener('mousemove',  function(ev) { ev.stopPropagation(); }, false);

      elem.addEventListener('input', 
        function(ev) {
          ccall('Window_OnTextChanged', 'void', ['string'], [ev.target.value]);
        }, false);
      window.cc_inputElem = elem;

      window.cc_divElem = document.createElement('div');
      window.cc_divElem.setAttribute('style', 'position:absolute; left:0; top:0; width:100%; height:100%; background-color: black; opacity:0.4; resize:none; pointer-events:none;');

      window.cc_container.appendChild(window.cc_divElem);
      window.cc_container.appendChild(elem);
    }

    // force on-screen keyboard to be shown
    elem.focus();
    elem.click();
  },
  interop_SetKeyboardText: function(text) {
    if (!window.cc_inputElem) return;
    var str = UTF8ToString(text);
    var cur = window.cc_inputElem.value;
    
    // when pressing 'Go' on the on-screen keyboard, some web browsers add \n to value
    if (cur.length && cur[cur.length - 1] == '\n') { cur = cur.substring(0, cur.length - 1); }
    if (str != cur) window.cc_inputElem.value = str;
  },
  interop_CloseKeyboard: function() {
    if (!window.cc_inputElem) return;
    window.cc_container.removeChild(window.cc_divElem);
    window.cc_container.removeChild(window.cc_inputElem);
    window.cc_divElem   = null;
    window.cc_inputElem = null;
  },
  interop_OpenFileDialog: function(filter) {
    var elem = window.cc_uploadElem;
    if (!elem) {
      elem = document.createElement('input');
      elem.setAttribute('type', 'file');
      elem.setAttribute('style', 'display: none');
      elem.accept = UTF8ToString(filter);

      elem.addEventListener('change', 
        function(ev) {
          var files = ev.target.files;
          for (var i = 0; i < files.length; i++) {
            var reader = new FileReader();
            var name   = files[i].name;

            reader.onload = function(e) { 
              var data = new Uint8Array(e.target.result);
              FS.createDataFile('/', name, data, true, true, true);
              ccall('Window_OnFileUploaded', 'void', ['string'], ['/' + name]);
              FS.unlink('/' + name);
            };
            reader.readAsArrayBuffer(files[i]);
          }
          window.cc_container.removeChild(window.cc_uploadElem);
          window.cc_uploadElem = null;
        }, false);
      window.cc_uploadElem = elem;
      window.cc_container.appendChild(elem);
    }
    elem.click();
  },


//########################################################################################################################
//--------------------------------------------------------GLContext-------------------------------------------------------
//#########################################################################################################################
  interop_GetGpuRenderer : function(buffer, len) { 
    var dbg = GLctx.getExtension('WEBGL_debug_renderer_info');
    var str = dbg ? GLctx.getParameter(dbg.UNMASKED_RENDERER_WEBGL) : "";
    stringToUTF8(str, buffer, len);
  },
  

//########################################################################################################################
//---------------------------------------------------------Sockets--------------------------------------------------------
//########################################################################################################################
  interop_AudioLog: function(err) {
    console.log(err);
    window.AUDIO.errors.push(''+err);
    return window.AUDIO.errors.length|0;
  },
  interop_InitAudio: function() {
    window.AUDIO = window.AUDIO || {
      context: null,
      sources: [],
      buffers: {},
      errors: [],
      seen: {},
    };
    if (window.AUDIO.context) return 0;

    try {
      if (window.AudioContext) {
        AUDIO.context = new window.AudioContext();
      } else {
        AUDIO.context = new window.webkitAudioContext();
      }
      return 0;
    } catch (err) {
      return _interop_AudioLog(err)
    }
  },
  interop_InitAudio__deps: ['interop_AudioLog'],
  interop_AudioCreate: function() {
    var src = {
      source: null,
      gain: null,
      playing: false,
    };
    AUDIO.sources.push(src);
    return AUDIO.sources.length|0; 
    // NOTE: 0 is used by Audio.c for "no source"
  },
  interop_AudioClose: function(ctxID) {
    var src = AUDIO.sources[ctxID - 1|0];
    if (src.source) src.source.stop();
    AUDIO.sources[ctxID - 1|0] = null;
  },
  interop_AudioPoll: function(ctxID, inUse) {
    var src = AUDIO.sources[ctxID - 1|0];
    HEAP32[inUse >> 2] = src.playing; // only 1 buffer
    return 0;
  },
  interop_AudioPlay: function(ctxID, sndID, volume, rate) {
    var src  = AUDIO.sources[ctxID - 1|0];
    var name = UTF8ToString(sndID);
    
    // do we need to download this file?
    if (!AUDIO.seen.hasOwnProperty(name)) {
      AUDIO.seen[name] = true;
      _interop_AudioDownload(name);
      return 0;
    }

    // still downloading or failed to download this file
    var buffer = AUDIO.buffers[name];
    if (!buffer) return 0;
    
    try {
      if (!src.gain) src.gain = AUDIO.context.createGain();
      
      // AudioBufferSourceNode only allows the buffer property
      //  to be assigned *ONCE* (throws InvalidStateError next time)
      // MDN says that these nodes are very inexpensive to create though
      //  https://developer.mozilla.org/en-US/docs/Web/API/AudioBufferSourceNode
      src.source = AUDIO.context.createBufferSource();
      src.source.buffer   = buffer;
      src.gain.gain.value = volume / 100;
      src.source.playbackRate.value = rate / 100;
      
      // source -> gain -> output
      src.source.connect(src.gain);
      src.gain.connect(AUDIO.context.destination);
      src.source.start();
      return 0;
    } catch (err) {
      return _interop_AudioLog(err)
    }
  },
  interop_AudioPlay__deps: ['interop_AudioDownload'],
  interop_AudioDownload: function(name) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/static/sounds/' + name + '.wav', true);   
    xhr.responseType = 'arraybuffer';
    
    xhr.onload = function() {
      var data = xhr.response;
      AUDIO.context.decodeAudioData(data, function(buffer) {
          AUDIO.buffers[name] = buffer;
        });
    };
    xhr.send();
  },
  interop_AudioDescribe: function(errCode, buffer, bufferLen) {
    if (errCode > AUDIO.errors.length) return 0;
    
    var str = AUDIO.errors[errCode - 1];
    return stringToUTF8(str, buffer, bufferLen);
  },
  
  
//########################################################################################################################
//-----------------------------------------------------------Font---------------------------------------------------------
//########################################################################################################################
  interop_SetFont: function(fontStr, size, flags) {
    if (!window.FONT_CANVAS) {
      window.FONT_CANVAS  = document.createElement('canvas');
      window.FONT_CONTEXT = window.FONT_CANVAS.getContext('2d');
    }
    
    var prefix = '';
    if (flags & 1) prefix += 'Bold ';
    
    var font = UTF8ToString(fontStr);
    var ctx  = window.FONT_CONTEXT;
    ctx.font = prefix + size + 'px ' + font;
    ctx.textAlign    = 'left';
    ctx.textBaseline = 'top';
    return ctx;
  },
  interop_TextWidth: function(textStr, textLen) {
    var text = UTF8ArrayToString(HEAPU8, textStr, textLen);
    var ctx  = window.FONT_CONTEXT;
    var data = ctx.measureText(text);
    return Math.ceil(data.width)|0;
  },
  interop_TextDraw: function(textStr, textLen, bmp, dstX, dstY, shadow) {
    var text = UTF8ArrayToString(HEAPU8, textStr, textLen);
    var ctx  = window.FONT_CONTEXT;

    // resize canvas if necessary so test fits
    var data = ctx.measureText(text);
    var text_width = Math.ceil(data.width)|0;
    if (text_width > ctx.canvas.width)
      ctx.canvas.width = text_width;
    
    var text_offset = 0.0;
    ctx.fillStyle   = "#ffffff";
    if (shadow) {
      text_offset   = 1.3;
      ctx.fillStyle = "#7f7f7f";
    }

    ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
    ctx.fillText(text, text_offset, text_offset);
    
    bmp  = bmp|0;
    dstX = dstX|0;
    dstY = dstY|0;
    
    var dst_pixels = HEAP32[(bmp + 0|0)>>2] + (dstX << 2);
    var dst_width  = HEAP32[(bmp + 4|0)>>2];
    var dst_height = HEAP32[(bmp + 8|0)>>2];
    
    // TODO not all of it
    var src = ctx.getImageData(0, 0, ctx.canvas.width, ctx.canvas.height);
    var src_pixels = src.data;
    var src_width  = src.width|0;
    var src_height = src.height|0;

    var img_width  = Math.min(src_width,  dst_width);
    var img_height = Math.min(src_height, dst_height);

    for (var y = 0; y < img_height; y++)
    {
      var yy = y + dstY;
      if (yy < 0 || yy >= dst_height) continue;
      
      var src_row =              (y *(src_width << 2))|0;
      var dst_row = dst_pixels + (yy*(dst_width << 2))|0;
      
      for (var x = 0; x < img_width; x++)
      {
         var xx = x + dstX;
         if (xx < 0 || xx >= dst_width) continue;

         HEAPU8[dst_row + (x<<2)+0] = src_pixels[src_row + (x<<2)+0];
         HEAPU8[dst_row + (x<<2)+1] = src_pixels[src_row + (x<<2)+1];
         HEAPU8[dst_row + (x<<2)+2] = src_pixels[src_row + (x<<2)+2];
         HEAPU8[dst_row + (x<<2)+3] = src_pixels[src_row + (x<<2)+3];
      }
    }
  },
});