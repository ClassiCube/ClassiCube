package com.classicube;
import java.io.File;
import java.io.InputStream;
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
import android.view.inputmethod.InputMethodManager;

// implements InputQueue.Callback
public class MainActivity extends Activity implements SurfaceHolder.Callback2 {
	LauncherView curView;
	
	// ======================================
	// -------------- COMMANDS --------------
	// ======================================
	class NativeCmdArgs { public int cmd, arg1, arg2, arg3; public Surface sur; }
	Queue<NativeCmdArgs> nativeCmds = new ConcurrentLinkedQueue<NativeCmdArgs>();
	Queue<NativeCmdArgs> freeCmds   = new ConcurrentLinkedQueue<NativeCmdArgs>();
	
	NativeCmdArgs getCmdArgs() {
		NativeCmdArgs args = freeCmds.poll();
		return args != null ? args : new NativeCmdArgs();
	}
	
	void pushCmd(int cmd) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd = cmd;
		nativeCmds.add(args);
	}
	
	void pushCmd(int cmd, int a1) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd  = cmd;
		args.arg1 = a1;
		nativeCmds.add(args);
	}
	
	void pushCmd(int cmd, int a1, int a2, int a3) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd  = cmd;
		args.arg1 = a1;
		args.arg2 = a2;
		args.arg3 = a3;
		nativeCmds.add(args);
	}
	
	void pushCmd(int cmd, Surface surface) {
		NativeCmdArgs args = getCmdArgs();
		args.cmd = cmd;
		args.sur = surface;
		nativeCmds.add(args);
	}
	
	final static int CMD_KEY_DOWN = 0;
	final static int CMD_KEY_UP   = 1;
	final static int CMD_KEY_CHAR = 2;
	
	final static int CMD_MOUSE_DOWN = 3;
	final static int CMD_MOUSE_UP   = 4;
	final static int CMD_MOUSE_MOVE = 5;
	
	final static int CMD_WIN_CREATED   = 6;
	final static int CMD_WIN_DESTROYED = 7;
	final static int CMD_WIN_RESIZED   = 8;
	final static int CMD_WIN_REDRAW	= 9;

	final static int CMD_APP_START   = 10;
	final static int CMD_APP_STOP	= 11;
	final static int CMD_APP_RESUME  = 12;
	final static int CMD_APP_PAUSE   = 13;
	final static int CMD_APP_DESTROY = 14;

	final static int CMD_GOT_FOCUS	  = 15;
	final static int CMD_LOST_FOCUS	 = 16;
	final static int CMD_CONFIG_CHANGED = 17;
	final static int CMD_LOW_MEMORY	 = 18;
	
	// ======================================
	// --------------- EVENTS ---------------
	// ======================================
	static boolean gameHooked;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.i("CC_WIN", "CREATE EVENT");
		Window window = getWindow();
		Log.i("CC_WIN", "GAME RUNNING?" + gameHooked);
		gameHooked = true;
		//window.takeSurface(this);
		//window.takeInputQueue(this);
		// TODO: Should this be RGBA_8888??
		window.setFormat(PixelFormat.RGBX_8888);
		window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_UNSPECIFIED | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		// TODO: semaphore for destroyed and surfaceDestroyed
   
		Log.i("CC_WIN", "handing off to native..");
		System.loadLibrary("classicube");	
		runGameAsync();
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
		int id = event.getPointerId(i);
		// TODO: Pass float to jni
		int x  = (int)event.getX(i);
		int y  = (int)event.getY(i);
		pushCmd(cmd, id, x, y);
	}
	
	boolean handleTouchEvent(MotionEvent event) {
		switch (event.getActionMasked()) {
		case MotionEvent.ACTION_DOWN:
		case MotionEvent.ACTION_POINTER_DOWN:
			pushTouch(CMD_MOUSE_DOWN, event, event.getActionIndex());
			break;
			
		case MotionEvent.ACTION_UP:
		case MotionEvent.ACTION_POINTER_UP:
			pushTouch(CMD_MOUSE_UP, event, event.getActionIndex());
			break;
			
		case MotionEvent.ACTION_MOVE:
			for (int i = 0; i < event.getPointerCount(); i++) {
				pushTouch(CMD_MOUSE_MOVE, event, i);
			}
		}
		return true;
	}
	
	public boolean onKeyDown(int keyCode, KeyEvent event) {
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
	}
	
	@Override
	protected void onResume() {
		attachSurface();
		super.onResume();
		pushCmd(CMD_APP_RESUME); 
	}
	
	@Override
	protected void onPause() {
		// can't use null.. TODO is there a better way?
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
	
	// this is called on the game thread
	public void processEvents() {
		for (;;) {
			NativeCmdArgs c = nativeCmds.poll();
			if (c == null) return;
			
			switch (c.cmd) {
			case CMD_KEY_DOWN: processKeyDown(c.arg1); break;
			case CMD_KEY_UP:   processKeyUp(c.arg1);   break;
			case CMD_KEY_CHAR: processKeyChar(c.arg1); break;
	
			case CMD_MOUSE_DOWN: processMouseDown(c.arg1, c.arg2, c.arg3); break;
			case CMD_MOUSE_UP:   processMouseUp(c.arg1,   c.arg2, c.arg3); break;
			case CMD_MOUSE_MOVE: processMouseMove(c.arg1, c.arg2, c.arg3); break;
	
			case CMD_WIN_CREATED:   processSurfaceCreated(c.sur);   break;
			case CMD_WIN_DESTROYED: processSurfaceDestroyed();	  break;
			case CMD_WIN_RESIZED:   processSurfaceResized(c.sur);   break;
			case CMD_WIN_REDRAW:	processSurfaceRedrawNeeded();   break;

			case CMD_APP_START:   processOnStart();   break;
			case CMD_APP_STOP:	processOnStop();	break;
			case CMD_APP_RESUME:  processOnResume();  break;
			case CMD_APP_PAUSE:   processOnPause();   break;
			case CMD_APP_DESTROY: processOnDestroy(); break;

			case CMD_GOT_FOCUS:	  processOnGotFocus();	  break;
			case CMD_LOST_FOCUS:	 processOnLostFocus();	 break;
			//case CMD_CONFIG_CHANGED: processOnConfigChanged(); break;
			case CMD_LOW_MEMORY:	 processOnLowMemory();	 break;
			}
			
			c.sur = null; // don't keep a reference to it
			freeCmds.add(c);
		}
	}
	
	native void processKeyDown(int code);
	native void processKeyUp(int code);
	native void processKeyChar(int code);
	
	native void processMouseDown(int id, int x, int y);
	native void processMouseUp(int id, int x, int y);
	native void processMouseMove(int id, int x, int y);

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
	public void surfaceCreated(SurfaceHolder holder) {
		Log.i("CC_WIN", "win created " + holder.getSurface());
		pushCmd(CMD_WIN_CREATED, holder.getSurface());
	}
	
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		Log.i("CC_WIN", "win changed " + holder.getSurface());
		pushCmd(CMD_WIN_RESIZED, holder.getSurface());
	}
	
	final Semaphore winDestroyedSem = new Semaphore(0, true);
	public void surfaceDestroyed(SurfaceHolder holder) {
		Log.i("CC_WIN", "win destroyed " + holder.getSurface());
		Log.i("CC_WIN", "cur view " + curView);
		holder.removeCallback(this);
		
		//08-02 21:03:02.967: E/BufferQueueProducer(1350): [SurfaceView - com.classicube.ClassiCube/com.classicube.MainActivity#0] disconnect: not connected (req=2)
		//08-02 21:03:02.968: E/SurfaceFlinger(1350): Failed to find layer (SurfaceView - com.classicube.ClassiCube/com.classicube.MainActivity#0) in layer parent (no-parent).

		pushCmd(CMD_WIN_DESTROYED);
		// per the android docs for SurfaceHolder.Callback
		// "If you have a rendering thread that directly accesses the surface, you must ensure
		// that thread is no longer touching the Surface before returning from this function."
		try {
			winDestroyedSem.acquire();
		} catch (InterruptedException e) { }
	}
	
	// Game calls this on its thread to notify the main thread
	// that it is safe to destroy the window surface now
	public void processedSurfaceDestroyed() {
		winDestroyedSem.release();
	}
	
	public void surfaceRedrawNeeded(SurfaceHolder holder) {
		Log.i("CC_WIN", "win dirty " + holder.getSurface());
		pushCmd(CMD_WIN_REDRAW);
	}
	
	void attachSurface() {
		curView = new LauncherView(this);
		curView.getHolder().addCallback(this);
		curView.getHolder().setFormat(PixelFormat.RGBX_8888);
		
		setContentView(curView);
		curView.requestFocus();
	}
	
	class LauncherView extends SurfaceView {
		
		public LauncherView(Context context) { super(context); }
		
		@Override
		public boolean dispatchTouchEvent(MotionEvent ev) {
			return MainActivity.this.handleTouchEvent(ev) || super.dispatchTouchEvent(ev);
		}
	}
	
	// ======================================
	// -------------- PLATFORM --------------
	// ======================================
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
		Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
		if (intent.resolveActivity(getPackageManager()) != null) {
			startActivity(intent);
		}
	}
	
	public String getExternalAppDir() {
		return getExternalFilesDir(null).getAbsolutePath();
	}
	
	public long getApkUpdateTime() {
		try {
			//String name = getPackageName();
			//ApplicationInfo info = getPackageManager().getApplicationInfo(name, 0);
			ApplicationInfo info = getApplicationInfo();
			File apkFile = new File(info.sourceDir);
			return apkFile.lastModified();
		} catch (Exception ex) {
			return 0;
		}
	}

	// ======================================
	// --------------- WINDOW ---------------
	// ======================================
	public void setWindowTitle(String str) { setTitle(str); }
	
	public void openKeyboard() {
		InputMethodManager input = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
		View view = getWindow().getDecorView();
		input.showSoftInput(view, 0);
	}

	public void closeKeyboard() {
		InputMethodManager input = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
		View view = getWindow().getDecorView();
		input.hideSoftInputFromWindow(view.getWindowToken(), 0);
	}

	public String getClipboardText() {
		ClipboardManager clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
		CharSequence chars = clipboard.getText();
		return chars == null ? null : chars.toString();
	}

	public void setClipboardText(String str) {
		ClipboardManager clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
		clipboard.setText(str);
	}
	
	DisplayMetrics getMetrics() {
		DisplayMetrics dm = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(dm);
		return dm;
	}

	// some devices return raw DPI values such as 159.37254 / 160.42015 which become 0.996 / 1.002
	// so simply round off to two decimal places to produce more useful end result
	float roundDpi(double x) { return Math.round(x * 100) / 100.0f; }

		// map android 160 baseline dpi
	public float getDpiX() { return roundDpi(getMetrics().xdpi / 160.0); }
	public float getDpiY() { return roundDpi(getMetrics().ydpi / 160.0); }

	final Semaphore dialogSem = new Semaphore(0, true);

	public void showAlert(final String title, final String message) {
		//final Activity activity = this;
		runOnUiThread(new Runnable() {
			public void run() {
				AlertDialog.Builder dlg = new AlertDialog.Builder(MainActivity.this, AlertDialog.THEME_HOLO_DARK);
				dlg.setTitle(title);
				dlg.setMessage(message);
				dlg.setPositiveButton("Close", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int id) {
						dialogSem.release();
					}
				});
				dlg.setCancelable(false);
				dlg.create().show();
			}
		});

		// wait for dialog to be closed
		// TODO: this fails because multiple dialog boxes show
	}

	public void enterFullscreen() {
		runOnUiThread(new Runnable() {
			public void run() {
				curView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE);
			}
		});
    }

    public void exitFullscreen() {
		runOnUiThread(new Runnable() {
			public void run() {
				curView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_VISIBLE);
			}
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
			Map<String, List<String>> all = conn.getHeaderFields();

			for (Map.Entry<String, List<String>> h : all.entrySet()) {
				String key = h.getKey();
				for (String value : h.getValue()) {
					if (key == null) {
						httpParseHeader(value);
					} else {
						httpParseHeader(key + ":" + value);
					}
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
