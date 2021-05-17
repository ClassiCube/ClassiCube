#include "Core.h"

#ifdef CC_BUILD_WEB
#include <emscripten/emscripten.h>

void interop_InitModule(void) {
	EM_ASM({
		Module['websocket']['subprotocol'] = 'ClassiCube';
		
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
	});
}
	

/*########################################################################################################################*
*-----------------------------------------------------------Game----------------------------------------------------------*
*#########################################################################################################################*/
void interop_TakeScreenshot(const char* path) {
	EM_ASM_({
		var name   = UTF8ToString($0);
		var canvas = Module['canvas'];
		if (canvas.toBlob) {
			canvas.toBlob(function(blob) { Module.saveBlob(blob, name); });
		} else if (canvas.msToBlob) {
			Module.saveBlob(canvas.msToBlob(), name);
		}
	}, path);
}


/*########################################################################################################################*
*-----------------------------------------------------------Http----------------------------------------------------------*
*#########################################################################################################################*/
void interop_DownloadAsync(const char* urlStr, int method) {
	/* onFinished = FUNC(data, len, status) */
	/* onProgress = FUNC(read, total) */
	EM_ASM_({
		var url        = UTF8ToString($0);
		var reqMethod  = $1 == 1 ? 'HEAD' : 'GET';	
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
	}, urlStr, method);
}

int interop_IsHttpsOnly(void) {
	/* If this webpage is https://, browsers deny any http:// downloading */
	return EM_ASM_INT_V({ return location.protocol === 'https:'; });
}


/*########################################################################################################################*
*-----------------------------------------------------------Menu----------------------------------------------------------*
*#########################################################################################################################*/
int interop_DownloadMap(const char* path, const char* filename) {
	return EM_ASM_({
		try {
			var name = UTF8ToString($0);
			var data = FS.readFile(name);
			var blob = new Blob([data], { type: 'application/octet-stream' });
			Module.saveBlob(blob, UTF8ToString($1));
			FS.unlink(name);
			return 0;
		} catch (e) {
			if (!(e instanceof FS.ErrnoError)) abort(e);
			return -e.errno;
		}
	}, path, filename);
}

void interop_UploadTexPack(const char* path) {
	/* Move from temp into texpacks folder */
	/* TODO: This is pretty awful and should be rewritten */
	EM_ASM_({ 
		var name = UTF8ToString($0);
		var data = FS.readFile(name);
		FS.writeFile('/texpacks/' + name.substring(1), data);
	}, path);
}


/*########################################################################################################################*
*---------------------------------------------------------Platform--------------------------------------------------------*
*#########################################################################################################################*/
void interop_GetIndexedDBError(char* buffer) {
	EM_ASM_({ if (window.cc_idbErr) stringToUTF8(window.cc_idbErr, $0, 64); }, buffer);
}

void interop_SyncFS(void) {
	EM_ASM({
		FS.syncfs(false, function(err) { 
			if (!err) return;
			console.log(err);
			ccall('Platform_LogError', 'void', ['string'], ['&cError saving files to IndexedDB:']);
			ccall('Platform_LogError', 'void', ['string'], ['   &c' + err]);
		}); 
	});
}

int interop_OpenTab(const char* url) {
	EM_ASM_({ window.open(UTF8ToString($0)); }, url);
	return 0;
}


/*########################################################################################################################*
*----------------------------------------------------------Window---------------------------------------------------------*
*#########################################################################################################################*/
int interop_CanvasWidth(void)  { return EM_ASM_INT_V({ return Module['canvas'].width  }); }
int interop_CanvasHeight(void) { return EM_ASM_INT_V({ return Module['canvas'].height }); }
int interop_ScreenWidth(void)  { return EM_ASM_INT_V({ return screen.width;  }); }
int interop_ScreenHeight(void) { return EM_ASM_INT_V({ return screen.height; }); }

int interop_IsAndroid(void) {
	return EM_ASM_INT_V({ return /Android/i.test(navigator.userAgent); });
}
int interop_IsIOS(void) {
	/* iOS 13 on iPad doesn't identify itself as iPad by default anymore */
	/*  https://stackoverflow.com/questions/57765958/how-to-detect-ipad-and-ipad-os-version-in-ios-13-and-up */
	return EM_ASM_INT_V({
		return /iPhone|iPad|iPod/i.test(navigator.userAgent) || 
		(navigator.platform === 'MacIntel' && navigator.maxTouchPoints && navigator.maxTouchPoints > 2);
	});
}

void interop_InitContainer(void) {
	/* Create wrapper div if necessary (so input textbox shows in fullscreen on android)*/
	EM_ASM({
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
	});
}

int interop_GetContainerID(void) {
	/* For chrome on android, need to make container div fullscreen instead */
	return EM_ASM_INT_V({ return document.getElementById('canvas_wrapper') ? 1 : 0; });
}

void interop_ForceTouchPageLayout(void) {
	EM_ASM( if (typeof(forceTouchLayout) === 'function') forceTouchLayout(); );
}

void interop_SetPageTitle(const char* title) {
	EM_ASM_({ document.title = UTF8ToString($0); }, title);
}

