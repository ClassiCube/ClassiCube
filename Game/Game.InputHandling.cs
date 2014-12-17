using System;
using ClassicalSharp.Particles;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {

	public partial class Game : GameWindow {
		
		public bool IsKeyDown( Key key ) {
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
			if( key == Key.F12 ) {
				screenshotRequested = true;
			} else if( key == Key.F11 ) {
				WindowState state = WindowState;
				if( state == WindowState.Fullscreen ) {
					WindowState = WindowState.Normal;
				} else if( state == WindowState.Normal || state == WindowState.Maximized ) {
					WindowState = WindowState.Fullscreen;
				}
			} else if( key == Key.F5 ) {
				bool useThirdPerson = Camera is FirstPersonCamera;
				SetCamera( useThirdPerson );
			} else if( key == Key.F7 ) {
				VSync = VSync == VSyncMode.Off ? VSyncMode.On : VSyncMode.Off;
			} else if( key == Key.F6 ) {
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
			} else if( key == Key.Escape && !Map.IsNotLoaded ) {
				if( !( activeScreen is PauseScreen ) ) {
					SetNewScreen( new PauseScreen( this ) );
				}
			} else if( activeScreen == null || !activeScreen.HandlesKeyDown( key ) ) {
				if( key == Key.B ) {
					SetNewScreen( new BlockSelectScreen( this ) );
				} else {
					LocalPlayer.HandleKeyDown( key );
				}
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
					Network.SendSetBlock( pos.X, pos.Y, pos.Z, 0 );
					UpdateBlock( pos.X, pos.Y, pos.Z, 0 );
				}
			}
			if( right ) {
				if( SelectedPos.TranslatedPos == null ) return;
				Vector3I pos = SelectedPos.TranslatedPos.Value;
				Block block = HeldBlock;
				byte oldBlock = 0;
				
				if( Map.IsValidPos( pos ) && CanReplace( oldBlock ) && CanPlace[(int)block] ) {
					Network.SendSetBlock( pos.X, pos.Y, pos.Z, (byte)block );
					UpdateBlock( pos.X, pos.Y, pos.Z, (byte)block );
				}
			}
		}
		
		bool CanReplace( byte block ) {
			return block == 0 || ( !CanPlace[block] && !CanDelete[block] && BlockInfo.IsLiquid( block ) );
		}
	}
}