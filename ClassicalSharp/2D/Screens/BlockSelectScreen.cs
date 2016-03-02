using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public partial class BlockSelectScreen : Screen {
		
		public BlockSelectScreen( Game game ) : base( game ) {
			font = new Font( game.FontName, 13 );
		}
		
		Block[] blocksTable;
		Texture blockInfoTexture;
		const int blocksPerRow = 10, maxRows = 8;
		int selIndex, rows;
		int startX, startY, blockSize;
		float selBlockExpand;
		readonly Font font;
		StringBuffer buffer = new StringBuffer( 128 );
		
		int TableX { get { return startX - 5 - 10; } }
		int TableY { get { return startY - 5 - 30; } }
		int TableWidth { get { return blocksPerRow * blockSize + 10 + 10; } }
		int TableHeight { get { return Math.Min( rows, maxRows ) * blockSize + 10 + 30; } }
		
		static FastColour backCol = new FastColour( 30, 30, 30, 200 );
		public override void Render( double delta ) {
			graphicsApi.Draw2DQuad( TableX, TableY, TableWidth, TableHeight, backCol );
			if( rows > maxRows )
				DrawScrollbar();
			graphicsApi.Texturing = true;
			graphicsApi.SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			
			IsometricBlockDrawer.lastTexId = -1;
			for( int i = 0; i < blocksTable.Length; i++ ) {
				int x, y;
				if( !GetCoords( i, out x, out y ) )
					continue;
				
				// We want to always draw the selected block on top of others
				if( i == selIndex ) continue;
				IsometricBlockDrawer.Draw( game, (byte)blocksTable[i], blockSize * 0.7f / 2f,
				                          x + blockSize / 2, y + blockSize / 2 );
			}
			
			if( selIndex != -1 ) {
				int x, y;
				GetCoords( selIndex, out x, out y );
				IsometricBlockDrawer.lastTexId = -1;
				IsometricBlockDrawer.Draw( game, (byte)blocksTable[selIndex], (blockSize + selBlockExpand) * 0.7f / 2,
				                          x + blockSize / 2, y + blockSize / 2 );
			}
			
			if( blockInfoTexture.IsValid )
				blockInfoTexture.Render( graphicsApi );
			game.hudScreen.RenderHotbar( delta );
			graphicsApi.Texturing = false;
		}
		
		bool GetCoords( int i, out int x, out int y ) {
			int col = i % blocksPerRow;
			int row = i / blocksPerRow;
			x = startX + blockSize * col;
			y = startY + blockSize * row + 3;
			y -= scrollY * blockSize;
			row -= scrollY;
			return row >= 0 && row < maxRows;
		}
		
		public override void Dispose() {
			font.Dispose();
			graphicsApi.DeleteTexture( ref blockInfoTexture );
			game.Events.BlockPermissionsChanged -= BlockPermissionsChanged;
			game.Keyboard.KeyRepeat = false;
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			blockSize = (int)(50 * Math.Sqrt(game.GuiInventoryScale));
			selBlockExpand = (float)(25 * Math.Sqrt(game.GuiInventoryScale));
			
			int rowsUsed = Math.Min( maxRows, rows );
			startX = game.Width / 2 - (blockSize * blocksPerRow) / 2;
			startY = game.Height / 2 - (rowsUsed * blockSize) / 2;
			blockInfoTexture.X1 = startX + (blockSize * blocksPerRow) / 2 - blockInfoTexture.Width / 2;
			blockInfoTexture.Y1 = startY - blockInfoTexture.Height - 5;
		}
		
		public override void Init() {
			blockSize = (int)(50 * Math.Sqrt(game.GuiInventoryScale));
			selBlockExpand = (float)(25 * Math.Sqrt(game.GuiInventoryScale));
			game.Events.BlockPermissionsChanged += BlockPermissionsChanged;
			
			RecreateBlockTable();
			SetBlockTo( game.Inventory.HeldBlock );
			game.Keyboard.KeyRepeat = true;
		}
		
		public void SetBlockTo( Block block ) {
			selIndex = Array.IndexOf<Block>( blocksTable, block );
			scrollY = (selIndex / blocksPerRow) - (maxRows - 1);
			ClampScrollY();
			MoveCursorToSelected();
			RecreateBlockInfoTexture();
		}
		
		void MoveCursorToSelected() {
			if( selIndex == -1 ) return;
			int x, y;
			GetCoords( selIndex, out x, out y );
			x += blockSize / 2; y += blockSize / 2;
			
			Point topLeft = game.PointToScreen( Point.Empty );
			x += topLeft.X; y += topLeft.Y;
			game.DesktopCursorPos = new Point( x, y );
		}

		void BlockPermissionsChanged( object sender, EventArgs e ) {
			RecreateBlockTable();
			if( selIndex >= blocksTable.Length )
				selIndex = blocksTable.Length - 1;
			
			scrollY = selIndex / blocksPerRow;
			ClampScrollY();
			RecreateBlockInfoTexture();
		}
		
		static string[] normalNames = null;
		void UpdateBlockInfoString( Block block ) {
			if( normalNames == null )
				MakeNormalNames();
			
			int index = 0;
			buffer.Clear();
			buffer.Append( ref index, "&f" );
			string value = game.BlockInfo.Name[(byte)block];
			if( (byte)block < BlockInfo.CpeBlocksCount && value == "Invalid" ) {
				buffer.Append( ref index, normalNames[(byte)block] );
			} else {
				buffer.Append( ref index, value );
			}
			buffer.Append( ref index, " (ID: " );
			buffer.AppendNum( ref index, (byte)block );
			buffer.Append( ref index, ", place: " );
			buffer.Append( ref index, game.Inventory.CanPlace[(int)block] ? "&aYes" : "&cNo" );
			buffer.Append( ref index, "&f, delete: " );
			buffer.Append( ref index, game.Inventory.CanDelete[(int)block] ? "&aYes" : "&cNo" );
			buffer.Append( ref index, "&f)" );
		}
		
		void MakeNormalNames() {
			normalNames = new string[BlockInfo.CpeBlocksCount];
			for( int i = 0; i < normalNames.Length; i++ ) {
				string origName = Enum.GetName( typeof(Block), (byte)i );
				buffer.Clear();
				
				if( origName == "TNT") {
					normalNames[i] = "TNT";
					continue;
				}
				int index = 0;
				SplitUppercase( origName, ref index );
				normalNames[i] = buffer.ToString();
			}
		}
		
		void SplitUppercase( string value, ref int index ) {
			for( int i = 0; i < value.Length; i++ ) {
				char c = value[i];
				if( Char.IsUpper( c ) && i > 0 ) {
					buffer.Append( ref index, ' ' );
				}
				buffer.Append( ref index, c );
			}
		}
		
		int lastCreatedIndex = -1000;
		void RecreateBlockInfoTexture() {
			if( selIndex == lastCreatedIndex ) return;
			lastCreatedIndex = selIndex;
			
			graphicsApi.DeleteTexture( ref blockInfoTexture );
			if( selIndex == -1 ) return;
			
			Block block = blocksTable[selIndex];
			UpdateBlockInfoString( block );
			string value = buffer.GetString();
			
			DrawTextArgs args = new DrawTextArgs( value, font, true );
			Size size = game.Drawer2D.MeasureChatSize( ref args );
			int x = startX + (blockSize * blocksPerRow) / 2 - size.Width / 2;
			int y = startY - size.Height - 5;
			
			args.SkipPartsCheck = true;
			blockInfoTexture = game.Drawer2D.MakeChatTextTexture( ref args, x, y );
		}
		
		void RecreateBlockTable() {
			int blocksCount = 0;
			int count = game.UseCPE ? BlockInfo.BlocksCount : BlockInfo.OriginalBlocksCount;
			for( int tile = 1; tile < count; tile++ ) {
				if( ShowTile( tile ) ) blocksCount++;
			}
			
			rows = Utils.CeilDiv( blocksCount, blocksPerRow );
			int rowsUsed = Math.Min( maxRows, rows );
			startX = game.Width / 2 - (blockSize * blocksPerRow) / 2;
			startY = game.Height / 2 - (rowsUsed * blockSize) / 2;
			blocksTable = new Block[blocksCount];
			
			int tableIndex = 0;
			for( int tile = 1; tile < count; tile++ ) {
				if( ShowTile( tile ) ) blocksTable[tableIndex++] = (Block)tile;
			}
		}
		
		bool ShowTile( int tile ) {
			if( game.PureClassicMode && (tile >= (byte)Block.Water && tile <= (byte)Block.StillLava) )
				return false;
			return tile < BlockInfo.CpeBlocksCount || game.BlockInfo.Name[tile] != "Invalid";
		}
		
		public override bool HandlesAllInput { get { return true; } }
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			if( draggingMouse ) {
				ScrollbarClick( mouseY );
				return true;
			}
			
			selIndex = -1;
			if( Contains( startX, startY, blocksPerRow * blockSize,
			             maxRows * blockSize, mouseX, mouseY ) ) {
				for( int i = 0; i < blocksTable.Length; i++ ) {
					int x, y;
					GetCoords( i, out x, out y );
					
					if( Contains( x, y, blockSize, blockSize, mouseX, mouseY ) ) {
						selIndex = i;
						break;
					}
				}
			}
			RecreateBlockInfoTexture();
			return true;
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( draggingMouse || game.hudScreen.hotbar.HandlesMouseClick( mouseX, mouseY, button ) )
				return true;
			if( button == MouseButton.Left && mouseX >= TableX && mouseX < TableX + scrollbarWidth ) {
				ScrollbarClick( mouseY );
				draggingMouse = true;
			} else if( button == MouseButton.Left ) {
				if( selIndex != -1 )
					game.Inventory.HeldBlock = blocksTable[selIndex];
				else if( Contains( TableX, TableY, TableWidth, TableHeight, mouseX, mouseY ) )
					return true;
				game.SetNewScreen( null );
			}
			return true;
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == game.Mapping( KeyBinding.PauseOrExit ) ||
			   key == game.Mapping( KeyBinding.OpenInventory ) ) {
				game.SetNewScreen( null );
			} else if( key == Key.Enter && selIndex != -1 ) {
				game.Inventory.HeldBlock = blocksTable[selIndex];
				game.SetNewScreen( null );
			} else if( (key == Key.Left || key == Key.Keypad4) && selIndex != -1 ) {
				ArrowKeyMove( -1 );
			} else if( (key == Key.Right || key == Key.Keypad6) && selIndex != -1 ) {
				ArrowKeyMove( 1 );
			} else if( (key == Key.Up || key == Key.Keypad8) && selIndex != -1 ) {
				ArrowKeyMove( -blocksPerRow );
			} else if( (key == Key.Down || key == Key.Keypad2) && selIndex != -1 ) {
				ArrowKeyMove( blocksPerRow );
			} else if( key >= Key.Number1 && key <= Key.Number9 ) {
				game.Inventory.HeldBlockIndex = (int)key - (int)Key.Number1;
			}
			return true;
		}
		
		void ArrowKeyMove( int delta ) {
			int startIndex = selIndex;
			selIndex += delta;
			if( selIndex < 0 )
				selIndex -= delta;
			if( selIndex >= blocksTable.Length )
				selIndex -= delta;
			
			int scrollDelta = (selIndex / blocksPerRow) - (startIndex / blocksPerRow);
			scrollY += scrollDelta;
			ClampScrollY();
			RecreateBlockInfoTexture();
			MoveCursorToSelected();
		}
	}
}
