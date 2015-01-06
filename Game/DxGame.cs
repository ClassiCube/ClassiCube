using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using ClassicalSharp.GraphicsAPI;
using Microsoft.DirectX.Direct3D;
using OpenTK;
using OpenTK.Input;
using FKeys = System.Windows.Forms.Keys;
using Forms = System.Windows.Forms;
using TK = OpenTK;

namespace ClassicalSharp {
	
	public class DxGame : Game {
		
		DxForm window;
		Device device;
		Stopwatch sw;
		
		public DxGame() {
		}

		void WindowPaint(object sender, PaintEventArgs e) {
			device.BeginScene();
			double elapsed = sw.Elapsed.TotalSeconds;
			sw.Reset();
			sw.Start();
			OnRenderFrame( elapsed );
		}
		
		protected override void SwapBuffers() {
			device.EndScene();
			device.Present();
		}

		void WindowResize(object sender, EventArgs e) {
			OnResize();
		}

		void WindowLoad( object sender, EventArgs e ) {
			window.Resize += WindowResize;
			window.Paint += WindowPaint;
			MakeKeyHandlerHack();
			MakeKeyMap();
			InitGraphics();
			Graphics = new DirectXApi( device );
			Load();
			sw = System.Diagnostics.Stopwatch.StartNew();
		}
		
		public void InitGraphics() {
			PresentParameters presentParams = new PresentParameters();
			presentParams.Windowed = true;
			presentParams.SwapEffect = SwapEffect.Discard;
			presentParams.PresentationInterval = PresentInterval.Default;
			presentParams.EnableAutoDepthStencil = true;
			presentParams.AutoDepthStencilFormat = DepthFormat.D16; // D32 doesn't work
			device = new Device( 0, DeviceType.Hardware, window, CreateFlags.HardwareVertexProcessing, presentParams );
			device.RenderState.ColorVertex = false;
			device.RenderState.Lighting = false;
			device.RenderState.CullMode = Cull.None;
		}
		
		class DxForm : Form {
			
			public DxForm( DxGame game ) : base() {
				//SetStyle( ControlStyles.UserPaint, true );
				//SetStyle( ControlStyles.
				//SetStyle( ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.Opaque, true );
				game.window = this;
				Load += game.WindowLoad;
			}
		}
		
		public override Rectangle Bounds {
			get { return window.Bounds; }
		}
		
		public override Size ClientSize {
			get { return window.ClientSize; }
		}
		
		public override bool Focused {
			get { return window.Focused; }
		}
		
		public override int Width {
			get { return window.Width; }
		}
		
		public override int Height {
			get { return window.Height; }
		}
		
		public override string Title {
			get { return window.Text; }
			set { window.Text = value; }
		}
		
		public override WindowState WindowState {
			get {
				bool windowed = device.PresentationParameters.Windowed;
				return windowed ? WindowState.Normal : WindowState.Fullscreen;
			}
			set {
				bool windowed = value == WindowState.Normal;
				device.PresentationParameters.Windowed = windowed;
			}
		}

		public override VSyncMode VSync {
			get {
				PresentInterval interval = device.PresentationParameters.PresentationInterval;
				return interval == PresentInterval.Default ? VSyncMode.On : VSyncMode.Off;
			}
			set {
				PresentInterval interval = value == VSyncMode.On ?  PresentInterval.Default : PresentInterval.Immediate;
				device.PresentationParameters.PresentationInterval = interval;
			}
		}
		
		[DllImport( "user32.dll", CallingConvention = CallingConvention.StdCall )]
		static extern ushort GetKeyState(int nVirtKey);
		
		public override bool IsKeyDown( Key key ) {
			FKeys formsKey;
			if( keyToKeys.TryGetValue( key, out formsKey ) ) {
				ushort flags = GetKeyState( (int)formsKey );
				return ( flags & 0x8000 ) != 0;
			}
			return false;
		}
		
		public override bool IsMousePressed( MouseButton button ) {
			MouseButtons pressed = Control.MouseButtons;
			if( button == MouseButton.Left ) return ( pressed & MouseButtons.Left ) != 0;
			if( button == MouseButton.Right ) return ( pressed & MouseButtons.Right ) != 0;
			return false; // TODO: other buttons
		}
		
		bool exit = false;
		public override void Exit() {
			exit = true;
		}
		
		protected override void RegisterInputHandlers() {
			window.KeyDown += WindowKeyDown;
			window.KeyUp += WindowKeyUp;
			window.KeyPress += WindowKeyPress;
			window.MouseWheel += WindowMouseWheel;
			window.MouseDown += WindowMouseDown;
			window.MouseUp += WindowMouseUp;
			window.MouseMove += WindowMouseMove;
		}
		
		KeyboardKeyEventArgs keyArgs;
		Action<Key> SetKey;
		void MakeKeyHandlerHack() {
			// since KeyboardKeyEventArgs doesn't take a constructor with Key and the
			// setter is internal, we have to override this with reflection.
			MethodInfo method = typeof( KeyboardKeyEventArgs ).GetMethod( "set_Key", BindingFlags.Instance | BindingFlags.NonPublic );
			Type type = typeof( Action<Key> );
			keyArgs = new KeyboardKeyEventArgs();
			SetKey = (Action<Key>)Delegate.CreateDelegate(type, keyArgs, method);
		}

		void WindowKeyUp( object sender, KeyEventArgs e ) {
			Key key = GetKey( e.KeyCode );
			if( key == Key.Unknown ) return;
			SetKey( key );
			KeyUpHandler( sender, keyArgs );
		}

		void WindowKeyDown( object sender, KeyEventArgs e ) {
			Key key = GetKey( e.KeyCode );
			if( key == Key.Unknown ) return;
			SetKey( key );
			KeyDownHandler( sender, keyArgs );
		}

