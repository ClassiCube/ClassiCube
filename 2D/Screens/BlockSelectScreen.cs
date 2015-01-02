using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class BlockSelectScreen : Screen {
		
		public BlockSelectScreen( Game window ) : base( window ) {
		}
		
		class BlockDrawInfo {
			public Texture Texture;
			public Block BlockId;
			
			public BlockDrawInfo( Texture texture, Block block ) {
				Texture = texture;
				BlockId = block;
			}
			
			public int X1 {
				get { return Texture.X1; }
			}
			
			public int Y1 {
				get { return Texture.Y1; }
			}
		}
		BlockDrawInfo[] blocksTable;
		Texture selectedBlock, blockInfoTexture;
		int blockSize = 48;
		int selectedIndex = 0;
		const int blocksPerRow = 10;
		int rows;
		int startX, startY;
		
		public override void Render( double delta ) {
			GraphicsApi.Texturing = true;
			GraphicsApi.Bind2DTexture( Window.TerrainAtlasTexId );
			
			for( int i = 0; i < blocksTable.Length; i++ ) {
				Texture texture = blocksTable[i].Texture;
				texture.RenderNoBind( GraphicsApi );
			}
			if( selectedIndex != -1 ) {
				int rowIndex = selectedIndex % blocksPerRow;
				int colIndex = selectedIndex / blocksPerRow;
				selectedBlock.X1 = startX + blockSize * rowIndex;
				selectedBlock.Y1 = startY + blockSize * colIndex;
				selectedBlock.Render( GraphicsApi );
			}
			if( blockInfoTexture.IsValid ) {
				blockInfoTexture.Render( GraphicsApi );
			}
			GraphicsApi.Texturing = false;
		}
		
		public override void Dispose() {
			GraphicsApi.DeleteTexture( ref selectedBlock );
			GraphicsApi.DeleteTexture( ref blockInfoTexture );
			Window.BlockPermissionsChanged -= BlockPermissionsChanged;
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			int yDiff = ( height - oldHeight ) / 2;
			int xDiff = ( width - oldWidth ) / 2;
			selectedBlock.Y1 += yDiff;
			selectedBlock.X1 += xDiff;
			startX += xDiff;
			startY += yDiff;
			blockInfoTexture.X1 += xDiff;
			blockInfoTexture.Y1 += yDiff;
			for( int i = 0; i < blocksTable.Length; i++ ) {
				Texture texture = blocksTable[i].Texture;
				texture.X1 += xDiff;
				texture.Y1 += yDiff;
				blocksTable[i].Texture = texture;
			}
		}
		
		public override void Init() {
			Window.BlockPermissionsChanged += BlockPermissionsChanged;
			using( Bitmap bmp = new Bitmap( blockSize, blockSize ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					using( Pen pen = new Pen( Color.White, blockSize / 8 ) ) {
						g.DrawRectangle( pen, 0, 0, blockSize, blockSize );
					}
				}
				selectedBlock = Utils2D.Make2DTexture( GraphicsApi, bmp, 0, 0 );
			}
			RecreateBlockTextures();
		}

		void BlockPermissionsChanged( object sender, EventArgs e ) {
			RecreateBlockTextures();
			if( selectedIndex >= blocksTable.Length ) {
				selectedIndex = blocksTable.Length - 1;
			}
			RecreateBlockInfoTexture();
		}
		
		Color backColour = Color.FromArgb( 120, 60, 60, 60 );
		
		string GetBlockInfo( Block block ) {
			return String.Format(
				"&f{0} (can place: {1}&f, can delete: {2}&f)",
				Utils.GetSpacedBlockName( block ),
				Window.CanPlace[(int)block] ? "&aYes" : "&cNo",
				Window.CanDelete[(int)block] ? "&aYes" : "&cNo" );
		}
		
		void RecreateBlockInfoTexture() {
			GraphicsApi.DeleteTexture( ref blockInfoTexture );
			if( selectedIndex == -1 ) return;
			
			Block block = blocksTable[selectedIndex].BlockId;
			string text = GetBlockInfo( block );
			List<DrawTextArgs> parts = Utils.SplitText( GraphicsApi, text, true );
			Size size = Utils2D.MeasureSize( parts, "Arial", 13, true );
			int x = startX + ( blockSize * blocksPerRow ) / 2 - size.Width / 2;
			int y = startY - size.Height;
			
			using( Bitmap bmp = new Bitmap( size.Width, size.Height ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					Utils2D.DrawRect( g, backColour, bmp.Width, bmp.Height );
					Utils2D.DrawText( g, parts, "Arial", 13 );
				}
				blockInfoTexture = Utils2D.Make2DTexture( GraphicsApi, bmp, x, y );
			}
		}
		
		void RecreateBlockTextures() {
			int blocksCount = 0;
			for( int i = 0; i < Window.CanPlace.Length; i++ ) {
				if( Window.CanPlace[i] || Window.CanDelete[i] ) {
					blocksCount++;
				}
			}
			
			rows = blocksCount / blocksPerRow + ( blocksCount % blocksPerRow != 0 ? 1 : 0 );
			startX = Window.Width / 2 - ( blockSize * blocksPerRow ) / 2;
			startY = Window.Height / 2 - ( rows * blockSize ) / 2;
			int x = startX, y = startY;
			blocksTable = new BlockDrawInfo[blocksCount];
			
			int tableIndex = 0;
			for( int i = 0; i < Window.CanPlace.Length; i++ ) {
				if( Window.CanPlace[i] || Window.CanDelete[i] ) {
					Block block = (Block)i;
					int texId = Window.BlockInfo.GetOptimTextureLoc( (byte)block, TileSide.Left );
					TextureRectangle rec = Window.TerrainAtlas.GetTexRec( texId );
					int verSize = blockSize;
					float height = Window.BlockInfo.BlockHeight( (byte)block );
					int blockY = y;
					if( height != 1 ) {
						rec.V1 = rec.V1 + Window.TerrainAtlas.invVerElementSize * height;
						verSize = (int)( blockSize * height );
						blockY = y + blockSize - verSize;
					}
					Texture texture = new Texture( -1, x, blockY, blockSize, verSize, rec );
					
					blocksTable[tableIndex++] = new BlockDrawInfo( texture, block );
					x += blockSize;
					if( tableIndex % blocksPerRow == 0 ) { // end of row, start next one.
						x = startX;
						y += blockSize;
					}
				}
			}
		}
		
		void MovePointerUp() {
			if( selectedIndex == -1 ) return;
			int colIndex = selectedIndex / blocksPerRow;
			if( colIndex == 0 ) return;
			selectedIndex -= blocksPerRow;
			RecreateBlockInfoTexture();
		}
		
		void MovePointerDown() {
			if( selectedIndex == -1 ) return;
			selectedIndex += blocksPerRow;
			if( selectedIndex >= blocksTable.Length ) {
				selectedIndex -= blocksPerRow;
			}
			RecreateBlockInfoTexture();
		}
		
		void MovePointerLeft() {
			if( selectedIndex == -1 ) return;
			int rowIndex = selectedIndex % blocksPerRow;
			if( rowIndex == 0 ) return;
			selectedIndex--;
			RecreateBlockInfoTexture();
		}
		
		void MovePointerRight() {
			if( selectedIndex == -1 ) return;
			int rowIndex = selectedIndex % blocksPerRow;
			if( rowIndex == blocksPerRow - 1 ) return;
			selectedIndex++;
			if( selectedIndex >= blocksTable.Length ) {
				selectedIndex = blocksTable.Length - 1;
			}
			RecreateBlockInfoTexture();
		}
		
		public override bool HandlesAllInput {
			get { return true; }
		}
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			selectedIndex = -1;
			if( Utils2D.Contains( startX, startY, blocksPerRow * blockSize, rows * blockSize, mouseX, mouseY ) ) {
				// clicking on the table. now find the intersecting cell.
				for( int i = 0; i < blocksTable.Length; i++ ) {
					int rowIndex = i % blocksPerRow;
					int colIndex = i / blocksPerRow;
					int x = startX + blockSize * rowIndex;
					int y = startY + blockSize * colIndex;
					
					if( Utils2D.Contains( x, y, blockSize, blockSize, mouseX, mouseY ) ) {
						selectedIndex = i;
						break;
					}
				}
			}
			RecreateBlockInfoTexture();
			return true;
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( button == MouseButton.Left ) {
				if( selectedIndex != -1 ) {
					BlockDrawInfo info = blocksTable[selectedIndex];
					try {
						Window.HeldBlock = info.BlockId;
					} catch( InvalidOperationException ) {
						Window.AddChat( "&e/client: &cThe server has forbidden you from changing your held block." );
					}
				}
				Window.SetNewScreen( new NormalScreen( Window ) );
			}
			return true;
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Window.Keys[KeyMapping.PauseOrExit] ) {
				Window.SetNewScreen( new NormalScreen( Window ) );
			} else if( key == Key.Left ) {
				MovePointerLeft();
			} else if( key == Key.Right ) {
				MovePointerRight();
			} else if( key == Key.Up ) {
				MovePointerUp();
			} else if( key == Key.Down ) {
				MovePointerDown();
			}
			return true;
		}
	}
}
