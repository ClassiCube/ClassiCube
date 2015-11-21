using System;
using OpenTK;
using OpenTK.Input;
using ClassicalSharp.Hotkeys;

namespace ClassicalSharp {

	public sealed class InputHandler {
		
		public HotkeyList Hotkeys;
		Game game;
		public InputHandler( Game game ) {
			this.game = game;
			RegisterInputHandlers();
			LoadMouseToKeyMappings();
			Keys = new KeyMap();
			Hotkeys = new HotkeyList();
			Hotkeys.LoadSavedHotkeys();
		}
		
		void RegisterInputHandlers() {
			game.Keyboard.KeyDown += KeyDownHandler;
			game.Keyboard.KeyUp += KeyUpHandler;
			game.KeyPress += KeyPressHandler;
			game.Mouse.WheelChanged += MouseWheelChanged;
			game.Mouse.Move += MouseMove;
			game.Mouse.ButtonDown += MouseButtonDown;
			game.Mouse.ButtonUp += MouseButtonUp;
		}
		
		Key mapLeft, mapMiddle, mapRight;
		void LoadMouseToKeyMappings() {
			mapLeft = Options.GetEnum( OptionsKey.MouseLeft, Key.Unknown );
			mapMiddle = Options.GetEnum( OptionsKey.MouseMiddle, Key.Unknown );
			mapRight = Options.GetEnum( OptionsKey.MouseRight, Key.Unknown );
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
			if( button == MouseButton.Left && IsKeyDown( mapLeft ) ) return true;
			if( button == MouseButton.Middle && IsKeyDown( mapMiddle ) ) return true;
			if( button == MouseButton.Right && IsKeyDown( mapRight ) ) return true;
			return false;
		}

		bool[] buttonsDown = new bool[3];
		DateTime lastClick = DateTime.MinValue;
		public void PickBlocks( bool cooldown, bool left, bool middle, bool right ) {
			DateTime now = DateTime.UtcNow;
			double delta = (now - lastClick).TotalMilliseconds;
			if( cooldown && delta < 250 ) return; // 4 times per second
			
			lastClick = now;
			Inventory inv = game.Inventory;
			
			if( game.Network.UsingPlayerClick && !game.ScreenLockedInput ) {
				byte targetId = game.Players.GetClosetPlayer( game.LocalPlayer );
				ButtonStateChanged( MouseButton.Left, left, targetId );
				ButtonStateChanged( MouseButton.Right, right, targetId );
				ButtonStateChanged( MouseButton.Middle, middle, targetId );
			}
			
			int buttonsDown = (left ? 1 : 0) + (right ? 1 : 0) + (middle ? 1 : 0);
			if( buttonsDown > 1 || game.ScreenLockedInput || inv.HeldBlock == Block.Air ) return;
			
			// always play animations, even if we aren't picking a block.
			if( left ) game.BlockHandRenderer.SetAnimationClick( true );
			else if( right ) game.BlockHandRenderer.SetAnimationClick( false );
			if( !game.SelectedPos.Valid ) return;
			
			if( middle ) {
				Vector3I pos = game.SelectedPos.BlockPos;
				byte block = 0;
				if( game.Map.IsValidPos( pos ) && (block = game.Map.GetBlock( pos )) != 0
				   && (inv.CanPlace[block] || inv.CanDelete[block]) ) {
					inv.HeldBlock = (Block)block;
				}
			} else if( left ) {
				Vector3I pos = game.SelectedPos.BlockPos;
				byte block = 0;
				if( game.Map.IsValidPos( pos ) && (block = game.Map.GetBlock( pos )) != 0
				   && inv.CanDelete[block] ) {
					game.ParticleManager.BreakBlockEffect( pos, block );
					game.UpdateBlock( pos.X, pos.Y, pos.Z, 0 );
					game.Network.SendSetBlock( pos.X, pos.Y, pos.Z, false, (byte)inv.HeldBlock );					
				}
			} else if( right ) {
				Vector3I pos = game.SelectedPos.TranslatedPos;
				if( !game.Map.IsValidPos( pos ) ) return;
				
				byte block = (byte)inv.HeldBlock;
				if( !game.CanPick( game.Map.GetBlock( pos ) ) && inv.CanPlace[block]
				   && CheckIsFree( game.SelectedPos, block ) ) {
					game.UpdateBlock( pos.X, pos.Y, pos.Z, block );
					game.Network.SendSetBlock( pos.X, pos.Y, pos.Z, true, block );					
				}
			}
		}
		
