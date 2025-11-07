package com.classicube;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.KeyStore;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
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
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.provider.Settings.Secure;
import android.text.InputType;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.WindowManager;
import android.view.View;
import android.view.Window;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;

import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509TrustManager;

// This class contains all the glue/interop code for bridging ClassiCube to the java Android world.
// Some functionality is only available on later Android versions - try {} catch {} is used in such places 
//   to ensure that the game can still run on earlier Android versions (albeit with reduced functionality)
// Currently the minimum required API level to run the game is level 9 (Android 2.3). 
// When using Android functionality, always aim to add a comment with the API level that the functionality 
//   was added in, as this will make things easier if the minimum required API level is ever changed again

// implements InputQueue.Callback
public class MainActivity extends Activity 
{
	public boolean launcher;
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

	public void pushCmd(int cmd) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd = cmd;
		pending.add(args);
	}

	public void pushCmd(int cmd, int a1) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd  = cmd;
		args.arg1 = a1;
		pending.add(args);
	}

	public void pushCmd(int cmd, int a1, int a2) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd  = cmd;
		args.arg1 = a1;
		args.arg2 = a2;
		pending.add(args);
	}
	
	public void pushCmd(int cmd, int a1, int a2, int a3, int a4) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd = cmd;
		args.arg1 = a1;
		args.arg2 = a2;
		args.arg3 = a3;
		args.arg4 = a4;
		pending.add(args);
	}

	public void pushCmd(int cmd, String text) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd = cmd;
		args.str = text;
		pending.add(args);
	}

	public void pushCmd(int cmd, int a1, String str) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd  = cmd;
		args.arg1 = a1;
		args.str  = str;
		pending.add(args);
	}

	public void pushCmd(int cmd, SurfaceHolder holder) {
		NativeCmdArgs args = getCmdArgs();
		Rect rect = holder.getSurfaceFrame();

		args.cmd  = cmd;
		args.sur  = holder.getSurface();
		args.arg1 = rect.width();
		args.arg2 = rect.height();
		pending.add(args);
	}
	
	public final static int CMD_KEY_DOWN = 0;
	public final static int CMD_KEY_UP   = 1;
	public final static int CMD_KEY_CHAR = 2;
	public final static int CMD_POINTER_DOWN = 3;
	public final static int CMD_POINTER_UP   = 4;
	public final static int CMD_POINTER_MOVE = 5;

	public final static int CMD_WIN_CREATED   = 6;
	public final static int CMD_WIN_DESTROYED = 7;
	public final static int CMD_WIN_RESIZED   = 8;
	public final static int CMD_WIN_REDRAW    = 9;

	public final static int CMD_APP_START   = 10;
	public final static int CMD_APP_STOP    = 11;
	public final static int CMD_APP_RESUME  = 12;
	public final static int CMD_APP_PAUSE   = 13;
	public final static int CMD_APP_DESTROY = 14;

	public final static int CMD_GOT_FOCUS   = 15;
	public final static int CMD_LOST_FOCUS  = 16;
	public final static int CMD_CONFIG_CHANGED = 17;
	public final static int CMD_LOW_MEMORY  = 18;

	public final static int CMD_KEY_TEXT   = 19;
	public final static int CMD_OFD_RESULT = 20;

	public final static int CMD_UI_CREATED  = 21;
	public final static int CMD_UI_CLICKED  = 22;
	public final static int CMD_UI_CHANGED  = 23;
	public final static int CMD_UI_STRING   = 24;

	public final static int CMD_GPAD_AXISL  = 25;
	public final static int CMD_GPAD_AXISR  = 26;
	
	
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

			case CMD_GPAD_AXISL: processJoystickL(c.arg1, c.arg2); break;
			case CMD_GPAD_AXISR: processJoystickR(c.arg1, c.arg2); break;

			case CMD_WIN_CREATED:   processSurfaceCreated(c.sur, c.arg1, c.arg2); break;
			case CMD_WIN_DESTROYED: processSurfaceDestroyed();      break;
			case CMD_WIN_RESIZED:   processSurfaceResized(c.arg1, c.arg2); break;
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

	native void processJoystickL(int x, int y);
	native void processJoystickR(int x, int y);

	native void processSurfaceCreated(Surface sur, int w, int h);
	native void processSurfaceDestroyed();
	native void processSurfaceResized(int w, int h);
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
	public View curView;

	public void setActiveView(View view) {
		// setContentView, requestFocus - API level 1
		curView = view;
		setContentView(view);
		curView.requestFocus();

		if (fullscreen) setUIVisibility(FULLSCREEN_FLAGS);
		hookMotionListener(view);
	}
	
	void hookMotionListener(View view) {
		try {
			CCMotionListener listener = new CCMotionListener(this);
			view.setOnGenericMotionListener(listener);
		} catch (Exception ex) {
			// Unsupported on android 12
		} catch (NoClassDefFoundError ex) {
			// Unsupported on android 12
		}
	}
	
	// SurfaceHolder.Callback - API level 1
	class CCSurfaceCallback implements SurfaceHolder.Callback {
		public void surfaceCreated(SurfaceHolder holder) {
			// getSurface - API level 1
			Log.i("CC_WIN", "win created " + holder.getSurface());
			Rect r = holder.getSurfaceFrame();
			MainActivity.this.pushCmd(CMD_WIN_CREATED, holder);
		}
		
		public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
			// getSurface - API level 1
			Log.i("CC_WIN", "win changed " + holder.getSurface());
			Rect r = holder.getSurfaceFrame();
			MainActivity.this.pushCmd(CMD_WIN_RESIZED, holder);
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
		CCView view = new CCView(this);
		view.getHolder().addCallback(callback);
		view.getHolder().setFormat(PixelFormat.RGBX_8888);

		setActiveView(view);
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
		CCView view = (CCView)curView;
		if (view.kbText != null) {
			String curText = view.kbText.toString();
			if (text.equals(curText)) return;
		}

		// Have to restart input because text changed externally
		// NOTE: Doing this still has issues, like changing keyboard tab back to default one,
		//   and one user has a problem where it also resets letters to uppercase
		// TODO: Consider just doing kbText.replace instead
		// (see https://chromium.googlesource.com/chromium/src/+/d1421a5faf9dc2d3b3cad10640576b24a092d9ba/content/public/android/java/src/org/chromium/content/browser/input/AdapterInputConnection.java)
		input.restartInput(curView);
	}

	public static int calcKeyboardType(int kbType) {
		// TYPE_CLASS_TEXT, TYPE_CLASS_NUMBER, TYPE_TEXT_VARIATION_PASSWORD - API level 3
		int type = kbType & 0xFF;
		
		if (type == 2) return InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD;
		if (type == 1) return InputType.TYPE_CLASS_NUMBER; // KEYBOARD_TYPE_NUMERIC
		if (type == 3) return InputType.TYPE_CLASS_NUMBER; // KEYBOARD_TYPE_INTEGER
		return InputType.TYPE_CLASS_TEXT;
	}
	
	public static int calcKeyboardOptions(int kbType) {
		// IME_ACTION_GO, IME_FLAG_NO_EXTRACT_UI - API level 3
		if ((kbType & 0x100) != 0) {
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
	// -------------------------------- SSL ---------------------------------
	// ======================================================================
	static X509TrustManager trust;
	static CertificateFactory certFactory;
	static ArrayList<X509Certificate> chain = new ArrayList<X509Certificate>();

	static X509TrustManager CreateTrust() throws Exception {
		TrustManagerFactory factory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
		// Load default Trust Anchor certificates
		factory.init((KeyStore)null);

		TrustManager[] trustManagers = factory.getTrustManagers();
		for (int i = 0; i < trustManagers.length; i++)
		{
			// Should be first entry, e.g. X509TrustManagerImpl
			if (trustManagers[i] instanceof X509TrustManager)
				return (X509TrustManager)trustManagers[i];
		}
		return null;
	}

	public static int sslCreateTrust() {
		try {
			trust = CreateTrust();
			return 1;
		} catch (Exception ex) {
			ex.printStackTrace();
			return 0;
		}
	}

	public static void sslAddCert(byte[] data) {
		try {
			if (certFactory == null) certFactory = CertificateFactory.getInstance("X.509");
			if (certFactory == null) return;

			InputStream in = new ByteArrayInputStream(data);
			X509Certificate cert = (X509Certificate) certFactory.generateCertificate(in);
			chain.add(cert);
		} catch (Exception ex) {
			ex.printStackTrace();
		}
	}

	public static int sslVerifyChain() {
		int result = -200;
		try {
			X509Certificate[] certs = new X509Certificate[chain.size()];
			for (int i = 0; i < chain.size(); i++) certs[i] = chain.get(i);

			trust.checkServerTrusted(certs, "INV");
			// standard java checks auth type, but android doesn't
			// See https://issues.chromium.org/issues/40955801
			result = 0;
		} catch (Exception ex) {
			ex.printStackTrace();
		}

		chain.clear();
		return result;
	}


	// ======================================================================
	// --------------------------- Legacy window ----------------------------
	// ======================================================================
	Canvas cur_canvas;
	public int[] legacy_lockCanvas(int l, int t, int r, int b) {
		CCView surface = (CCView)curView;
		Rect rect = new Rect(l, t, r, b);
		cur_canvas = surface.getHolder().lockCanvas(rect);

		int w = (r - l);
		int h = (b - t);
		return new int[w * h];
	}

	public void legacy_unlockCanvas(int l, int t, int r, int b, int[] buf) {
		CCView surface = (CCView)curView;
		int w = (r - l);
		int h = (b - t);

		// TODO avoid need to swap R/B
		for (int i = 0; i < buf.length; i++)
		{
			int color = buf[i];
			int R = color & 0x00FF0000;
			int B = color & 0x000000FF;

			buf[i] = (color & 0xFF00FF00) | (B << 16) | (R >> 16);
		}

		cur_canvas.drawBitmap(buf, 0, w, l, t, w, h, false, null);
		surface.getHolder().unlockCanvasAndPost(cur_canvas);
		cur_canvas = null;
	}
}
