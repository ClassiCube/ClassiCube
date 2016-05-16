// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public partial class InventoryScreen : Screen {
		
		public InventoryScreen( Game game ) : base( game ) {
			font = new Font( game.FontName, 16 );
		}
		
		Block[] blocksTable;
		Texture blockInfoTexture;
		const int maxRows = 8;
		int blocksPerRow { 
			get { return game.ClassicMode && !game.ClassicHacks ? 9 : 10; } 
		}
		
		int selIndex, rows;
		int startX, startY, blockSize;
		float selBlockExpand;
		readonly Font font;
		StringBuffer buffer = new StringBuffer( 128 );
		IsometricBlockDrawer drawer = new IsometricBlockDrawer();
		
		int TableX { get { return startX - 5 - 10; } }
		int TableY { get { return startY - 5 - 30; } }
		int TableWidth { get { return blocksPerRow * blockSize + 10 + 10; } }
		int TableHeight { get { return Math.Min( rows, maxRows ) * blockSize + 10 + 30; } }
		
		static FastColour normBackCol = new FastColour( 30, 30, 30, 200 );
		static FastColour classicBackCol = new FastColour( 48, 48, 96, 192 );
		static VertexP3fT2fC4b[] vertices = new VertexP3fT2fC4b[8 * 10 * (4 * 4)];
		int vb;
		
		public override void Render( double delta ) {
			FastColour backCol = game.ClassicMode ? classicBackCol : normBackCol;
			api.Draw2DQuad( TableX, TableY, TableWidth, TableHeight, backCol );
			if( rows > maxRows )
				DrawScrollbar();
			api.Texturing = true;
			api.SetBatchFormat( VertexFormat.P3fT2fC4b );
			
			drawer.BeginBatch( game, vertices, vb );
			for( int i = 0; i < blocksTable.Length; i++ ) {
				int x, y;
				if( !GetCoords( i, out x, out y ) ) continue;
				
				// We want to always draw the selected block on top of others
				if( i == selIndex ) continue;
				drawer.DrawBatch( (byte)blocksTable[i], blockSize * 0.7f / 2f,
				                 x + blockSize / 2, y + blockSize / 2 );
			}
			
			if( selIndex != -1 ) {
				int x, y;
				GetCoords( selIndex, out x, out y );
				drawer.DrawBatch( (byte)blocksTable[selIndex], (blockSize + selBlockExpand) * 0.7f / 2,
				                 x + blockSize / 2, y + blockSize / 2 );
			}
			drawer.EndBatch();
			
			if( blockInfoTexture.IsValid )
				blockInfoTexture.Render( api );
			game.hudScreen.RenderHotbar( delta );
			api.Texturing = false;
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
		
		Point GetMouseCoords( int i ) {
			int x, y;
			GetCoords( i, out x, out y );
			x += blockSize / 2; y += blockSize / 2;
			
			Point topLeft = game.PointToScreen( Point.Empty );
			x += topLeft.X; y += topLeft.Y;
			return new Point( x, y );
		}
		
		public override void Dispose() {
			font.Dispose();
			api.DeleteTexture( ref blockInfoTexture );
			api.DeleteDynamicVb( vb );
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
			vb = game.Graphics.CreateDynamicVb( VertexFormat.P3fT2fC4b, vertices.Length );
			
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
			game.DesktopCursorPos = GetMouseCoords( selIndex );
		}

		void BlockPermissionsChanged( object sender, EventArgs e ) {
			RecreateBlockTable();
			if( selIndex >= blocksTable.Length )
				selIndex = blocksTable.Length - 1;
			
			scrollY = selIndex / blocksPerRow;
			ClampScrollY();
			RecreateBlockInfoTexture();
		}
		
		void UpdateBlockInfoString( Block block ) {
			int index = 0;
			buffer.Clear();
			buffer.Append( ref index, "&f" );
			string value = game.BlockInfo.GetBlockName( (byte)block );
			buffer.Append( ref index, value );
			if( game.ClassicMode ) return;
			
			buffer.Append( ref index, " (ID: " );
			buffer.AppendNum( ref index, (byte)block );
			buffer.Append( ref index, ", place: " );
			buffer.Append( ref index, game.Inventory.CanPlace[(int)block] ? "&aYes" : "&cNo" );
			buffer.Append( ref index, "&f, delete: " );
			buffer.Append( ref index, game.Inventory.CanDelete[(int)block] ? "&aYes" : "&cNo" );
			buffer.Append( ref index, "&f)" );
		}
		
		int lastCreatedIndex = -1000;
		void RecreateBlockInfoTexture() {
			if( selIndex == lastCreatedIndex ) return;
			lastCreatedIndex = selIndex;
			
			api.DeleteTexture( ref blockInfoTexture );
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
			int count = game.UseCPE ? BlockInfo.BlocksCount : BlockInfo.OriginalCount;
			for( int i = 1; i < count; i++ ) {
				Block block = game.Inventory.MapBlock( i );
				if( Show( block ) ) blocksCount++;
			}
			
			rows = Utils.CeilDiv( blocksCount, blocksPerRow );
			int rowsUsed = Math.Min( maxRows, rows );
			startX = game.Width / 2 - (blockSize * blocksPerRow) / 2;
			startY = game.Height / 2 - (rowsUsed * blockSize) / 2;
			blocksTable = new Block[blocksCount];
			
			int index = 0;
			for( int i = 1; i < count; i++ ) {
				Block block = game.Inventory.MapBlock( i );
				if( Show( block ) )  blocksTable[index++] = block;
			}
		}
		
		bool Show( Block block ) {
			bool hackBlocks = !game.ClassicMode || game.ClassicHacks;
			if( !hackBlocks && IsHackBlock( block ) )
				return false;
			int count = game.UseCPEBlocks ? BlockInfo.CpeCount : BlockInfo.OriginalCount;
			return (byte)block < count || game.BlockInfo.Name[(byte)block] != "Invalid";
		}
		
		bool IsHackBlock( Block block ) {
			return block == Block.DoubleSlab || block == Block.Bedrock ||
				block == Block.Grass || (block >= Block.Water && block <= Block.StillLava);
		}
	}
}