		bool CheckIsFree( PickedPos selected, byte newBlock ) {
			if( !CannotPassThrough( newBlock ) ) return true;
			if( IntersectsOtherPlayers( selected.TranslatedPos, newBlock ) ) return false;
			
			Vector3I pos = selected.TranslatedPos;
			float height = game.BlockInfo.Height[newBlock];
			BoundingBox blockBB = new BoundingBox( pos.X, pos.Y, pos.Z,
			                                      pos.X + 1, pos.Y + height, pos.Z + 1 );
			BoundingBox localBB = game.LocalPlayer.CollisionBounds;
			
			if( game.LocalPlayer.noClip || !localBB.Intersects( blockBB ) ) return true;
			
			if( game.LocalPlayer.PushbackBlockPlacing ) {
				return PushbackPlace( selected, blockBB );
			} else {
				localBB.Min.Y += 0.25f + Entity.Adjustment;
				if( localBB.Intersects( blockBB ) ) return false;
				
				// Push player up if they are jumping and trying to place a block underneath them.
				Vector3 p = game.LocalPlayer.Position;
				p.Y = pos.Y + height + Entity.Adjustment;
				LocationUpdate update = LocationUpdate.MakePos( p, false );
				game.LocalPlayer.SetLocation( update, false );
				return true;
			}
		}
		
		bool PushbackPlace( PickedPos selected, BoundingBox blockBB ) {
			Vector3 newP = game.LocalPlayer.Position;
			Vector3 oldP = game.LocalPlayer.Position;
			
			// Offset position by the closest face
			if( selected.BlockFace == CpeBlockFace.XMax )
				newP.X = blockBB.Max.X + 0.5f;
			else if( selected.BlockFace == CpeBlockFace.ZMax )
				newP.Z = blockBB.Max.Z + 0.5f;
			else if( selected.BlockFace == CpeBlockFace.XMin )
				newP.X = blockBB.Min.X - 0.5f;
			else if( selected.BlockFace == CpeBlockFace.ZMin )
				newP.Z = blockBB.Min.Z - 0.5f;
			else if( selected.BlockFace == CpeBlockFace.YMax )
				newP.Y = blockBB.Min.Y + 1 + Entity.Adjustment;
			else if( selected.BlockFace == CpeBlockFace.YMin )
				newP.Y = blockBB.Min.Y - game.LocalPlayer.CollisionSize.Y - Entity.Adjustment;
			
			Vector3I newLoc = Vector3I.Floor( newP );
			bool validPos = newLoc.X >= 0 && newLoc.Y >= 0 && newLoc.Z >= 0 &&
				newLoc.X < game.Map.Width && newP.Z < game.Map.Length;
			if( !validPos ) return false;
			
			game.LocalPlayer.Position = newP;
			if( !game.LocalPlayer.noClip && game.LocalPlayer.TouchesAny( CannotPassThrough ) ) {
				game.LocalPlayer.Position = oldP;
				return false;
			}
			
			game.LocalPlayer.Position = oldP;
			LocationUpdate update = LocationUpdate.MakePos( newP, false );
			game.LocalPlayer.SetLocation( update, false );
			return true;
		}
		
		bool CannotPassThrough( byte block ) {
			return game.BlockInfo.CollideType[block] == BlockCollideType.Solid;
		}
		
		bool IntersectsOtherPlayers( Vector3I pos, byte newType ) {
			float height = game.BlockInfo.Height[newType];
			BoundingBox blockBB = new BoundingBox( pos.X, pos.Y, pos.Z,
			                                      pos.X + 1, pos.Y + height, pos.Z + 1 );
			
			for( int id = 0; id < 255; id++ ) {
				Player player = game.Players[id];
				if( player == null ) continue;
				if( player.CollisionBounds.Intersects( blockBB ) ) return true;
			}
			return false;
		}
		
		void ButtonStateChanged( MouseButton button, bool pressed, byte targetId ) {
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
				lastClick = DateTime.UtcNow;
			
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
				lastClick = DateTime.UtcNow;
			}
		}

		void MouseMove( object sender, MouseMoveEventArgs e ) {
			if( !game.GetActiveScreen.HandlesMouseMove( e.X, e.Y ) ) {
			}
		}

