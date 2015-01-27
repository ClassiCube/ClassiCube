using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class ExtPlayerListWidget : Widget {
		
		public ExtPlayerListWidget( Game window ) : base( window ) {
			HorizontalDocking = Docking.Centre;
			VerticalDocking = Docking.Centre;
		}
		
		const int namesPerColumn = 20;
		List<PlayerInfo> info = new List<PlayerInfo>( 256 );
		int rows;
		int xMin, xMax, yHeight;
		static FastColour tableCol = new FastColour( 100, 100, 100, 80 );
		
		class PlayerInfo {
			
			public Texture Texture;
			
			public string Name;
			
			public short Ping;
			
			public PlayerInfo( IGraphicsApi graphics, PlayerListInfo p ) {
				Name = p.Name;
				Ping = p.Ping;
				List<DrawTextArgs> parts = Utils.SplitText( graphics, Name, true );
				Size size = Utils2D.MeasureSize( Utils.StripColours( Name ), "Arial", 12, true );
				Texture = Utils2D.MakeTextTexture( parts, "Arial", 12, size, 0, 0 );
			}
			
			public override string ToString() {
				return Name;
			}
		}
		
		class PlayerInfoComparer : IComparer<PlayerInfo> {
			
			public int Compare( PlayerInfo x, PlayerInfo y ) {
				return x.Name.CompareTo( y.Name );
			}
		}
		
		public override void Init() {
			CreateInitialPlayerInfo();
			rows = (int)Math.Ceiling( (double)info.Count / namesPerColumn );
			SortPlayerInfo();
			Window.CpeListInfoAdded += PlayerListInfoAdded;
			Window.CpeListInfoRemoved += PlayerListInfoRemoved;
			Window.CpeListInfoChanged += PlayerListInfoChanged;
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
			Window.CpeListInfoAdded -= PlayerListInfoAdded;
			Window.CpeListInfoChanged -= PlayerListInfoChanged;
			Window.CpeListInfoRemoved -= PlayerListInfoRemoved;
		}
		
		void PlayerListInfoChanged( object sender, TextEventArgs e ) {
			for( int i = 0; i < info.Count; i++ ) {
				PlayerInfo pInfo = info[i];
				if( pInfo.Name == e.Text ) {
					GraphicsApi.DeleteTexture( ref pInfo.Texture );
					info[i] = new PlayerInfo( GraphicsApi, Window.PlayersList[e.Text] );
					SortPlayerInfo();
					break;
				}
			}
		}

		void PlayerListInfoRemoved( object sender, TextEventArgs e ) {
			for( int i = 0; i < info.Count; i++ ) {
				PlayerInfo pInfo = info[i];
				if( pInfo.Name == e.Text ) {
					GraphicsApi.DeleteTexture( ref pInfo.Texture );
					info.RemoveAt( i );
					rows = (int)Math.Ceiling( (double)info.Count / namesPerColumn );
					SortPlayerInfo();
					break;
				}
			}
		}

		void PlayerListInfoAdded( object sender, TextEventArgs e ) {
			PlayerListInfo player = Window.PlayersList[e.Text];
			info.Add( new PlayerInfo( GraphicsApi, player ) );
			rows = (int)Math.Ceiling( (double)info.Count / namesPerColumn );
			SortPlayerInfo();
		}

		void CreateInitialPlayerInfo() {
			foreach( var pair in Window.PlayersList ) {
				info.Add( new PlayerInfo( GraphicsApi, pair.Value ) );
			}
		}
		
		void SortInfoList() {
			if( info.Count == 0 ) return;
			info.Sort( (a, b) => a.Name.CompareTo( b.Name ) );
		}
		
		void SortPlayerInfo() {
			bool evenRows = rows % 2 == 0;
			SortInfoList();
			
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
			Y = Window.Height / 2 - height / 2 - boundsSize;
			Width = width + boundsSize * 2;
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