using System;
using System.Collections.Generic;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class BlockSelectScreen : Screen {
		
		public BlockSelectScreen( Game window ) : base( window ) {
			font = new Font( "Arial", 13 );
		}
		
		class BlockDrawInfo {
			public Texture Texture;
			public Block BlockId;
			
			public BlockDrawInfo( Texture texture, Block block ) {
				Texture = texture;
				BlockId = block;
			}
		}
		BlockDrawInfo[] blocksTable;
		Texture selectedBlock, blockInfoTexture;
		int blockSize = 48;
		int selectedIndex;
		const int blocksPerRow = 10;
		int rows;
		int startX, startY;
		readonly Font font;
		StringBuffer buffer = new StringBuffer( 96 );
		
		public override void Render( double delta ) {
			graphicsApi.Texturing = true;
			graphicsApi.BindTexture( game.TerrainAtlas.TexId );
			
			for( int i = 0; i < blocksTable.Length; i++ ) {
				Texture texture = blocksTable[i].Texture;
				texture.RenderNoBind( graphicsApi );
			}
			if( selectedIndex != -1 ) {
				int col = selectedIndex % blocksPerRow;
				int row = selectedIndex / blocksPerRow;
				selectedBlock.X1 = startX + blockSize * col;
				selectedBlock.Y1 = startY + blockSize * row;
				selectedBlock.Render( graphicsApi );
			}
			if( blockInfoTexture.IsValid ) {
				blockInfoTexture.Render( graphicsApi );
			}
			graphicsApi.Texturing = false;
		}
		
		public override void Dispose() {
			font.Dispose();
			graphicsApi.DeleteTexture( ref selectedBlock );
			graphicsApi.DeleteTexture( ref blockInfoTexture );
			game.BlockPermissionsChanged -= BlockPermissionsChanged;
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			int yDiff = ( height - oldHeight ) / 2;
			int xDiff = ( width - oldWidth ) / 2;
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
			game.BlockPermissionsChanged += BlockPermissionsChanged;
			Size size = new Size( blockSize, blockSize );
			using( Bitmap bmp = Utils2D.CreatePow2Bitmap( size ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					Utils2D.DrawRectBounds( g, Color.White, blockSize / 8, 0, 0, blockSize, blockSize );
				}
				selectedBlock = Utils2D.Make2DTexture( graphicsApi, bmp, size, 0, 0 );
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
		
		static readonly Color backColour = Color.FromArgb( 120, 60, 60, 60 );
		unsafe void UpdateBlockInfoString( Block block ) {
			fixed( char* ptr = buffer.value ) {
				char* ptr2 = ptr;
				buffer.Clear( ptr );
				buffer.Append( ref ptr2, "&f" );
				if( block == Block.TNT ) {
					buffer.Append( ref ptr2, "TNT" );
				} else {
					string value = game.BlockInfo.GetName( (byte)block );
					if( (byte)block < BlockInfo.CpeBlocksCount ) {
						SplitUppercase( value, ref ptr2 );
					} else {
						buffer.Append( ref ptr2, value );
					}
				}
				buffer.Append( ref ptr2, " (can place: " );
				buffer.Append( ref ptr2, game.CanPlace[(int)block] ? "&aYes" : "&cNo" );
				buffer.Append( ref ptr2, "&f, can delete: " );
				buffer.Append( ref ptr2, game.CanDelete[(int)block] ? "&aYes" : "&cNo" );
				buffer.Append( ref ptr2, "&f)" );
			}
		}		
		
		unsafe void SplitUppercase( string value, ref char* ptr ) {
			for( int i = 0; i < value.Length; i++ ) {
				char c = value[i];
				if( Char.IsUpper( c ) && i > 0 ) {
					buffer.Append( ref ptr, ' ' );
				}
				buffer.Append( ref ptr, c );
			}
		}
		
		int lastCreatedIndex = -1000;
		void RecreateBlockInfoTexture() {
			if( selectedIndex == lastCreatedIndex ) return;
			lastCreatedIndex = selectedIndex;
			
			graphicsApi.DeleteTexture( ref blockInfoTexture );
			if( selectedIndex == -1 ) return;
			
			Block block = blocksTable[selectedIndex].BlockId;
			UpdateBlockInfoString( block );
			string value = buffer.GetString();
			
			Size size = Utils2D.MeasureSize( value, font, true );
			int x = startX + ( blockSize * blocksPerRow ) / 2 - size.Width / 2;
			int y = startY - size.Height;
			
			using( Bitmap bmp = Utils2D.CreatePow2Bitmap( size ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					Utils2D.DrawRect( g, backColour, 0, 0, bmp.Width, bmp.Height );
					DrawTextArgs args = new DrawTextArgs( graphicsApi, value, true );
					args.SkipPartsCheck = true;
					Utils2D.DrawText( g, font, ref args, 0, 0 );
				}
				blockInfoTexture = Utils2D.Make2DTexture( graphicsApi, bmp, size, x, y );
			}
		}
		
		void RecreateBlockTextures() {
			int blocksCount = 0;
			for( int i = 0; i < BlockInfo.BlocksCount; i++ ) {
				if( game.CanPlace[i] || game.CanDelete[i] ) {
					blocksCount++;
				}
			}
			
			rows = blocksCount / blocksPerRow + ( blocksCount % blocksPerRow != 0 ? 1 : 0 );
			startX = game.Width / 2 - ( blockSize * blocksPerRow ) / 2;
			startY = game.Height / 2 - ( rows * blockSize ) / 2;
			int x = startX, y = startY;
			blocksTable = new BlockDrawInfo[blocksCount];
			
			int tableIndex = 0;
			for( int tile = 1; tile < BlockInfo.BlocksCount; tile++ ) {
				if( game.CanPlace[tile] || game.CanDelete[tile] ) {
					Block block = (Block)tile;
					int texId = game.BlockInfo.GetTextureLoc( (byte)block, TileSide.Left );
					TextureRectangle rec = game.TerrainAtlas.GetTexRec( texId );
					int verSize = blockSize;
					float height = game.BlockInfo.BlockHeight( (byte)block );
					int blockY = y;
					if( height != 1 ) {
						rec.V1 = rec.V1 + TerrainAtlas2D.invElementSize * height;
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
		
		public override bool HandlesAllInput {
			get { return true; }
		}
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			selectedIndex = -1;
			if( Contains( startX, startY, blocksPerRow * blockSize, rows * blockSize, mouseX, mouseY ) ) {
				for( int i = 0; i < blocksTable.Length; i++ ) {
					int col = i % blocksPerRow;
					int row = i / blocksPerRow;
					int x = startX + blockSize * col;
					int y = startY + blockSize * row;
					
					if( Contains( x, y, blockSize, blockSize, mouseX, mouseY ) ) {
						selectedIndex = i;
						break;
					}
				}
			}
			RecreateBlockInfoTexture();
			return true;
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( button == MouseButton.Left && selectedIndex != -1 ) {
				BlockDrawInfo info = blocksTable[selectedIndex];
				game.HeldBlock = info.BlockId;
				game.SetNewScreen( new NormalScreen( game ) );
			}
			return true;
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == game.Keys[KeyMapping.PauseOrExit] ) {
				game.SetNewScreen( new NormalScreen( game ) );
			}
			return true;
		}
		
		static bool Contains( int recX, int recY, int width, int height, int x, int y ) {
			return x >= recX && y >= recY && x < recX + width && y < recY + height;
		}
	}
}
