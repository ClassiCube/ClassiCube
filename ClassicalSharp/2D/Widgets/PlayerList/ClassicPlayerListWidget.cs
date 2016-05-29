// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;

namespace ClassicalSharp.Gui {
	
	public sealed class ClassicPlayerListWidget : PlayerListWidget {
		
		bool extList;
		int elemHeight;
		ChatTextWidget overview;
		static FastColour lightTableCol = new FastColour( 20, 20, 20, 180 );
		public ClassicPlayerListWidget( Game game, Font font ) : base( game, font ) {
			textures = new Texture[256];
			extList = game.Network.UsingExtPlayerList;
		}
		
		PlayerInfo[] info = new PlayerInfo[256];
		class PlayerInfo {
			
			public string Name, ColouredName;
			public byte Id;
			
			public PlayerInfo( TabListEntry p ) {
				ColouredName = p.PlayerName;
				Name = Utils.StripColours( p.PlayerName );
				Id = p.NameId;
			}
		}
		
		public override string GetNameUnder( int mouseX, int mouseY ) {
			for( int i = 0; i < namesCount; i++ ) {
				Texture texture = textures[i];
				if( texture.IsValid && texture.Bounds.Contains( mouseX, mouseY ) )
					return Utils.StripColours( info[i].Name );
			}
			return null;
		}
		
		protected override void OnSort() {
			int width = 0, centreX = game.Width / 2;
			for( int col = 0; col < columns; col++)
				width += GetColumnWidth( col );
			if( width < 480 ) width = 480;
			
			xMin = centreX - width / 2;
			xMax = centreX + width / 2;
			
			int x = xMin, y = game.Height / 2 - yHeight / 2;
			for( int col = 0; col < columns; col++ ) {
				SetColumnPos( col, x, y );
				x += GetColumnWidth( col );
			}
		}
		
		public override void Render( double delta ) {
			api.Texturing = false;
			int offset = overview.Height;
			int height = namesPerColumn * (elemHeight + 1) + boundsSize * 2 + offset;
			api.Draw2DQuad( X, Y - offset, Width, height, lightTableCol );
			
			api.Texturing = true;
			overview.MoveTo( game.Width / 2 - overview.Width / 2,
			                Y - offset + boundsSize / 2 );
			overview.Render( delta );
			
			for( int i = 0; i < namesCount; i++ ) {
				Texture texture = textures[i];
				if( texture.IsValid )
					texture.Render( api );
			}
		}
		
		public override void Init() {
			DrawTextArgs measureArgs = new DrawTextArgs( "ABC", font, false );
			
			elemHeight = game.Drawer2D.MeasureChatSize( ref measureArgs ).Height;
			overview = ChatTextWidget.Create( game, 0, 0, "Connected players:",
			                                 Anchor.Centre, Anchor.Centre, font );
			
			base.Init();
			game.EntityEvents.TabListEntryAdded += TabyEntryAdded;
			game.EntityEvents.TabListEntryRemoved += TabEntryRemoved;
			game.EntityEvents.TabListEntryChanged += TabEntryChanged;
		}
		
		public override void Dispose() {
			base.Dispose();
			overview.Dispose();
			game.EntityEvents.TabListEntryAdded -= TabyEntryAdded;
			game.EntityEvents.TabListEntryChanged -= TabEntryChanged;
			game.EntityEvents.TabListEntryRemoved -= TabEntryRemoved;
		}

		protected override void CreateInitialPlayerInfo() {
			TabListEntry[] entries = game.TabList.Entries;
			for( int i = 0; i < entries.Length; i++ ) {
				TabListEntry e = entries[i];
				if( e != null )
					AddPlayerInfo( new PlayerInfo( e ), -1 );
			}
		}
		
		void AddPlayerInfo( PlayerInfo pInfo, int index ) {
			DrawTextArgs args = new DrawTextArgs( pInfo.ColouredName, font, false );
			Texture tex = game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );
			game.Drawer2D.ReducePadding( ref tex, Utils.Floor( font.Size ), 3 );
			
			if( index < 0 ) {
				info[namesCount] = pInfo;
				textures[namesCount] = tex;
				namesCount++;
			} else {
				info[index] = pInfo;
				textures[index] = tex;
			}
		}
		
		void TabyEntryAdded( object sender, IdEventArgs e ) {
			AddPlayerInfo( new PlayerInfo( game.TabList.Entries[e.Id] ), -1 );
			SortPlayerInfo();
		}
		
		void TabEntryChanged( object sender, IdEventArgs e ) {
			for( int i = 0; i < namesCount; i++ ) {
				PlayerInfo pInfo = info[i];
				if( pInfo.Id != e.Id ) continue;
				
				Texture tex = textures[i];
				api.DeleteTexture( ref tex );
				AddPlayerInfo( new PlayerInfo( game.TabList.Entries[e.Id] ), i );
				SortPlayerInfo();
				return;
			}
		}
		
		void TabEntryRemoved( object sender, IdEventArgs e ) {
			for( int i = 0; i < namesCount; i++ ) {
				PlayerInfo pInfo = info[i];
				if( pInfo.Id == e.Id ) {
					RemoveInfoAt( info, i );
					return;
				}
			}
		}
		
		PlayerInfoComparer comparer = new PlayerInfoComparer();
		class PlayerInfoComparer : IComparer<PlayerInfo> {
			
			public int Compare( PlayerInfo x, PlayerInfo y ) {
				return x.Name.CompareTo( y.Name );
			}
		}
		
		protected override void SortInfoList() {
			Array.Sort( info, textures, 0, namesCount, comparer );
		}
	}
}