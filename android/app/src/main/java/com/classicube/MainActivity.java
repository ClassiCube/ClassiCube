package com.classicube;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.Queue;
import java.util.concurrent.Semaphore;
import java.util.concurrent.ConcurrentLinkedQueue;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.ClipboardManager;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.res.Configuration;
import android.database.Cursor;
import android.graphics.PixelFormat;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.provider.Settings.Secure;
import android.text.Editable;
import android.text.InputType;
import android.text.Selection;
import android.text.SpannableStringBuilder;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.view.View;
import android.view.Window;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;

// This class contains all the glue/interop code for bridging ClassiCube to the java Android world.
// Some functionality is only available on later Android versions - try {} catch {} is used in such places 
//   to ensure that the game can still run on earlier Android versions (albeit with reduced functionality)
// Currently the minimum required API level to run the game is level 9 (Android 2.3). 
// When using Android functionality, always aim to add a comment with the API level that the functionality 
//   was added in, as this will make things easier if the minimum required API level is ever changed again

// implements InputQueue.Callback
public class MainActivity extends Activity 
{
	// ==================================================================
	// ---------------------------- COMMANDS ----------------------------
	// ==================================================================
	//  The main thread (which receives events) is separate from the game thread (which processes events)
	//  Therefore pushing/pulling events must be thread-safe, which is achieved through ConcurrentLinkedQueue
	//  Additionally, a cache is used (freeCmds) to avoid constantly allocating NativeCmdArgs instances
	class NativeCmdArgs { public int cmd, arg1, arg2, arg3, arg4; public String str; public Surface sur; }
	// static to persist across activity destroy/create
	static Queue<NativeCmdArgs> pending  = new ConcurrentLinkedQueue<NativeCmdArgs>();
	static Queue<NativeCmdArgs> freeCmds = new ConcurrentLinkedQueue<NativeCmdArgs>();
	
	NativeCmdArgs getCmdArgs() {
		NativeCmdArgs args = freeCmds.poll();
		return args != null ? args : new NativeCmdArgs();
	}
	