void interop_AddClipboardListeners(void) {
	/* Copy text, but only if user isn't selecting something else on the webpage */
	/* (don't check window.clipboardData here, that's handled in interop_TrySetClipboardText instead) */
	EM_ASM(window.addEventListener('copy', 
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
	);

	/* Paste text (window.clipboardData is handled in interop_TryGetClipboardText instead) */
	EM_ASM(window.addEventListener('paste',
		function(e) {
			if (e.clipboardData) {
				var contents = e.clipboardData.getData('text/plain');
				ccall('Window_GotClipboardText', 'void', ['string'], [contents]);
			}
		});
	);
}

void interop_TryGetClipboardText(void) {
	/* For IE11, use window.clipboardData to get the clipboard */
	EM_ASM_({ 
		if (window.clipboardData) {
			var contents = window.clipboardData.getData('Text');
			ccall('Window_StoreClipboardText', 'void', ['string'], [contents]);
		} 
	});
}

void interop_TrySetClipboardText(const char* text) {
	/* For IE11, use window.clipboardData to set the clipboard */
	/* For other browsers, instead use the window.copy events */
	EM_ASM_({ 
		if (window.clipboardData) {
			if (window.getSelection && window.getSelection().toString()) return;
			window.clipboardData.setData('Text', UTF8ToString($0));
		} else {
			window.cc_copyText = UTF8ToString($0);
		}
	}, text);
}

void interop_EnterFullscreen(void) {
	/* emscripten sets css size to screen's base width/height, */
	/*  except that becomes wrong when device rotates. */
	/* Better to just set CSS width/height to always be 100% */
	EM_ASM({
		var canvas = Module['canvas'];
		canvas.style.width  = '100%';
		canvas.style.height = '100%';
	});

	/* By default, pressing Escape will immediately exit fullscreen - which is */
	/*   quite annoying given that it is also the Menu key. Some browsers allow */
	/*   'locking' the Escape key, so that you have to hold down Escape to exit. */
	/* NOTE: This ONLY works when the webpage is a https:// one */
	EM_ASM({ try { navigator.keyboard.lock(["Escape"]); } catch (ex) { } });
}

/* Adjust from document coordinates to element coordinates */
void interop_AdjustXY(int* x, int* y) {
	EM_ASM_({
		var canvasRect = Module['canvas'].getBoundingClientRect();
		HEAP32[$0 >> 2] = HEAP32[$0 >> 2] - canvasRect.left;
		HEAP32[$1 >> 2] = HEAP32[$1 >> 2] - canvasRect.top;
	}, x, y);
}

void interop_RequestCanvasResize(void) {
	EM_ASM( if (typeof(resizeGameCanvas) === 'function') resizeGameCanvas(); );
}

void interop_SetCursorVisible(int visible) {
	if (visible) {
		EM_ASM(Module['canvas'].style['cursor'] = 'default'; );
	} else {
		EM_ASM(Module['canvas'].style['cursor'] = 'none'; );
	}
}

void interop_ShowDialog(const char* title, const char* msg) {
	EM_ASM_({ alert(UTF8ToString($0) + "\n\n" + UTF8ToString($1)); }, title, msg);
}

void interop_OpenKeyboard(const char* text, int type, const char* placeholder) {
	EM_ASM_({
		var elem = window.cc_inputElem;
		if (!elem) {
			if ($1 == 1) {
				elem = document.createElement('input');
				elem.setAttribute('inputmode', 'decimal');
			} else {
				elem = document.createElement('textarea');
			}
			elem.setAttribute('style', 'position:absolute; left:0; bottom:0; margin: 0px; width: 100%');
			elem.setAttribute('placeholder', UTF8ToString($2));
			elem.value = UTF8ToString($0);

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
	}, text, type, placeholder);
}

/* NOTE: When pressing 'Go' on the on-screen keyboard, some web browsers add \n to value */
void interop_SetKeyboardText(const char* text) {
	EM_ASM_({
		if (!window.cc_inputElem) return;
		var str = UTF8ToString($0);
		var cur = window.cc_inputElem.value;
		
		if (cur.length && cur[cur.length - 1] == '\n') { cur = cur.substring(0, cur.length - 1); }
		if (str != cur) window.cc_inputElem.value = str;
	}, text);
}

void interop_CloseKeyboard(void) {
	EM_ASM({
		if (!window.cc_inputElem) return;
		window.cc_container.removeChild(window.cc_divElem);
		window.cc_container.removeChild(window.cc_inputElem);
		window.cc_divElem   = null;
		window.cc_inputElem = null;
	});
}

void interop_OpenFileDialog(const char* filter) {
	EM_ASM_({
		var elem = window.cc_uploadElem;
		if (!elem) {
			elem = document.createElement('input');
			elem.setAttribute('type', 'file');
			elem.setAttribute('style', 'display: none');
			elem.accept = UTF8ToString($0);

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
	}, filter);
}


/*########################################################################################################################*
*--------------------------------------------------------GLContext--------------------------------------------------------*
*#########################################################################################################################*/
void interop_GetGpuRenderer(char* buffer, int len) { 
	EM_ASM_({
		var dbg = GLctx.getExtension('WEBGL_debug_renderer_info');
		var str = dbg ? GLctx.getParameter(dbg.UNMASKED_RENDERER_WEBGL) : "";
		stringToUTF8(str, $0, $1);
	}, buffer, len);
}
#endif
