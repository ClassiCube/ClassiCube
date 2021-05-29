// Copyright 2010 The Emscripten Authors. All rights reserved.
// Emscripten is available under two separate licenses, 
//  the MIT license and the University of Illinois/NCSA Open Source License. 
// Both these licenses can be found in the LICENSE file.

mergeInto(LibraryManager.library, {
  
  interop_InitModule: function() {
    Module.saveBlob = function(blob, name) {
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
    }
  },
  interop_TakeScreenshot: function(path) {
    var name   = UTF8ToString(path);
    var canvas = Module['canvas'];
    if (canvas.toBlob) {
      canvas.toBlob(function(blob) { Module.saveBlob(blob, name); });
    } else if (canvas.msToBlob) {
      Module.saveBlob(canvas.msToBlob(), name);
    }
  },
  
  
//########################################################################################################################
//-----------------------------------------------------------Http---------------------------------------------------------
//########################################################################################################################
  interop_DownloadAsync: function(urlStr, method) {
    // onFinished = FUNC(data, len, status)
    // onProgress = FUNC(read, total)
    var url        = UTF8ToString(urlStr);
    var reqMethod  = method == 1 ? 'HEAD' : 'GET';  
    var onFinished = Module["_Http_OnFinishedAsync"];
    var onProgress = Module["_Http_OnUpdateProgress"];

    var xhr = new XMLHttpRequest();
    xhr.open(reqMethod, url);
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
      onFinished(data, len || getContentLength(e), xhr.status);
    };
    xhr.onerror    = function(e) { onFinished(0, 0, xhr.status);  };
    xhr.ontimeout  = function(e) { onFinished(0, 0, xhr.status);  };
    xhr.onprogress = function(e) { onProgress(e.loaded, e.total); };

    try { xhr.send(); } catch (e) { onFinished(0, 0, 0); }
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
      Module.saveBlob(blob, UTF8ToString(filename));
      FS.unlink(name);
      return 0;
    } catch (e) {
      if (!(e instanceof FS.ErrnoError)) abort(e);
      return -e.errno;
    }
  },
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
  interop_GetIndexedDBError: function(buffer) {
    if (window.cc_idbErr) stringToUTF8(window.cc_idbErr, buffer, 64);
  },
  interop_SyncFS: function() {
    FS.syncfs(false, function(err) { 
      if (!err) return;
      console.log(err);
      ccall('Platform_LogError', 'void', ['string'], ['&cError saving files to IndexedDB:']);
      ccall('Platform_LogError', 'void', ['string'], ['   &c' + err]);
    }); 
  },
  interop_OpenTab: function(url) {
    window.open(UTF8ToString(url));
    return 0;
  },
  interop_Log: function(msg, len) {
    Module.print(UTF8ArrayToString(HEAPU8, msg, len));
  },
  interop_InitSockets: function() {
    window.SOCKETS = {
      EBADF:-8,EISCONN:-30,ENOTCONN:-53,EAGAIN:-6,EHOSTUNREACH:-23,EINPROGRESS:-26,EALREADY:-7,ECONNRESET:-15,EINVAL:-28,ECONNREFUSED:-14,
      sockets: [],
      
      createSocket:function() {
        var sock = {
          error: null, // Used in getsockopt for SOL_SOCKET/SO_ERROR test
          recv_queue: [],
          socket: null,
        };  
        SOCKETS.sockets.push(sock);
      
        return (SOCKETS.sockets.length - 1) | 0;
      },
      connect:function(fd, addr, port) {
        var sock = SOCKETS.sockets[fd];
        if (!sock) return SOCKETS.EBADF;
      
        // early out if we're already connected / in the middle of connecting
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
          sock.error = -SOCKETS.ECONNREFUSED; // Used in getsockopt for SOL_SOCKET/SO_ERROR test.
        };
        // always "fail" in non-blocking mode
        return SOCKETS.EINPROGRESS;
      },
      poll:function(fd) {
        var sock = SOCKETS.sockets[fd];
        if (!sock) return SOCKETS.EBADF;
            
        var ws   = sock.socket;
        if (!ws) return 0;
        var mask = 0;

        if (sock.recv_queue.length || (ws.readyState === ws.CLOSING || ws.readyState === ws.CLOSED)) mask |= 1;
        if (ws.readyState === ws.OPEN) mask |= 2;
        return mask;
      },
      getPending:function(fd) {
        var sock = SOCKETS.sockets[fd];
        if (!sock) return SOCKETS.EBADF;

        return sock.recv_queue.length;
      },
      getError:function(fd) {
        var sock = SOCKETS.sockets[fd];
        if (!sock) return SOCKETS.EBADF;

        return sock.error || 0;
      },
      close:function(fd) {
        var sock = SOCKETS.sockets[fd];
        if (!sock) return SOCKETS.EBADF;
      
        try {
          sock.socket.close();
        } catch (e) {
        }
        delete sock.socket;
        return 0;
      },
      send:function(fd, src, length) {
        var sock = SOCKETS.sockets[fd];
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
      recv:function(fd, dst, length) {
        var sock = SOCKETS.sockets[fd];
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
      }
    };
  },
  interop_SocketCreate: function() {
    return SOCKETS.createSocket();
  },
  interop_SocketConnect: function(sock, addr, port) {
    var str = UTF8ToString(addr);
    return SOCKETS.connect(sock, str, port);
  },
  interop_SocketClose: function(sock) {
    return SOCKETS.close(sock);
  },
  interop_SocketSend: function(sock, data, len) {
    return SOCKETS.send(sock, data, len);
  },
  interop_SocketRecv: function(sock, data, len) {
    return SOCKETS.recv(sock, data, len);
  },
  interop_SocketGetPending: function(sock) {
    return SOCKETS.getPending(sock);
  },
  interop_SocketGetError: function(sock) {
    return SOCKETS.getError(sock);
  },
  interop_SocketPoll: function(sock) {
    return SOCKETS.poll(sock);
  },


//########################################################################################################################
//----------------------------------------------------------Window--------------------------------------------------------
//########################################################################################################################
  interop_CanvasWidth: function()  { return Module['canvas'].width  },
  interop_CanvasHeight: function() { return Module['canvas'].height },
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
    var elem = window.cc_inputElem;
    if (!elem) {
      if (type == 1) {
        elem = document.createElement('input');
        elem.setAttribute('inputmode', 'decimal');
      } else {
        elem = document.createElement('textarea');
      }
      elem.setAttribute('style', 'position:absolute; left:0; bottom:0; margin: 0px; width: 100%');
      elem.setAttribute('placeholder', UTF8ToString(placeholder));
      elem.value = UTF8ToString(text);

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
    elem.focus();
    elem.click();
  },
  // NOTE: When pressing 'Go' on the on-screen keyboard, some web browsers add \n to value
  interop_SetKeyboardText: function(text) {
    if (!window.cc_inputElem) return;
    var str = UTF8ToString(text);
    var cur = window.cc_inputElem.value;
    
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
  }
});