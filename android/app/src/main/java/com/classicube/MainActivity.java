package com.classicube;
import java.io.File;
import java.io.InputStream;
import java.lang.reflect.Method;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
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
import android.graphics.PixelFormat;
import android.net.Uri;
import android.os.Bundle;
import android.os.StrictMode;
import android.provider.Settings.Secure;
import android.text.Editable;
import android.text.InputType;
import android.text.Selection;
import android.text.SpannableStringBuilder;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.InputQueue;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.view.View;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.view.Window;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethod;
import android.view.inputmethod.InputMethodManager;

// This class contains all the glue/interop code for bridging ClassiCube to the java Android world.
// Some functionality is only available on later Android versions - try {} catch {} is used in such places 
//   to ensure that the game can still run on earlier Android versions (albeit with reduced functionality)
// Currently the minimum required API level to run the game is level 9 (Android 2.3). 
// When using Android functionality, always aim to add a comment with the API level that the functionality 
//   was added in, as this will make things easier if the minimum required API level is ever changed again

// implements InputQueue.Callback
public class MainActivity extends Activity {
	
	// ======================================
	// -------------- COMMANDS --------------
	// ======================================
	//  The main thread (which receives events) is separate from the game thread (which processes events)
	//  Therefore pushing/pulling events must be thread-safe, which is achieved through ConcurrentLinkedQueue
	//  Additionally, a cache is used (freeCmds) to avoid constantly allocating NativeCmdArgs instances
	class NativeCmdArgs { public int cmd, arg1, arg2, arg3, arg4; public String str; public Surface sur; }
	Queue<NativeCmdArgs> pending  = new ConcurrentLinkedQueue<NativeCmdArgs>();
	Queue<NativeCmdArgs> freeCmds = new ConcurrentLinkedQueue<NativeCmdArgs>();
	
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
	
	final static int CMD_POINTER_DOWN = 3;
	final static int CMD_POINTER_UP   = 4;
	final static int CMD_KEY_UP   = 1;
	final static int CMD_KEY_CHAR = 2;
	final static int CMD_KEY_TEXT = 19;
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
	
	// ======================================
	// --------------- EVENTS ---------------
	// ======================================
	static boolean gameRunning;
	InputMethodManager input;

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
	
	void HACK_avoidFileUriExposedErrors() {
		// StrictMode - API level 9
		// disableDeathOnFileUriExposure - API level 24 ?????
		try {
			Method m = StrictMode.class.getMethod("disableDeathOnFileUriExposure");
			m.invoke(null);
		}  catch (NoClassDefFoundError ex) {
			ex.printStackTrace();
		} catch (Exception ex) {
			ex.printStackTrace();
		}
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
		// TODO: semaphore for destroyed and surfaceDestroyed

		// avoid FileUriExposed exception when taking screenshots on recent Android versions
		HACK_avoidFileUriExposedErrors();

		if (!gameRunning) startGameAsync();
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
	
	native void runGameAsync();
	
	// ======================================
	// --------------- VIEWS ----------------
	// ======================================
	volatile boolean fullscreen;
	final Semaphore winDestroyedSem = new Semaphore(0, true);
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
			attrs.imeOptions  = EditorInfo.IME_ACTION_GO | EditorInfo.IME_FLAG_NO_EXTRACT_UI;

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
	
	// ======================================
	// -------------- PLATFORM --------------
	// ======================================
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
	
	public String getExternalAppDir() {
		// getExternalFilesDir - API level 8
		return getExternalFilesDir(null).getAbsolutePath();
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

	// ======================================
	// --------------- WINDOW ---------------
	// ======================================
	//  Implements java Android side of the Android Window backend (See Window.c)
	volatile int keyboardType;
	volatile String keyboardText = "";
	// setTitle - API level 1
	public void setWindowTitle(String str) { setTitle(str); }

	public void openKeyboard(String text, int type) {
		// restartInput, showSoftInput - API level 3
		keyboardType = type;
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
		if (keyboardType == 2) return InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD;
		if (keyboardType == 1) return InputType.TYPE_CLASS_NUMBER;
		return InputType.TYPE_CLASS_TEXT;
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
		// Only release when no free permits (otherwise showAlert doesn't block when called)
		if (dialogSem.availablePermits() > 0) return;
		dialogSem.release();
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
			File file = new File(getExternalAppDir() + "/screenshots/" + path);
			Intent intent = new Intent();
			
			intent.setAction(Intent.ACTION_SEND);
			intent.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(file));
			intent.setType("image/png");
			intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
			startActivity(Intent.createChooser(intent, "share via"));
		} catch (Exception ex) {
			return ex.toString();
		}
		return "";
	}

	// ======================================
	// ---------------- HTTP ----------------
	// ======================================
	//  Implements java Android side of the Android HTTP backend (See Http.c)
	HttpURLConnection conn;
	InputStream src;
	byte[] readCache = new byte[8192];

	public int httpInit(String url, String method) {
		try {
			conn = (HttpURLConnection)new URL(url).openConnection();
			conn.setDoInput(true);
			conn.setRequestMethod(method);
			conn.setInstanceFollowRedirects(true);
			return 0;
		} catch (Exception ex) {
			return httpOnError(ex);
		}
	}

	public void httpSetHeader(String name, String value) {
		conn.setRequestProperty(name, value);
	}

	public int httpSetData(byte[] data) {
		try {
			conn.setDoOutput(true);
			conn.getOutputStream().write(data);
			conn.getOutputStream().flush();
			return 0;
		} catch (Exception ex) {
			return httpOnError(ex);
		}
	}

	public int httpPerform() {
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

	void httpFinish() {
		conn = null;
		try {
			src.close();
		} catch (Exception ex) { }
		src = null;
	}

	// TODO: Should we prune this list?
	List<String> errors = new ArrayList<String>();

	int httpOnError(Exception ex) {
		ex.printStackTrace();
		httpFinish();
		errors.add(ex.getMessage());
		return -errors.size(); // don't want 0 as an error code
	}

	public String httpDescribeError(int res) {
		res = -res - 1;
		return res >= 0 && res < errors.size() ? errors.get(res) : null;
	}

	native void httpParseHeader(String header);
	native void httpAppendData(byte[] data, int len);
}
