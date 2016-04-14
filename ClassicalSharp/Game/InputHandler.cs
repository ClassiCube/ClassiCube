// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Gui;
using ClassicalSharp.Hotkeys;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {

	public sealed class InputHandler {
		
		public HotkeyList Hotkeys;
		Game game;
		bool[] buttonsDown = new bool[3];
		PickingHandler picking;
		public InputHandler( Game game ) {
			this.game = game;
			RegisterInputHandlers();
			Keys = new KeyMap();
			Hotkeys = new HotkeyList();
			Hotkeys.LoadSavedHotkeys();
			picking = new PickingHandler( game, this );
		}
		
		void RegisterInputHandlers() {
			game.Keyboard.KeyDown += KeyDownHandler;
			game.Keyboard.KeyUp += KeyUpHandler;
			game.window.KeyPress += KeyPressHandler;
			game.Mouse.WheelChanged += MouseWheelChanged;
			game.Mouse.Move += MouseMove;
			game.Mouse.ButtonDown += MouseButtonDown;
			game.Mouse.ButtonUp += MouseButtonUp;
		}
		
		public KeyMap Keys;
		public bool IsKeyDown( Key key ) {
			return game.Keyboard[key];
		}
		
		/// <summary> Returns whether the key associated with the given key binding is currently held down. </summary>
		public bool IsKeyDown( KeyBinding binding ) {
			Key key = Keys[binding];
			return game.Keyboard[key];
		}
		
		public bool IsMousePressed( MouseButton button ) {
			bool down = game.Mouse[button];
			if( down ) return true;
			
			// Key --> mouse mappings
			if( button == MouseButton.Left && IsKeyDown( KeyBinding.MouseLeft ) ) return true;
			if( button == MouseButton.Middle && IsKeyDown( KeyBinding.MouseMiddle ) ) return true;
			if( button == MouseButton.Right && IsKeyDown( KeyBinding.MouseRight ) ) return true;
			return false;
		}
		
		public void PickBlocks( bool cooldown, bool left, bool middle, bool right ) {
			picking.PickBlocks( cooldown, left, middle, right );
		}
		
		internal void ButtonStateChanged( MouseButton button, bool pressed, byte targetId ) {
			if( buttonsDown[(int)button] ) {
				game.Network.SendPlayerClick( button, false, targetId, game.SelectedPos );
				buttonsDown[(int)button] = false;
			}
			if( pressed ) {
				game.Network.SendPlayerClick( button, true, targetId, game.SelectedPos );
				buttonsDown[(int)button] = true;
			}
		}
		
		internal void ScreenChanged( Screen oldScreen, Screen newScreen ) {
			if( oldScreen != null && oldScreen.HandlesAllInput )
				picking.lastClick = DateTime.UtcNow;
			
			if( game.Network.UsingPlayerClick ) {
				byte targetId = game.Players.GetClosetPlayer( game.LocalPlayer );
				ButtonStateChanged( MouseButton.Left, false, targetId );
				ButtonStateChanged( MouseButton.Right, false, targetId );
				ButtonStateChanged( MouseButton.Middle, false, targetId );
			}
		}
		
		
		#region Event handlers
		
		void MouseButtonUp( object sender, MouseButtonEventArgs e ) {
			if( !game.GetActiveScreen.HandlesMouseUp( e.X, e.Y, e.Button ) ) {
				if( game.Network.UsingPlayerClick && e.Button <= MouseButton.Middle ) {
					byte targetId = game.Players.GetClosetPlayer( game.LocalPlayer );
					ButtonStateChanged( e.Button, false, targetId );
				}
			}
		}

		void MouseButtonDown( object sender, MouseButtonEventArgs e ) {
			if( !game.GetActiveScreen.HandlesMouseClick( e.X, e.Y, e.Button ) ) {
				bool left = e.Button == MouseButton.Left;
				bool middle = e.Button == MouseButton.Middle;
				bool right = e.Button == MouseButton.Right;
				PickBlocks( false, left, middle, right );
			} else {
				picking.lastClick = DateTime.UtcNow;
			}
		}

		void MouseMove( object sender, MouseMoveEventArgs e ) {
			if( !game.GetActiveScreen.HandlesMouseMove( e.X, e.Y ) ) {
			}
		}

		float deltaAcc = 0;
		void MouseWheelChanged( object sender, MouseWheelEventArgs e ) {
			if( game.GetActiveScreen.HandlesMouseScroll( e.Delta ) ) return;
			
			Inventory inv = game.Inventory;
			bool hotbar = IsKeyDown( Key.AltLeft ) || IsKeyDown( Key.AltRight );
			if( (!hotbar && game.Camera.DoZoom( e.DeltaPrecise )) || DoFovZoom( e.DeltaPrecise ) 
			   || !inv.CanChangeHeldBlock )
				return;
			ScrollHotbar( e.DeltaPrecise );
		}
		
		void ScrollHotbar( float deltaPrecise ) {
			// Some mice may use deltas of say (0.2, 0.2, 0.2, 0.2, 0.2)
			// We must use rounding at final step, not at every intermediate step.
			Inventory inv = game.Inventory;
			deltaAcc += deltaPrecise;
			int delta = (int)deltaAcc;
			deltaAcc -= delta;
			
			int diff = -delta % inv.Hotbar.Length;
			int newIndex = inv.HeldBlockIndex + diff;
			if( newIndex < 0 ) newIndex += inv.Hotbar.Length;
			if( newIndex >= inv.Hotbar.Length ) newIndex -= inv.Hotbar.Length;
			inv.HeldBlockIndex = newIndex;
		}

		void KeyPressHandler( object sender, KeyPressEventArgs e ) {
			char key = e.KeyChar;
			if( !game.GetActiveScreen.HandlesKeyPress( key ) ) {
			}
		}
		
		void KeyUpHandler( object sender, KeyboardKeyEventArgs e ) {
			Key key = e.Key;
			if( SimulateMouse( key, false ) ) return;
			
			if( !game.GetActiveScreen.HandlesKeyUp( key ) ) {
				if( key == Keys[KeyBinding.ZoomScrolling] )
					SetFOV( game.DefaultFov, false );
			}
		}

		static int[] viewDistances = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };
		Key lastKey;
		void KeyDownHandler( object sender, KeyboardKeyEventArgs e ) {
			Key key = e.Key;
			if( SimulateMouse( key, true ) ) return;
			
			if( IsShutdown( key ) ) {
				game.Exit();
			} else if( key == Keys[KeyBinding.Screenshot] ) {
				game.screenshotRequested = true;
			} else if( !game.GetActiveScreen.HandlesKeyDown( key ) ) {
				if( !HandleBuiltinKey( key ) && !game.LocalPlayer.HandleKeyDown( key ) )
					HandleHotkey( key );
			}
			lastKey = key;
		}
		
		bool IsShutdown( Key key ) {
			if( key == Key.F4 && (lastKey == Key.AltLeft || lastKey == Key.AltRight) )
				return true;
			// On OSX, Cmd+Q should also terminate the process.
			if( !OpenTK.Configuration.RunningOnMacOS ) return false;
			return key == Key.Q && (lastKey == Key.WinLeft || lastKey == Key.WinRight);
		}
		
		void HandleHotkey( Key key ) {
			string text;
			bool more;
			if( !Hotkeys.IsHotkey( key, game.Keyboard, out text, out more ) ) return;
			
			if( !more )
				game.Network.SendChat( text, false );
			else if( game.activeScreen == null )
				game.hudScreen.OpenTextInputBar( text );
		}
		
		MouseButtonEventArgs simArgs = new MouseButtonEventArgs();
		bool SimulateMouse( Key key, bool pressed ) {
			Key left = Keys[KeyBinding.MouseLeft], middle = Keys[KeyBinding.MouseMiddle],
			right = Keys[KeyBinding.MouseRight];
			
			if( !(key == left || key == middle || key == right ) )
				return false;
			simArgs.Button = key == left ? MouseButton.Left :
				key == middle ? MouseButton.Middle : MouseButton.Right;
			simArgs.X = game.Mouse.X;
			simArgs.Y = game.Mouse.Y;
			simArgs.IsPressed = pressed;
			
			if( pressed ) MouseButtonDown( null, simArgs );
			else MouseButtonUp( null, simArgs );
			return true;
		}
		
		bool HandleBuiltinKey( Key key ) {
			if( key == Keys[KeyBinding.HideGui] ) {
				game.HideGui = !game.HideGui;
			} else if( key == Keys[KeyBinding.HideFps] ) {
				game.ShowFPS = !game.ShowFPS;
			} else if( key == Keys[KeyBinding.Fullscreen] ) {
				WindowState state = game.window.WindowState;
				if( state != WindowState.Minimized ) {
					game.window.WindowState = state == WindowState.Fullscreen ?
						WindowState.Normal : WindowState.Fullscreen;
				}
			} else if( key == Keys[KeyBinding.ShowAxisLines] ) {
				game.ShowAxisLines = !game.ShowAxisLines;
			} else if( key == Keys[KeyBinding.ThirdPersonCamera] ) {
				game.CycleCamera();
			} else if( key == Keys[KeyBinding.ViewDistance] ) {
				if( game.IsKeyDown( Key.ShiftLeft ) || game.IsKeyDown( Key.ShiftRight ) ) {
					CycleDistanceBackwards();
				} else {
					CycleDistanceForwards();
				}
			} else if( key == Keys[KeyBinding.PauseOrExit] && !game.World.IsNotLoaded ) {
				game.SetNewScreen( new PauseScreen( game ) );
			} else if( key == Keys[KeyBinding.OpenInventory] ) {
				game.SetNewScreen( new InventoryScreen( game ) );
			} else if( key == Key.F9 ) {
				game.ShowClock = !game.ShowClock;
			} else {
				return false;
			}
			return true;
		}
		
		void CycleDistanceForwards() {
			for( int i = 0; i < viewDistances.Length; i++ ) {
				int dist = viewDistances[i];
				if( dist > game.UserViewDistance ) {
					game.SetViewDistance( dist, true ); return;
				}
			}
			game.SetViewDistance( viewDistances[0], true );
		}
		
		void CycleDistanceBackwards() {
			for( int i = viewDistances.Length - 1; i >= 0; i-- ) {
				int dist = viewDistances[i];
				if( dist < game.UserViewDistance ) {
					game.SetViewDistance( dist, true ); return;
				}
			}
			game.SetViewDistance( viewDistances[viewDistances.Length - 1], true );
		}
		
		float fovIndex = -1;
		bool DoFovZoom( float deltaPrecise ) {
			if( !game.IsKeyDown( KeyBinding.ZoomScrolling ) ) return false;
			LocalPlayer p = game.LocalPlayer;
			if( !p.Hacks.Enabled || !p.Hacks.CanAnyHacks || !p.Hacks.CanUseThirdPersonCamera )
				return false;
			
			if( fovIndex == -1 ) fovIndex = game.ZoomFov;
			fovIndex -= deltaPrecise * 5;
			
			Utils.Clamp( ref fovIndex, 1, game.DefaultFov );
			return SetFOV( (int)fovIndex, true );
		}
		
		public bool SetFOV( int fov, bool setZoom ) {
			LocalPlayer p = game.LocalPlayer;
			if( game.Fov == fov ) return true;
			if( !p.Hacks.Enabled || !p.Hacks.CanAnyHacks || !p.Hacks.CanUseThirdPersonCamera )
				return false;
			
			game.Fov = fov;
			if( setZoom ) game.ZoomFov = fov;
			game.UpdateProjection();
			return true;
		}
		#endregion
	}
}