		void MouseWheelChanged( object sender, MouseWheelEventArgs e ) {
			if( !game.GetActiveScreen.HandlesMouseScroll( e.Delta ) ) {
				Inventory inv = game.Inventory;
				if( game.Camera.MouseZoom( e ) || !inv.CanChangeHeldBlock ) return;
				
				int diff = -e.Delta % inv.Hotbar.Length;
				int newIndex = inv.HeldBlockIndex + diff;
				if( newIndex < 0 ) newIndex += inv.Hotbar.Length;
				if( newIndex >= inv.Hotbar.Length ) newIndex -= inv.Hotbar.Length;
				inv.HeldBlockIndex = newIndex;
			}
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
			}
		}

		static int[] viewDistances = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };
		void KeyDownHandler( object sender, KeyboardKeyEventArgs e ) {
			Key key = e.Key;
			if( SimulateMouse( key, true ) ) return;
			
			if( key == Key.F4 && (game.IsKeyDown( Key.AltLeft ) || game.IsKeyDown( Key.AltRight )) ) {
				game.Exit();
			} else if( key == Keys[KeyBinding.Screenshot] ) {
				game.screenshotRequested = true;
			} else if( !game.GetActiveScreen.HandlesKeyDown( key ) ) {
				
				if( !HandleBuiltinKey( key ) && !game.LocalPlayer.HandleKeyDown( key ) ) {
					HandleHotkey( key );
				}
			}
		}
		
		void HandleHotkey( Key key ) {			
			string text;
			bool more;
			
			if( Hotkeys.IsHotkey( key, game.Keyboard, out text, out more ) ) {
				if( !more )
					game.Network.SendChat( text, false );
				else if( game.activeScreen == null )
					game.hudScreen.OpenTextInputBar( text );
			}
		}
		
		MouseButtonEventArgs simArgs = new MouseButtonEventArgs();
		bool SimulateMouse( Key key, bool pressed ) {
			if( !(key == mapLeft || key == mapMiddle || key == mapRight ) )
				return false;
			simArgs.Button = key == mapLeft ? MouseButton.Left :
				key == mapMiddle ? MouseButton.Middle : MouseButton.Right;
			simArgs.X = game.Mouse.X;
			simArgs.Y = game.Mouse.Y;
			simArgs.IsPressed = pressed;
			
			if( pressed ) {
				MouseButtonDown( null, simArgs );
			} else {
				MouseButtonUp( null, simArgs );
			}
			return true;
		}
		
		bool HandleBuiltinKey( Key key ) {
			if( key == Keys[KeyBinding.HideGui] ) {
				game.HideGui = !game.HideGui;
			} else if( key == Keys[KeyBinding.HideFps] ) {
				game.ShowFPS = !game.ShowFPS;
			} else if( key == Keys[KeyBinding.Fullscreen] ) {
				WindowState state = game.WindowState;
				if( state != WindowState.Minimized ) {
					game.WindowState = state == WindowState.Fullscreen ?
						WindowState.Normal : WindowState.Fullscreen;
				}
			} else if( key == Keys[KeyBinding.ThirdPersonCamera] ) {
				bool useThirdPerson = !(game.Camera is ForwardThirdPersonCamera);
				game.SetCamera( useThirdPerson );
			} else if( key == Keys[KeyBinding.ViewDistance] ) {
				if( game.IsKeyDown( Key.ShiftLeft ) || game.IsKeyDown( Key.ShiftRight ) ) {
					CycleDistanceBackwards();
				} else {
					CycleDistanceForwards();
				}
			} else if( key == Keys[KeyBinding.PauseOrExit] && !game.Map.IsNotLoaded ) {
				game.SetNewScreen( new PauseScreen( game ) );
			} else if( key == Keys[KeyBinding.OpenInventory] ) {
				game.SetNewScreen( new BlockSelectScreen( game ) );
			} else {
				return false;
			}
			return true;
		}
		
		void CycleDistanceForwards() {
			for( int i = 0; i < viewDistances.Length; i++ ) {
				int dist = viewDistances[i];
				if( dist > game.ViewDistance ) {
					game.SetViewDistance( dist ); return;
				}
			}
			game.SetViewDistance( viewDistances[0] );
		}
		
		void CycleDistanceBackwards() {
			for( int i = viewDistances.Length - 1; i >= 0; i-- ) {
				int dist = viewDistances[i];
				if( dist < game.ViewDistance ) {
					game.SetViewDistance( dist ); return;
				}
			}
			game.SetViewDistance( viewDistances[viewDistances.Length - 1] );
		}
		#endregion
	}
}