		void WindowKeyPress( object sender, Forms.KeyPressEventArgs e ) {
			TK.KeyPressEventArgs args = new TK.KeyPressEventArgs( e.KeyChar );
			KeyPressHandler( sender, args );
		}

		void WindowMouseWheel( object sender, Forms.MouseEventArgs e ) {
			MouseWheelEventArgs args = new MouseWheelEventArgs( e.X, e.Y, 0, (int)( e.Delta / 120f ) );
			MouseWheelChanged( sender, args );
		}

		void WindowMouseDown( object sender, Forms.MouseEventArgs e ) {
			MouseButton tkButton = GetMouseButton( e.Button );
			if( tkButton == MouseButton.LastButton ) return;
			MouseButtonEventArgs args = new MouseButtonEventArgs( e.X, e.Y, tkButton, true );
			MouseButtonUp( sender, args );
		}

		void WindowMouseUp( object sender, Forms.MouseEventArgs e ) {
			MouseButton tkButton = GetMouseButton( e.Button );
			if( tkButton == MouseButton.LastButton ) return;
			MouseButtonEventArgs args = new MouseButtonEventArgs( e.X, e.Y, tkButton, false );
			MouseButtonUp( sender, args );
		}

		void WindowMouseMove( object sender, Forms.MouseEventArgs e ) {
			MouseMoveEventArgs args = new MouseMoveEventArgs( e.X, e.Y, 0, 0 );
			MouseMove( sender, args );
		}
		
		MouseButton GetMouseButton( MouseButtons value) {
			if( value == MouseButtons.Left ) return MouseButton.Left;
			if( value == MouseButtons.Right ) return MouseButton.Right;
			if( value == MouseButtons.Middle ) return MouseButton.Middle;
			return MouseButton.LastButton;
		}
		
		Key GetKey( Keys value ) {
			Key key;
			if( keysToKey.TryGetValue( value, out key ) ) {
				return key;
			}
			return Key.Unknown;
		}
		
		public override void Dispose() {			
			base.Dispose();
			device.Dispose();
			window.Dispose();
		}
		
		public override void Run() {
			Application.EnableVisualStyles();
			Application.Run( new DxForm( this ) );
		}
		
		Dictionary<Key, Keys> keyToKeys = new Dictionary<Key, Keys>();
		Dictionary<Keys, Key> keysToKey = new Dictionary<Keys, Key>();
		void AddMap( Key tkKey, Keys formKey ) {
			keyToKeys.Add( tkKey, formKey );
			keysToKey.Add( formKey, tkKey );
		}
		
		void MakeKeyMap() {
			for( int i = 0; i < 24; i++ ) {
				AddMap( Key.F1 + i, FKeys.F1 + i );
			}
			for( int i = 0; i <= 9; i++ ) {
				AddMap( Key.Number0 + i, FKeys.D0 + i );
				AddMap( Key.Keypad0 + i, FKeys.NumPad0 + i );
			}
			for( int i = 0; i < 26; i++ ) {
				AddMap( Key.A + i, FKeys.A + i );
			}

			AddMap( Key.Tab, FKeys.Tab ); AddMap( Key.CapsLock, FKeys.CapsLock );
			AddMap( Key.LControl, FKeys.LControlKey ); AddMap( Key.LShift, FKeys.LShiftKey );
			AddMap( Key.LAlt, FKeys.LMenu ); AddMap( Key.RControl, FKeys.RControlKey );
			AddMap( Key.RShift, FKeys.RShiftKey ); AddMap( Key.RAlt, FKeys.RMenu );
			AddMap( Key.Space, FKeys.Space ); AddMap( Key.Back, FKeys.Back );
			AddMap( Key.Escape, FKeys.Escape ); AddMap( Key.Enter, FKeys.Enter );
			AddMap( Key.Semicolon, FKeys.Oem1 ); AddMap( Key.Slash, FKeys.Oem2 );
			AddMap( Key.Tilde, FKeys.Oem3 ); AddMap( Key.BracketLeft, FKeys.Oem4 );
			AddMap( Key.BackSlash, FKeys.Oem5 ); AddMap( Key.BracketRight, FKeys.Oem6 );
			AddMap( Key.Quote, FKeys.Oem7 ); AddMap( Key.Plus, FKeys.Oemplus );
			AddMap( Key.Comma, FKeys.Oemcomma ); AddMap( Key.Minus, FKeys.OemMinus );
			AddMap( Key.Period, FKeys.OemPeriod ); AddMap( Key.Home, FKeys.Home );
			AddMap( Key.End, FKeys.End ); AddMap( Key.Delete, FKeys.Delete );
			AddMap( Key.PageUp, FKeys.PageUp ); AddMap( Key.PageDown, FKeys.PageDown );
			AddMap( Key.PrintScreen, FKeys.PrintScreen ); AddMap( Key.Pause, FKeys.Pause );
			AddMap( Key.NumLock, FKeys.NumLock ); AddMap( Key.ScrollLock, FKeys.Scroll );
			AddMap( Key.Clear, FKeys.Clear ); AddMap( Key.Insert, FKeys.Insert );
			AddMap( Key.Sleep, FKeys.Sleep ); AddMap( Key.KeypadDecimal, FKeys.Decimal );
			AddMap( Key.KeypadAdd, FKeys.Add ); AddMap( Key.KeypadSubtract, FKeys.Subtract );
			AddMap( Key.KeypadDivide, FKeys.Divide ); AddMap( Key.KeypadMultiply, FKeys.Multiply );
			AddMap( Key.Up, FKeys.Up ); AddMap( Key.Down, FKeys.Down );
			AddMap( Key.Left, FKeys.Left ); AddMap( Key.Right, FKeys.Right );
		}
	}
}
