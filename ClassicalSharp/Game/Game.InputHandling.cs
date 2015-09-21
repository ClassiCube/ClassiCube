using System;
using System.IO;
using ClassicalSharp.Particles;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {

	public partial class Game : GameWindow {
		
		public bool IsKeyDown( Key key ) {
			return Keyboard[key];
		}
		
		public bool IsKeyDown( KeyMapping mapping ) {
			Key key = Keys[mapping];
			return Keyboard[key];
		}
		
		public bool IsMousePressed( MouseButton button ) {
			return Mouse[button];
		}
		
		void RegisterInputHandlers() {
			Keyboard.KeyDown += KeyDownHandler;
			Keyboard.KeyUp += KeyUpHandler;
			KeyPress += KeyPressHandler;
			Mouse.WheelChanged += MouseWheelChanged;
			Mouse.Move += MouseMove;
			Mouse.ButtonDown += MouseButtonDown;
			Mouse.ButtonUp += MouseButtonUp;
		}

		bool[] buttonsDown = new bool[3];
		void MouseButtonUp( object sender, MouseButtonEventArgs e ) {
			if( activeScreen == null || !activeScreen.HandlesMouseUp( e.X, e.Y, e.Button ) ) {
				if( SendPlayerClick( e.Button ) ) {
					byte targetId = Players.GetClosetPlayer( this );
					ButtonStateChanged( e.Button, false, targetId );
				}
			}
		}
		
		bool SendPlayerClick( MouseButton button ) {
			return Network.UsingPlayerClick && button <= MouseButton.Middle;
		}

		void MouseButtonDown( object sender, MouseButtonEventArgs e ) {
			if( activeScreen == null || !activeScreen.HandlesMouseClick( e.X, e.Y, e.Button ) ) {
				bool left = e.Button == MouseButton.Left;
				bool right = e.Button == MouseButton.Right;
				bool middle = e.Button == MouseButton.Middle;
				PickBlocks( false, left, right, middle );
			} else {
				lastClick = DateTime.UtcNow;
			}
		}

		void MouseMove( object sender, MouseMoveEventArgs e ) {
			if( activeScreen == null || !activeScreen.HandlesMouseMove( e.X, e.Y ) ) {
			}
		}

		void MouseWheelChanged( object sender, MouseWheelEventArgs e ) {
			if( activeScreen == null || !activeScreen.HandlesMouseScroll( e.Delta ) ) {
				if( Camera.MouseZoom( e ) || !CanChangeHeldBlock ) return;
				
				int diff = -e.Delta % Hotbar.Length;
				int newIndex = HeldBlockIndex + diff;
				if( newIndex < 0 ) newIndex += Hotbar.Length;
				if( newIndex >= Hotbar.Length ) newIndex -= Hotbar.Length;
				HeldBlockIndex = newIndex;
			}
		}

		void KeyPressHandler( object sender, KeyPressEventArgs e ) {
			char key = e.KeyChar;
			if( activeScreen == null || !activeScreen.HandlesKeyPress( key ) ) {
			}
		}
		
		void KeyUpHandler( object sender, KeyboardKeyEventArgs e ) {
			Key key = e.Key;
			if( activeScreen == null || !activeScreen.HandlesKeyUp( key ) ) {
			}
		}

		bool screenshotRequested = false;
		static int[] viewDistances = { 16, 32, 64, 128, 256, 512 };
		void KeyDownHandler( object sender, KeyboardKeyEventArgs e ) {
			Key key = e.Key;
			if( key == Key.F4 && ( IsKeyDown( Key.AltLeft ) || IsKeyDown( Key.AltRight ) ) ) {
				Exit();
			} else if( activeScreen == null || !activeScreen.HandlesKeyDown( key ) ) {
				if( !HandleBuiltinKey( key ) ) {
					LocalPlayer.HandleKeyDown( key );
				}
			}
		}
		
		bool HandleBuiltinKey( Key key ) {
			if( key == Keys[KeyMapping.HideGui] ) {
				HideGui = !HideGui;
			} else if( key == Keys[KeyMapping.Screenshot] ) {
				screenshotRequested = true;
			} else if( key == Keys[KeyMapping.Fullscreen] ) {
				WindowState state = WindowState;
				if( state != WindowState.Minimized ) {
					WindowState = state == WindowState.Fullscreen ?
						WindowState.Normal : WindowState.Fullscreen;
				}
			} else if( key == Keys[KeyMapping.ThirdPersonCamera] ) {
				bool useThirdPerson = Camera is FirstPersonCamera;
				SetCamera( useThirdPerson );
			} else if( key == Keys[KeyMapping.VSync] ) {
				Graphics.SetVSync( this, !VSync );
			} else if( key == Keys[KeyMapping.ViewDistance] ) {
				for( int i = 0; i < viewDistances.Length; i++ ) {
					int newDist = viewDistances[i];
					if( newDist > ViewDistance ) {
						SetViewDistance( newDist );
						return true;
					}
				}
				SetViewDistance( viewDistances[0] );
			} else if( key == Keys[KeyMapping.PauseOrExit] && !Map.IsNotLoaded ) {
				SetNewScreen( new PauseScreen( this ) );
			} else if( key == Keys[KeyMapping.OpenInventory] ) {
				SetNewScreen( new BlockSelectScreen( this ) );
			} else {
				return false;
			}
			return true;
		}
		
		DateTime lastClick = DateTime.MinValue;
		void PickBlocks( bool cooldown, bool left, bool right, bool middle ) {
			DateTime now = DateTime.UtcNow;
			double delta = ( now - lastClick ).TotalMilliseconds;
			if( cooldown && delta < 250 ) return; // 4 times per second
			lastClick = now;
			if( Network.UsingPlayerClick && !ScreenLockedInput ) {
				byte targetId = Players.GetClosetPlayer( this );
				ButtonStateChanged( MouseButton.Left, left, targetId );
				ButtonStateChanged( MouseButton.Right, right, targetId );
				ButtonStateChanged( MouseButton.Middle, middle, targetId );
			}
			
			int buttonsDown = ( left ? 1 : 0 ) + ( right ? 1 : 0 ) + ( middle ? 1 : 0 );
			if( !SelectedPos.Valid || buttonsDown > 1 || ScreenLockedInput || HeldBlock == Block.Air ) return;
			
			if( middle ) {
				Vector3I pos = SelectedPos.BlockPos;
				byte block = Map.GetBlock( pos );
				if( block != 0 && ( CanPlace[block] || CanDelete[block] ) ) {
					HeldBlock = (Block)block;
				}
			} else if( left ) {
				Vector3I pos = SelectedPos.BlockPos;
				byte block = Map.GetBlock( pos );
				if( block != 0 && CanDelete[block] ) {
					ParticleManager.BreakBlockEffect( pos, block );
					Network.SendSetBlock( pos.X, pos.Y, pos.Z, false, (byte)HeldBlock );
					UpdateBlock( pos.X, pos.Y, pos.Z, 0 );
				}
			} else if( right ) {
				Vector3I pos = SelectedPos.TranslatedPos;
				if( !Map.IsValidPos( pos ) ) return;
				
				Block block = HeldBlock;
				if( !CanPick( Map.GetBlock( pos ) ) && CanPlace[(int)block] ) {
					Network.SendSetBlock( pos.X, pos.Y, pos.Z, true, (byte)block );
					UpdateBlock( pos.X, pos.Y, pos.Z, (byte)block );
				}
			}
		}
		
		void ButtonStateChanged( MouseButton button, bool pressed, byte targetId ) {
			if( buttonsDown[(int)button] ) {
				Network.SendPlayerClick( button, false, targetId, SelectedPos );
				buttonsDown[(int)button] = false;
			}
			if( pressed ) {
				Network.SendPlayerClick( button, true, targetId, SelectedPos );
				buttonsDown[(int)button] = true;
			}
		}
		
		internal bool CanPick( byte block ) {
			return !(block == 0 || (BlockInfo.IsLiquid( block ) && !(CanPlace[block] && CanDelete[block])));
		}
		
		public KeyMap Keys = new KeyMap();
	}
	
	public enum KeyMapping {
		Forward, Back, Left, Right, Jump, Respawn, SetSpawn, OpenChat,
		SendChat, PauseOrExit, OpenInventory, Screenshot, Fullscreen, VSync,
		ThirdPersonCamera, ViewDistance, Fly, Speed, NoClip, FlyUp,
		FlyDown, PlayerList, HideGui,
	}
	
	public class KeyMap {
		
		public Key this[KeyMapping key] {
			get { return Keys[(int)key]; }
			set { Keys[(int)key] = value; SaveKeyBindings(); }
		}
		
		Key[] Keys;
		bool IsReservedKey( Key key ) {
			return key == Key.Escape || key == Key.Slash || key == Key.BackSpace ||
				( key >= Key.Insert && key <= Key.End ) ||
				( key >= Key.Up && key <= Key.Right ) || // chat screen movement
				( key >= Key.Number0 && key <= Key.Number9 ); // block hotbar
		}
		
		public bool IsKeyOkay( Key key, out string reason ) {
			if( IsReservedKey( key ) ) {
				reason = "Given key is reserved for gui";
				return false;
			}
			reason = null;
			return true;
		}
		
		public KeyMap() {
			// See comment in Game() constructor
			#if !__MonoCS__
			Keys = new Key[] {
				Key.W, Key.S, Key.A, Key.D, Key.Space, Key.R, Key.Y, Key.T,
				Key.Enter, Key.Escape, Key.B, Key.F12, Key.F11, Key.F7,
				Key.F5, Key.F, Key.Z, Key.ShiftLeft, Key.X, Key.Q,
				Key.E, Key.Tab, Key.F1 };
			#else
			Keys = new Key[23];
			Keys[0] = Key.W; Keys[1] = Key.S; Keys[2] = Key.A; Keys[3] = Key.D;
			Keys[4] = Key.Space; Keys[5] = Key.R; Keys[6] = Key.Y; Keys[7] = Key.T;
			Keys[8] = Key.Enter; Keys[9] = Key.Escape; Keys[10] = Key.B;
			Keys[11] = Key.F12; Keys[12] = Key.F11; Keys[13] = Key.F7;
			Keys[14] = Key.F5; Keys[15] = Key.F; Keys[16] = Key.Z;
			Keys[17] = Key.ShiftLeft; Keys[18] = Key.X; Keys[19] = Key.Q;
			Keys[20] = Key.E; Keys[21] = Key.Tab; Keys[22] = Key.F1;
			#endif
			LoadKeyBindings();
		}
		
		void LoadKeyBindings() {
			try {
				Options.Load();
			} catch( IOException ) {
				Utils.LogWarning( "Unable to load options.txt" );
				return;
			}
			string[] names = KeyMapping.GetNames( typeof( KeyMapping ) );
			for( int i = 0; i < names.Length; i++ ) {
				string key = "key-" + names[i];
				string value = Options.Get( key );
				if( value == null ) {
					Options.Set( key, Keys[i].ToString() );
					continue;
				}
				
				Key mapping;
				try {
					mapping = (Key)Enum.Parse( typeof( Key ), value, true );
				} catch( ArgumentException ) {
					Options.Set( key, Keys[i].ToString() );
					continue;
				}
				if( !IsReservedKey( mapping ) )
					Keys[i] = mapping;
			}
		}
		
		void SaveKeyBindings() {
			string[] names = KeyMapping.GetNames( typeof( KeyMapping ) );
			for( int i = 0; i < names.Length; i++ ) {
				Options.Set( "key-" + names[i], Keys[i].ToString() );
			}
			try {
				Options.Save();
			} catch( IOException ) {
				Utils.LogWarning( "Unable to sace options.txt" );
			}
		}
	}
}