	void pushCmd(int cmd) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd = cmd;
		pending.add(args);
	}
	
	void pushCmd(int cmd, int a1) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd  = cmd;
		args.arg1 = a1;
		pending.add(args);
	}
	
	void pushCmd(int cmd, int a1, int a2, int a3, int a4) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd = cmd;
		args.arg1 = a1;
		args.arg2 = a2;
		args.arg3 = a3;
		args.arg4 = a4;
		pending.add(args);
	}

	void pushCmd(int cmd, String text) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd = cmd;
		args.str = text;
		pending.add(args);
	}
	
	void pushCmd(int cmd, Surface surface) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd = cmd;
		args.sur = surface;
		pending.add(args);
	}
	
	final static int CMD_KEY_DOWN = 0;
	final static int CMD_KEY_UP   = 1;
	final static int CMD_KEY_CHAR = 2;
	final static int CMD_POINTER_DOWN = 3;
	final static int CMD_POINTER_UP   = 4;
	final static int CMD_POINTER_MOVE = 5;
	
	final static int CMD_WIN_CREATED   = 6;
	final static int CMD_WIN_DESTROYED = 7;
	final static int CMD_WIN_RESIZED   = 8;
	final static int CMD_WIN_REDRAW    = 9;

	final static int CMD_APP_START   = 10;
	final static int CMD_APP_STOP    = 11;
	final static int CMD_APP_RESUME  = 12;
	final static int CMD_APP_PAUSE   = 13;
	final static int CMD_APP_DESTROY = 14;

	final static int CMD_GOT_FOCUS   = 15;
	final static int CMD_LOST_FOCUS  = 16;
	final static int CMD_CONFIG_CHANGED = 17;
	final static int CMD_LOW_MEMORY  = 18;

	final static int CMD_KEY_TEXT   = 19;
	final static int CMD_OFD_RESULT = 20;
	
	
	// ====================================================================
	// ------------------------------ EVENTS ------------------------------
	// ====================================================================
	InputMethodManager input;
	// static to persist across activity destroy/create
	static boolean gameRunning;

	void startGameAsync() {
		Log.i("CC_WIN", "handing off to native..");
		try {
			System.loadLibrary("classicube");
		} catch (UnsatisfiedLinkError ex) {
			ex.printStackTrace();
			showAlertAsync("Failed to start", ex.getMessage());
			return;
		}
		
		gameRunning = true;
		runGameAsync();
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// requestWindowFeature - API level 1
		// setSoftInputMode, SOFT_INPUT_STATE_UNSPECIFIED, SOFT_INPUT_ADJUST_RESIZE - API level 3
		input = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
		Log.i("CC_WIN", "CREATE EVENT");
		Window window = getWindow();
		Log.i("CC_WIN", "GAME RUNNING?" + gameRunning);
		//window.takeSurface(this);
		//window.takeInputQueue(this);
		// TODO: Should this be RGBA_8888??
		window.setFormat(PixelFormat.RGBX_8888);
		window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_UNSPECIFIED | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		// TODO: rendering over display cutouts causes a problem where opening onscreen keyboard
		//  stops resizing the game view. (e.g. meaning you can't see in-game chat input anymore)
		//  Apparently intentional (see LayoutParams.SOFT_INPUT_ADJUST_RESIZE documentation)
		// Need to find a solution that both renders over display cutouts and doesn't mess up onscreen input
		// renderOverDisplayCutouts();
		// TODO: semaphore for destroyed and surfaceDestroyed

		if (!gameRunning) startGameAsync();
		// TODO rethink to avoid this
		if (gameRunning) updateInstance();
		super.onCreate(savedInstanceState);
	}

	
	/*@Override
	public boolean dispatchKeyEvent(KeyEvent event) {
		int action = event.getAction();
		int code   = event.getKeyCode();
		
		if (action == KeyEvent.ACTION_DOWN) {
			pushCmd(CMD_KEY_DOWN, keyCode);
		
			int keyChar = event.getUnicodeChar();
			if (keyChar != 0) pushCmd(CMD_KEY_CHAR, keyChar);
			return true;
		} else if (action == KeyEvent.ACTION_UP) {
			pushCmd(CMD_KEY_UP, keyCode);
			return true;
		}
		return super.dispatchKeyEvent(event);
	}*/
	
	void pushTouch(int cmd, MotionEvent event, int i) {
		// getPointerId, getX, getY - API level 5
		int id = event.getPointerId(i);
		// TODO: Pass float to jni
		int x  = (int)event.getX(i);
		int y  = (int)event.getY(i);
		pushCmd(cmd, id, x, y, 0);
	}
	
	boolean handleTouchEvent(MotionEvent event) {
		// getPointerCount - API level 5
		// getActionMasked, getActionIndex - API level 8
		switch (event.getActionMasked()) {
		case MotionEvent.ACTION_DOWN:
		case MotionEvent.ACTION_POINTER_DOWN:
			pushTouch(CMD_POINTER_DOWN, event, event.getActionIndex());
			break;
			
		case MotionEvent.ACTION_UP:
		case MotionEvent.ACTION_POINTER_UP:
			pushTouch(CMD_POINTER_UP, event, event.getActionIndex());
			break;
			
		case MotionEvent.ACTION_MOVE:
			for (int i = 0; i < event.getPointerCount(); i++) {
				pushTouch(CMD_POINTER_MOVE, event, i);
			}
		}
		return true;
	}
	
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		// Ignore volume keys
		if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) return false;
		if (keyCode == KeyEvent.KEYCODE_VOLUME_MUTE) return false;
		if (keyCode == KeyEvent.KEYCODE_VOLUME_UP)   return false;
		
		// TODO: not always handle (use Window_MapKey)
		pushCmd(CMD_KEY_DOWN, keyCode);
		
		int keyChar = event.getUnicodeChar();
		if (keyChar != 0) pushCmd(CMD_KEY_CHAR, keyChar);
		return true;
	}
	
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		// TODO: not always handle (use Window_MapKey)
		pushCmd(CMD_KEY_UP, keyCode);
		return true;
	}
	
	@Override
	protected void onStart() { 
		super.onStart();  
		pushCmd(CMD_APP_START); 
	}
	
	@Override
	protected void onStop() { 
		super.onStop();
		pushCmd(CMD_APP_STOP);
		// In case game thread is blocked on showing dialog
		releaseDialogSem();
	}
	
	@Override
	protected void onResume() {
		attachSurface();
		super.onResume();
		pushCmd(CMD_APP_RESUME); 
	}
	
	@Override
	protected void onPause() {
		// setContentView - API level 1
		// Can't use null.. TODO is there a better way?
		setContentView(new View(this));
		super.onPause();
		pushCmd(CMD_APP_PAUSE); 
	}
	
	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		pushCmd(hasFocus ? CMD_GOT_FOCUS : CMD_LOST_FOCUS);
	}
	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		//pushCmd(CMD_CONFIG_CHANGED);
		// not needed because it's the surfaceChanged event that matters
	}
	@Override
	public void onLowMemory() { super.onLowMemory(); pushCmd(CMD_LOW_MEMORY); }
	
	@Override
	public void onDestroy() {
		Log.i("CC_WIN", "APP DESTROYED");
		super.onDestroy();
		pushCmd(CMD_APP_DESTROY);
	}
	
	// Called by the game thread to actually process events
	public void processEvents() {
		for (;;) {
			NativeCmdArgs c = pending.poll();
			if (c == null) return;
			
			switch (c.cmd) {
			case CMD_KEY_DOWN: processKeyDown(c.arg1); break;
			case CMD_KEY_UP:   processKeyUp(c.arg1);   break;
			case CMD_KEY_CHAR: processKeyChar(c.arg1); break;
			case CMD_KEY_TEXT: processKeyText(c.str);  break;
	
			case CMD_POINTER_DOWN: processPointerDown(c.arg1, c.arg2, c.arg3, c.arg4); break;
			case CMD_POINTER_UP:   processPointerUp(  c.arg1, c.arg2, c.arg3, c.arg4); break;
			case CMD_POINTER_MOVE: processPointerMove(c.arg1, c.arg2, c.arg3, c.arg4); break;
	
			case CMD_WIN_CREATED:   processSurfaceCreated(c.sur);   break;
			case CMD_WIN_DESTROYED: processSurfaceDestroyed();      break;
			case CMD_WIN_RESIZED:   processSurfaceResized(c.sur);   break;
			case CMD_WIN_REDRAW:    processSurfaceRedrawNeeded();   break;

			case CMD_APP_START:   processOnStart();   break;
			case CMD_APP_STOP:    processOnStop();    break;
			case CMD_APP_RESUME:  processOnResume();  break;
			case CMD_APP_PAUSE:   processOnPause();   break;
			case CMD_APP_DESTROY: processOnDestroy(); break;

			case CMD_GOT_FOCUS:	  processOnGotFocus();	  break;
			case CMD_LOST_FOCUS:	 processOnLostFocus();	 break;
			//case CMD_CONFIG_CHANGED: processOnConfigChanged(); break;
			case CMD_LOW_MEMORY:	 processOnLowMemory();	 break;

			case CMD_OFD_RESULT: processOFDResult(c.str); break;
			}

			c.str = null;
			c.sur = null; // don't keep a reference to it
			freeCmds.add(c);
		}
	}
	
	native void processKeyDown(int code);
	native void processKeyUp(int code);
	native void processKeyChar(int code);
	native void processKeyText(String str);
	
	native void processPointerDown(int id, int x, int y, int isMouse);
	native void processPointerUp(  int id, int x, int y, int isMouse);
	native void processPointerMove(int id, int x, int y, int isMouse);

	native void processSurfaceCreated(Surface sur);
	native void processSurfaceDestroyed();
	native void processSurfaceResized(Surface sur);
	native void processSurfaceRedrawNeeded();

	native void processOnStart();
	native void processOnStop();
	native void processOnResume();
	native void processOnPause();
	native void processOnDestroy();

	native void processOnGotFocus();
	native void processOnLostFocus();
	//native void processOnConfigChanged();
	native void processOnLowMemory();

	native void processOFDResult(String path);
	
	native void runGameAsync();
	native void updateInstance();
	
	
	// ====================================================================
	// ------------------------------ VIEWS -------------------------------
	// ====================================================================
	volatile boolean fullscreen;
	// static to persist across activity destroy/create
	static final Semaphore winDestroyedSem = new Semaphore(0, true);
	SurfaceHolder.Callback callback;
	CCView curView;
	
	// SurfaceHolder.Callback - API level 1
	class CCSurfaceCallback implements SurfaceHolder.Callback {
		public void surfaceCreated(SurfaceHolder holder) {
			// getSurface - API level 1
			Log.i("CC_WIN", "win created " + holder.getSurface());
			MainActivity.this.pushCmd(CMD_WIN_CREATED, holder.getSurface());
		}
		
		public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
			// getSurface - API level 1
			Log.i("CC_WIN", "win changed " + holder.getSurface());
			MainActivity.this.pushCmd(CMD_WIN_RESIZED, holder.getSurface());
		}
		
		public void surfaceDestroyed(SurfaceHolder holder) {
			// getSurface, removeCallback - API level 1
			Log.i("CC_WIN", "win destroyed " + holder.getSurface());
			Log.i("CC_WIN", "cur view " + curView);
			holder.removeCallback(this);
			
			//08-02 21:03:02.967: E/BufferQueueProducer(1350): [SurfaceView - com.classicube.ClassiCube/com.classicube.MainActivity#0] disconnect: not connected (req=2)
			//08-02 21:03:02.968: E/SurfaceFlinger(1350): Failed to find layer (SurfaceView - com.classicube.ClassiCube/com.classicube.MainActivity#0) in layer parent (no-parent).
	
			MainActivity.this.pushCmd(CMD_WIN_DESTROYED);
			// In case game thread is blocked showing a dialog on main thread
			releaseDialogSem();
			
			// per the android docs for SurfaceHolder.Callback
			// "If you have a rendering thread that directly accesses the surface, you must ensure
			// that thread is no longer touching the Surface before returning from this function."
			try {
				winDestroyedSem.acquire();
			} catch (InterruptedException e) { }
		}
	}
	
	// SurfaceHolder.Callback2 - API level 9
	class CCSurfaceCallback2 extends CCSurfaceCallback implements SurfaceHolder.Callback2 {
		public void surfaceRedrawNeeded(SurfaceHolder holder) {
			// getSurface - API level 1
			Log.i("CC_WIN", "win dirty " + holder.getSurface());
			MainActivity.this.pushCmd(CMD_WIN_REDRAW);
		}
	}
	
	// Called by the game thread to notify the main thread
	// that it is safe to destroy the window surface now
	public void processedSurfaceDestroyed() { winDestroyedSem.release(); }
	
	void createSurfaceCallback() {
		if (callback != null) return;
		try {
			callback = new CCSurfaceCallback2(); 
		} catch (NoClassDefFoundError ex) {
			ex.printStackTrace();
			callback = new CCSurfaceCallback();
		}
	}
	 
	void attachSurface() {
		// setContentView, requestFocus, getHolder, addCallback, RGBX_8888 - API level 1
		createSurfaceCallback();
		curView = new CCView(this);
		curView.getHolder().addCallback(callback);
		curView.getHolder().setFormat(PixelFormat.RGBX_8888);
		
		setContentView(curView);
		curView.requestFocus();
		if (fullscreen) setUIVisibility(FULLSCREEN_FLAGS);
	}

	class CCView extends SurfaceView {
		SpannableStringBuilder kbText;

		public CCView(Context context) {
			// setFocusable, setFocusableInTouchMode - API level 1
			super(context);
			setFocusable(true);
			setFocusableInTouchMode(true);
		}

		@Override
		public boolean dispatchTouchEvent(MotionEvent ev) {
			return MainActivity.this.handleTouchEvent(ev) || super.dispatchTouchEvent(ev);
		}

		@Override
		public InputConnection onCreateInputConnection(EditorInfo attrs) {
			// BaseInputConnection, IME_ACTION_GO, IME_FLAG_NO_EXTRACT_UI - API level 3
			attrs.actionLabel = null;
			attrs.inputType   = MainActivity.this.getKeyboardType();
			attrs.imeOptions  = MainActivity.this.getKeyboardOptions();

			kbText = new SpannableStringBuilder(MainActivity.this.keyboardText);

			InputConnection ic = new BaseInputConnection(this, true) {
				boolean inited;
				void updateText() { MainActivity.this.pushCmd(CMD_KEY_TEXT, kbText.toString()); }

				@Override
				public Editable getEditable() {
					if (!inited) {
						// needed to set selection, otherwise random crashes later with backspacing
						// set selection to end, so backspacing after opening keyboard with text still works
						Selection.setSelection(kbText, kbText.toString().length());
						inited = true;
					}
					return kbText;
				}

				@Override
				public boolean setComposingText(CharSequence text, int newCursorPosition) {
					boolean success = super.setComposingText(text, newCursorPosition);
					updateText();
					return success;
				}

				@Override
				public boolean deleteSurroundingText(int beforeLength, int afterLength) {
					
					boolean success = super.deleteSurroundingText(beforeLength, afterLength);
					updateText();
					return success;
				}

				@Override
				public boolean commitText(CharSequence text, int newCursorPosition) {
					boolean success = super.commitText(text, newCursorPosition);
					updateText();
					return success;
				}

				@Override
				public boolean sendKeyEvent(KeyEvent ev) {
					// getSelectionStart - API level 1
					if (ev.getAction() != KeyEvent.ACTION_DOWN) return super.sendKeyEvent(ev);
					int code  = ev.getKeyCode();
					int uni   = ev.getUnicodeChar();

					// start is -1 sometimes, and trying to insert/delete there crashes
					int start = Selection.getSelectionStart(kbText);
					if (start == -1) start = kbText.toString().length();

					if (code == KeyEvent.KEYCODE_ENTER) {
						// enter maps to \n but that should not be intercepted
					} else if (code == KeyEvent.KEYCODE_DEL) {
						if (start <= 0) return false;
						kbText.delete(start - 1, start);
						updateText();
						return false;
					} else if (uni != 0) {
						kbText.insert(start, String.valueOf((char)uni));
						updateText();
						return false;
					}
					return super.sendKeyEvent(ev);
				}

			};
			//String text = MainActivity.this.keyboardText;
			//if (text != null) ic.setComposingText(text, 0);
			return ic;
		}
	}
	
	
	// ==================================================================
	// ---------------------------- PLATFORM ----------------------------
	// ==================================================================
	//  Implements java Android side of the Android Platform backend (See Platform.c)
	public void setupForGame() {
		// Once a surface has been locked for drawing with canvas, can't ever be detached
		// This means trying to attach an OpenGL ES context to the surface will fail
		// So just destroy the current surface and make a new one
		runOnUiThread(new Runnable() {
			public void run() {
				attachSurface();
			}
		});
	}
	
	public void startOpen(String url) {
		// ACTION_VIEW, resolveActivity, getPackageManager, startActivity - API level 1
		Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
		if (intent.resolveActivity(getPackageManager()) != null) {
			startActivity(intent);
		}
	}
	
	public String getGameDataDirectory() {
		// getExternalFilesDir - API level 8
		return getExternalFilesDir(null).getAbsolutePath();
	}

	public String getGameCacheDirectory() {
		// getExternalCacheDir - API level 8
		File root = getExternalCacheDir();
		if (root != null) return root.getAbsolutePath();

		// although exceedingly rare, getExternalCacheDir() can technically fail
		//   "... May return null if shared storage is not currently available."
		// getCacheDir - API level 1
		return getCacheDir().getAbsolutePath();
	}
	
	public String getUUID() {
		// getContentResolver - API level 1
		// getString, ANDROID_ID - API level 3
		return Secure.getString(getContentResolver(), Secure.ANDROID_ID);
	}
	
	public long getApkUpdateTime() {
		try {
			// getApplicationInfo - API level 4
			ApplicationInfo info = getApplicationInfo();
			File apkFile = new File(info.sourceDir);
			
			// https://developer.android.com/reference/java/io/File#lastModified()
			//  lastModified is returned in milliseconds
			return apkFile.lastModified() / 1000;
		} catch (Exception ex) {
			return 0;
		}
	}
	

	// ====================================================================
	// ------------------------------ WINDOW ------------------------------
	// ====================================================================
	//  Implements java Android side of the Android Window backend (See Window.c)
	volatile int keyboardType;
	volatile String keyboardText = "";
	// setTitle - API level 1
	public void setWindowTitle(String str) { setTitle(str); }

	public void openKeyboard(String text, int flags) {
		// restartInput, showSoftInput - API level 3
		keyboardType = flags;
		keyboardText = text;
		//runOnUiThread(new Runnable() {
			//public void run() {
				// Restart view so it uses the right INPUT_TYPE
				if (curView != null) input.restartInput(curView);
				if (curView != null) input.showSoftInput(curView, 0);
			//}
		//});
	}

	public void closeKeyboard() {
		// InputMethodManager, hideSoftInputFromWindow - API level 3
		// getWindow, getDecorView, getWindowToken - API level 1
		InputMethodManager input = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
		View view = getWindow().getDecorView();
		input.hideSoftInputFromWindow(view.getWindowToken(), 0);
		keyboardText = "";
		//runOnUiThread(new Runnable() {
			//public void run() {
				if (curView != null) input.hideSoftInputFromWindow(curView.getWindowToken(), 0);
			//}
		//});
	}

	public void setKeyboardText(String text) {
		keyboardText = text;
		// Restart view because text changed externally
		if (curView == null) return;

		// Try to avoid restarting input if possible
		if (curView.kbText != null) {
			String curText = curView.kbText.toString();
			if (text.equals(curText)) return;
		}

		// Have to restart input because text changed externally
		// NOTE: Doing this still has issues, like changing keyboard tab back to default one,
		//   and one user has a problem where it also resets letters to uppercase
		// TODO: Consider just doing kbText.replace instead
		// (see https://chromium.googlesource.com/chromium/src/+/d1421a5faf9dc2d3b3cad10640576b24a092d9ba/content/public/android/java/src/org/chromium/content/browser/input/AdapterInputConnection.java)
		input.restartInput(curView);
	}

	public int getKeyboardType() {
		// TYPE_CLASS_TEXT, TYPE_CLASS_NUMBER, TYPE_TEXT_VARIATION_PASSWORD - API level 3
		int type = keyboardType & 0xFF;
		
		if (type == 2) return InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD;
		if (type == 1) return InputType.TYPE_CLASS_NUMBER; // KEYBOARD_TYPE_NUMERIC
		if (type == 3) return InputType.TYPE_CLASS_NUMBER; // KEYBOARD_TYPE_INTEGER
		return InputType.TYPE_CLASS_TEXT;
	}
	
	public int getKeyboardOptions() {
		// IME_ACTION_GO, IME_FLAG_NO_EXTRACT_UI - API level 3
		if ((keyboardType & 0x100) != 0) {
			return EditorInfo.IME_ACTION_SEND | EditorInfo.IME_FLAG_NO_EXTRACT_UI;
		} else {
			return EditorInfo.IME_ACTION_GO   | EditorInfo.IME_FLAG_NO_EXTRACT_UI;
		}
	}

	public String getClipboardText() {
		// ClipboardManager, getText() - API level 11
		ClipboardManager clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
		CharSequence chars = clipboard.getText();
		return chars == null ? null : chars.toString();
	}

	public void setClipboardText(String str) {
		// ClipboardManager, setText() - API level 11
		ClipboardManager clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
		clipboard.setText(str);
	}
	
	DisplayMetrics getMetrics() {
		// getDefaultDisplay, getMetrics - API level 1
		DisplayMetrics dm = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(dm);
		return dm;
	}

	// Using raw DPI gives values such as 1.86, 3.47, 1.62.. not exactly ideal
	// One device also gave differing x/y DPI which stuffs up the movement overlay
	public float getDpiX() { return getMetrics().density; }
	public float getDpiY() { return getMetrics().density; }

	final Semaphore dialogSem = new Semaphore(0, true);
	
	void releaseDialogSem() {
		// Only release when no waiting threads (otherwise showAlert doesn't block when called)
		if (!dialogSem.hasQueuedThreads()) return;
		dialogSem.release();
	}

	void renderOverDisplayCutouts() {
		// FLAG_TRANSLUCENT_STATUS - API level 19
		// layoutInDisplayCutoutMode - API level 28
		// LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES - API level 28
		try {
			Window window = getWindow();
			window.addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
			window.getAttributes().layoutInDisplayCutoutMode =
				WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
		} catch (NoSuchFieldError ex) {
			ex.printStackTrace();
		} catch (NoSuchMethodError ex) {
			ex.printStackTrace();
		}
	}

	void showAlertAsync(final String title, final String message) {
		//final Activity activity = this;
		// setTitle, setMessage, setPositiveButton, setCancelable, create, show - API level 1
		runOnUiThread(new Runnable() {
			public void run() {
				AlertDialog.Builder dlg = new AlertDialog.Builder(MainActivity.this);
				dlg.setTitle(title);
				dlg.setMessage(message);
				
				dlg.setPositiveButton("Close", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int id) { releaseDialogSem(); }
				});
				dlg.setCancelable(false);
				dlg.create().show();
			}
		});
	}

	
	public void showAlert(final String title, final String message) {
		showAlertAsync(title, message);
		try {
			dialogSem.acquire(); // Block game thread
		} catch (InterruptedException e) { }
	}

	public int getWindowState() { return fullscreen ? 1 : 0; }
	// SYSTEM_UI_FLAG_HIDE_NAVIGATION - API level 14
	// SYSTEM_UI_FLAG_FULLSCREEN - API level 16
	// SYSTEM_UI_FLAG_IMMERSIVE_STICKY - API level 19
	final static int FULLSCREEN_FLAGS = View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
	
	void setUIVisibility(int flags) {
		if (curView == null) return;
		// setSystemUiVisibility - API level 11
		try {
			curView.setSystemUiVisibility(flags);
		} catch (NoSuchMethodError ex) {
			ex.printStackTrace();
		}
	}

	public void enterFullscreen() {
		fullscreen = true;
		runOnUiThread(new Runnable() {
			public void run() { setUIVisibility(FULLSCREEN_FLAGS); }
		});
    }

    public void exitFullscreen() {
		fullscreen = false;
		runOnUiThread(new Runnable() {
			public void run() { setUIVisibility(View.SYSTEM_UI_FLAG_VISIBLE); }
		});
    }
	
	public String shareScreenshot(String path) {
		try {
			Uri uri;
			if (android.os.Build.VERSION.SDK_INT >= 23){ // android 6.0
				uri = CCFileProvider.getUriForFile("screenshots/" + path);
			} else {
				// when trying to use content:// URIs on my android 4.0.3 test device
				//   - 1 app crashed
				//   - 1 app wouldn't show image previews
				// so fallback to file:// on older devices as they seem to reliably work
				File file = new File(getGameDataDirectory() + "/screenshots/" + path);
				uri = Uri.fromFile(file);
			}
			Intent intent = new Intent();
			
			intent.setAction(Intent.ACTION_SEND);
			intent.putExtra(Intent.EXTRA_STREAM, uri);
			intent.setType("image/png");
			intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
			startActivity(Intent.createChooser(intent, "share via"));
		} catch (Exception ex) {
			return ex.toString();
		}
		return "";
	}


	static String uploadFolder, savePath;
	final static int OPEN_REQUEST = 0x4F50454E;
	final static int SAVE_REQUEST = 0x53415645;

	// https://stackoverflow.com/questions/36557879/how-to-use-native-android-file-open-dialog
	// https://developer.android.com/guide/topics/providers/document-provider
	// https://developer.android.com/training/data-storage/shared/documents-files#java
	// https://stackoverflow.com/questions/5657411/android-getting-a-file-uri-from-a-content-uri
	public int openFileDialog(String folder) {
		uploadFolder = folder;

		try {
			Intent intent = new Intent()
					.setType("*/*")
					.setAction(Intent.ACTION_GET_CONTENT);

			startActivityForResult(Intent.createChooser(intent, "Select a file"), OPEN_REQUEST);
		} catch (Exception ex) {
			ex.printStackTrace();
			return 0;// TODO log error to in-game
		}
		return 1;
	}

	// https://stackoverflow.com/questions/8586691/how-to-open-file-save-dialog-in-android
	public int saveFileDialog(String path, String name) {
		savePath = path;

		try {
			Intent intent = new Intent()
					.setType("/")
					.addCategory(Intent.CATEGORY_OPENABLE)
					.setAction(Intent.ACTION_CREATE_DOCUMENT)
					.setType("application/octet-stream")
					.putExtra(Intent.EXTRA_TITLE, name);

			startActivityForResult(Intent.createChooser(intent, "Choose destination"), SAVE_REQUEST);
		} catch (Exception ex) {
			ex.printStackTrace();
			return 0;// TODO log error to in-game
		}
		return 1;
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		if (resultCode != RESULT_OK) return;

		if (requestCode == OPEN_REQUEST) {
			handleOpenResult(data);
		} else if (requestCode == SAVE_REQUEST) {
			handleSaveResult(data);
		}
	}

	void handleSaveResult(Intent data) {
		try {
			Uri selected = data.getData();
			saveTempToContent(selected, savePath);
		} catch (Exception ex) {
			ex.printStackTrace();
			// TODO log error to in-game
		}
	}

	void saveTempToContent(Uri uri, String path) throws IOException {
		File file = new File(getGameDataDirectory() + "/" + path);
		OutputStream output = null;
		InputStream input   = null;

		try {
			input  = new FileInputStream(file);
			output = getContentResolver().openOutputStream(uri);
			copyStreamData(input, output);
			file.delete();
		} finally {
			if (output != null) output.close();
			if (input != null)  input.close();
		}
	}

	void handleOpenResult(Intent data) {
		try {
			Uri selected = data.getData();
			String name  = getContentFilename(selected);
			String path  = saveContentToTemp(selected, uploadFolder, name);
			pushCmd(CMD_OFD_RESULT, uploadFolder + "/" + name);
		} catch (Exception ex) {
			ex.printStackTrace();
			// TODO log error to in-game
		}
	}

	String getContentFilename(Uri uri) {
		Cursor cursor = getContentResolver().query(uri, new String[] { OpenableColumns.DISPLAY_NAME }, null, null, null);
		if (cursor != null && cursor.moveToFirst()) {
			int cIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
			if (cIndex != -1) return cursor.getString(cIndex);
		}
		return null;
	}

	String saveContentToTemp(Uri uri, String folder, String name) throws IOException {
		//File file = new File(getExternalFilesDir(null), folder + "/" + name);
		File file = new File(getGameDataDirectory() + "/" + folder + "/" + name);
		file.getParentFile().mkdirs();

		OutputStream output = null;
		InputStream input   = null;

		try {
			output = new FileOutputStream(file);
			input  = getContentResolver().openInputStream(uri);
			copyStreamData(input, output);
		} finally {
			if (output != null) output.close();
			if (input != null)  input.close();
		}
		return file.getAbsolutePath();
	}

	static void copyStreamData(InputStream input, OutputStream output) throws IOException {
		byte[] temp = new byte[8192];
		int length;
		while ((length = input.read(temp)) > 0)
			output.write(temp, 0, length);
	}


	// ======================================================================
	// -------------------------------- HTTP --------------------------------
	// ======================================================================
	//  Implements java Android side of the Android HTTP backend (See Http.c)
	static HttpURLConnection conn;
	static InputStream src;
	static byte[] readCache = new byte[8192];

	public static int httpInit(String url, String method) {
		try {
			conn = (HttpURLConnection)new URL(url).openConnection();
			conn.setDoInput(true);
			conn.setRequestMethod(method);
			conn.setInstanceFollowRedirects(true);
			
			httpAddMethodHeaders(method);
			return 0;
		} catch (Exception ex) {
			return httpOnError(ex);
		}
	}
	
	static void httpAddMethodHeaders(String method) {
		if (!method.equals("HEAD")) return;

		// Ever since dropbox switched to to chunked transfer encoding,
		//  sending a HEAD request to dropbox always seems to result in the
		//  next GET request failing with 'Unexpected status line' ProtocolException
		// Seems to be a known issue: https://github.com/square/okhttp/issues/3689
		// Simplest workaround is to ask for connection to be closed after HEAD request
		httpSetHeader("connection", "close");
	}

	public static void httpSetHeader(String name, String value) {
		conn.setRequestProperty(name, value);
	}

	public static int httpSetData(byte[] data) {
		try {
			conn.setDoOutput(true);
			conn.getOutputStream().write(data);
			conn.getOutputStream().flush();
			return 0;
		} catch (Exception ex) {
			return httpOnError(ex);
		}
	}

	public static int httpPerform() {
		int len;
		try {
			conn.connect();
			// Some implementations also provide this as getHeaderField(0), but some don't
			httpParseHeader("HTTP/1.1 " + conn.getResponseCode() + " MSG");
			
			// Legitimate webservers aren't going to reply with over 200 headers
			for (int i = 0; i < 200; i++) {
				String key = conn.getHeaderFieldKey(i);
				String val = conn.getHeaderField(i);
				if (key == null && val == null) break;
				
				if (key == null) {
					httpParseHeader(val);
				} else {
					httpParseHeader(key + ":" + val);
				}
			}

			src = conn.getInputStream();
			while ((len = src.read(readCache)) > 0) {
				httpAppendData(readCache, len);
			}

			httpFinish();
			return 0;
		} catch (Exception ex) {
			return httpOnError(ex);
		}
	}

	static void httpFinish() {
		conn = null;
		try {
			src.close();
		} catch (Exception ex) { }
		src = null;
	}

	// TODO: Should we prune this list?
	static List<String> errors = new ArrayList<String>();

	static int httpOnError(Exception ex) {
		ex.printStackTrace();
		httpFinish();
		errors.add(ex.getMessage());
		return -errors.size(); // don't want 0 as an error code
	}

	public static String httpDescribeError(int res) {
		res = -res - 1;
		return res >= 0 && res < errors.size() ? errors.get(res) : null;
	}

	native static void httpParseHeader(String header);
	native static void httpAppendData(byte[] data, int len);
}
