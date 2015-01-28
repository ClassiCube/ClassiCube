using System;
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

		void MouseButtonUp( object sender, MouseButtonEventArgs e ) {
			if( activeScreen == null || !activeScreen.HandlesMouseUp( e.X, e.Y, e.Button ) ) {
				
			}
		}

		void MouseButtonDown( object sender, MouseButtonEventArgs e ) {
			if( activeScreen == null || !activeScreen.HandlesMouseClick( e.X, e.Y, e.Button ) ) {
				bool left = e.Button == MouseButton.Left;
				bool right = e.Button == MouseButton.Right;
				PickBlocks( false, left, right );
			} else {
				lastClick = DateTime.UtcNow;
			}
		}

		void MouseMove( object sender, MouseMoveEventArgs e ) {
			if( activeScreen == null || !activeScreen.HandlesMouseMove( e.X, e.Y ) ) {
				
			}
		}

		void MouseWheelChanged( object sender, MouseWheelEventArgs e ) {
			Camera.MouseZoom( e );
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
			if( key == Keys[KeyMapping.Screenshot] ) {
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
				VSync = VSync == VSyncMode.Off ? VSyncMode.On : VSyncMode.Off;
			} else if( key == Keys[KeyMapping.ViewDistance] ) {
				if( ViewDistance >= viewDistances[viewDistances.Length - 1] ) {
					SetViewDistance( viewDistances[0] );
				} else {
					for( int i = 0; i < viewDistances.Length; i++ ) {
						int newDistance = viewDistances[i];
						if( newDistance > ViewDistance ) {
							SetViewDistance( newDistance );
							break;
						}
					}
				}
			} else if( key == Keys[KeyMapping.PauseOrExit] && !Map.IsNotLoaded ) {
				if( !( activeScreen is PauseScreen ) ) {
					SetNewScreen( new PauseScreen( this ) );
				}
			} else if( activeScreen == null || !activeScreen.HandlesKeyDown( key ) ) {
				LocalPlayer.HandleKeyDown( key );
			}
		}
		
		DateTime lastClick = DateTime.MinValue;
		void PickBlocks( bool cooldown, bool left, bool right ) {
			if( SelectedPos == null || left == right || ScreenLockedInput ) return;
			DateTime now = DateTime.UtcNow;
			double delta = ( now - lastClick ).TotalMilliseconds;
			if( cooldown && delta < 250 ) return; // 4 times per second
			lastClick = now;
			
			if( left ) {
				Vector3I pos = SelectedPos.BlockPos;
				byte block = 0;
				
				if( Map.IsValidPos( pos ) && ( block = Map.GetBlock( pos ) ) != 0 && CanDelete[block] ) {
					ParticleManager.BreakBlockEffect( pos, block );
					Network.SendDeleteBlock( SelectedPos );
					UpdateBlock( pos.X, pos.Y, pos.Z, 0 );
				}
			}
			if( right ) {
				if( SelectedPos.TranslatedPos == null ) return;
				Vector3I pos = SelectedPos.TranslatedPos.Value;
				Block block = Block.Grass; // TODO: Picking
				byte oldBlock = 0;
				
				if( Map.IsValidPos( pos ) && CanReplace( oldBlock ) && CanPlace[(int)block] ) {
					Network.SendPlaceBlock( SelectedPos );
					UpdateBlock( pos.X, pos.Y, pos.Z, (byte)block );
				}
			}
		}
		
		bool CanReplace( byte block ) {
			return block == 0 || ( !CanPlace[block] && !CanDelete[block] && BlockInfo.IsLiquid( block ) );
		}
		
		public KeyMap Keys = new KeyMap();
	}
	
	public class KeyMap {
		
		public Key this[KeyMapping key] {
			get { return Keys[(int)key]; }
			set { Keys[(int)key] = value; }
		}
		
		Key[] Keys = new Key[] {
			Key.W, Key.S, Key.A, Key.D, Key.Space, Key.T,
			Key.Enter, Key.Escape, Key.B, Key.F12, Key.F11, Key.F7,
			Key.F5, Key.F6, Key.Z, Key.LShift, Key.X, Key.Q, Key.E,
			Key.Tab, Key.H,
		};
		
		bool IsReservedKey( Key key ) {
			return key == Key.Slash || key == Key.BackSpace ||
				( key >= Key.Insert && key <= Key.End ) ||
				( key >= Key.Up && key <= Key.Right ) || // chat screen movement
				( key >= Key.Number0 && key <= Key.Number9 ); // block hotbar
		}
		
		public bool IsLockedKey( Key key ) {
			return key == Key.Escape || ( key >= Key.F1 && key <= Key.F35 );
		}
		
		public bool IsKeyOkay( Key key, out string reason ) {
			if( IsReservedKey( key ) ) {
				reason = "Given key is reserved for gui";
				return false;
			}
			for( int i = 0; i < Keys.Length; i++ ) {
				if( Keys[i] == key ) {
					reason = "Key is already assigned";
					return false;
				}
			}
			reason = null;
			return true;
		}
	}
	
	public enum KeyMapping {
		Forward, Back, Left, Right, Jump, OpenChat,
		SendChat, PauseOrExit, OpenInventory, Screenshot, Fullscreen, VSync,
		ThirdPersonCamera, ViewDistance, Fly, Speed, NoClip, FlyUp, FlyDown,
		PlayerList, ChatHistoryMode,
	}
}