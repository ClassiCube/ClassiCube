using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public sealed class PlayerListWidget : Widget {
		
		readonly Font font;
		public PlayerListWidget( Game window, Font font ) : base( window ) {
			HorizontalDocking = Docking.Centre;
			VerticalDocking = Docking.Centre;
			this.font = font;
		}
		
		const int namesPerColumn = 20;
		List<PlayerInfo> info = new List<PlayerInfo>( 256 );
		int rows;
		int xMin, xMax, yHeight;
		static FastColour tableCol = new FastColour( 100, 100, 100, 80 );
		
		class PlayerInfo {
			
			public Texture Texture;
			
			public string Name;
			
			public byte PlayerId;
			
			public PlayerInfo( IGraphicsApi graphics, Player p, Font font ) {
				Name = p.DisplayName;
				PlayerId = p.ID;
				List<DrawTextArgs> parts = Utils2D.SplitText( graphics, Name, true );
				Size size = Utils2D.MeasureSize( Utils.StripColours( Name ), font, true );
				Texture = Utils2D.MakeTextTexture( parts, font, size, 0, 0 );
			}
		}
		
		public override void Init() {		
			CreateInitialPlayerInfo();
			rows = (int)Math.Ceiling( (double)info.Count / namesPerColumn );
			SortPlayerInfo();
			Window.EntityAdded += PlayerSpawned;
			Window.EntityRemoved += PlayerDespawned;
			Window.EntityInfoChanged += PlayerInfoChanged;
		}
		
		public override void Render( double delta ) {
			GraphicsApi.Draw2DQuad( X, Y, Width, Height, tableCol );
			for( int i = 0; i < info.Count; i++ ) {
				Texture texture = info[i].Texture;
				if( texture.IsValid ) {
					texture.Render( GraphicsApi );
				}
			}
		}
		
		public override void Dispose() {
			for( int i = 0; i < info.Count; i++ ) {
				GraphicsApi.DeleteTexture( ref info[i].Texture );
			}
			Window.EntityAdded -= PlayerSpawned;
			Window.EntityRemoved -= PlayerDespawned;
			Window.EntityInfoChanged -= PlayerInfoChanged;
		}
		
		void PlayerSpawned( object sender, IdEventArgs e ) {
			Player player = Window.NetPlayers[e.Id];
			info.Add( new PlayerInfo( GraphicsApi, player, font ) );
			rows = (int)Math.Ceiling( (double)info.Count / namesPerColumn );
			SortPlayerInfo();
		}
		
		void PlayerDespawned( object sender, IdEventArgs e ) {
			for( int i = 0; i < info.Count; i++ ) {
				PlayerInfo pInfo = info[i];
				if( pInfo.PlayerId == e.Id ) {
					GraphicsApi.DeleteTexture( ref pInfo.Texture );
					info.RemoveAt( i );
					rows = (int)Math.Ceiling( (double)info.Count / namesPerColumn );
					SortPlayerInfo();
					break;
				}
			}
		}
		
		void PlayerInfoChanged( object sender, IdEventArgs e ) {
			for( int i = 0; i < info.Count; i++ ) {
				PlayerInfo pInfo = info[i];
				if( pInfo.PlayerId == e.Id ) {
					GraphicsApi.DeleteTexture( ref pInfo.Texture );
					info[i] = new PlayerInfo( GraphicsApi, Window.NetPlayers[e.Id], font );
					SortPlayerInfo();
					break;
				}
			}
		}

		void CreateInitialPlayerInfo() {
			for( int i = 0; i < Window.NetPlayers.Length; i++ ) {
				Player player = Window.NetPlayers[i];
				if( player != null ) {
					info.Add( new PlayerInfo( GraphicsApi, player, font ) );
				}
			}
			Player localPlayer = Window.LocalPlayer;
			info.Add( new PlayerInfo( GraphicsApi, localPlayer, font ) );
		}
		
		void SortPlayerInfo() {
			bool evenRows = rows % 2 == 0;
			info.Sort( (a, b) => a.Name.CompareTo( b.Name ) );
			int centreX = Window.Width / 2;
			int maxColHeight = GetMaxColumnHeight();
			int y = Window.Height / 2 - maxColHeight / 2;
			yHeight = maxColHeight;
			
			if( evenRows ) {
				int x = centreX;
				for( int col = rows / 2 - 1; col >= 0; col-- ) {
					x -= GetColumnWidth( col );
					SetColumnPos( col, x, y );
				}
				xMin = x;
				
				x = centreX;
				for( int col = rows / 2; col < rows; col++ ) {
					SetColumnPos( col, x, y );
					x += GetColumnWidth( col );
				}
				xMax = x;
			} else {
				if( rows == 1 ) {
					int colWidth = GetColumnWidth( 0 );
					int x = centreX - colWidth / 2;
					SetColumnPos( 0, x, y );
					xMin = centreX - colWidth / 2;
					xMax = centreX + colWidth / 2;
				} else {
					int middleColHalfWidth = ( GetColumnWidth( rows / 2 ) + 1 ) / 2; // ceiling divide by 2
					int x = centreX - middleColHalfWidth;
					
					for( int col = rows / 2 - 1; col >= 0; col-- ) {
						x -= GetColumnWidth( col );
						SetColumnPos( col, x, y );
					}
					xMin = x;
					
					x = centreX + middleColHalfWidth;
					for( int col = rows / 2 + 1; col < rows; col++ ) {
						SetColumnPos( col, x, y );
						x += GetColumnWidth( col );
					}
					xMax = x;
					
					x = centreX - middleColHalfWidth;
					SetColumnPos( rows / 2, x, y );
				}
			}
			UpdateTableDimensions();
		}
		
		const int boundsSize = 10;
		void UpdateTableDimensions() {
			int width = xMax - xMin;
			int height = yHeight;
			X = xMin - boundsSize;
			Width = width + boundsSize * 2;
			Y = Window.Height / 2 - height / 2 - boundsSize;
			Height = height + boundsSize * 2;
		}
		
		int GetMaxColumnHeight() {
			int maxHeight = 0;
			for( int col = 0; col < rows; col++ ) {
				int height = GetColumnHeight( col );
				if( height > maxHeight ) {
					maxHeight = height;
				}
			}
			return maxHeight;
		}
		
		int GetColumnWidth( int column ) {
			int i = column * namesPerColumn;
			int maxWidth = 0;
			int max = Math.Min( info.Count, i + namesPerColumn );
			
			for( ; i < max; i++ ) {
				Texture texture = info[i].Texture;
				if( texture.Width > maxWidth ) {
					maxWidth = texture.Width;
				}
			}
			return maxWidth;
		}
		
		int GetColumnHeight( int column ) {
			int i = column * namesPerColumn;
			int total = 0;
			int max = Math.Min( info.Count, i + namesPerColumn );
			
			for( ; i < max; i++ ) {
				total += info[i].Texture.Height;
			}
			return total;
		}
		
		void SetColumnPos( int column, int x, int y ) {
			int i = column * namesPerColumn;
			int max = Math.Min( info.Count, i + namesPerColumn );
			
			for( ; i < max; i++ ) {
				PlayerInfo pInfo = info[i];
				pInfo.Texture.X1 = x;
				pInfo.Texture.Y1 = y;
				y += pInfo.Texture.Height;
			}
		}
		
		public override void MoveTo( int newX, int newY ) {
			int deltaX = newX - X;
			int deltaY = newY - Y;
			for( int i = 0; i < info.Count; i++ ) {
				PlayerInfo pInfo = info[i];
				pInfo.Texture.X1 += deltaX;
				pInfo.Texture.Y1 += deltaY;
			}
			X = newX;
			Y = newY;
		}		
	}
}