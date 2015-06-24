using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public class ExtPlayerListWidget : PlayerListWidget {
		
		public ExtPlayerListWidget( Game window, Font font ) : base( window, font ) {
		}
		
		PlayerInfo[] info = new PlayerInfo[256];
		class PlayerInfo {
			
			public string Name;			
			public string GroupName;		
			public byte GroupRank;			
			public byte NameId;
			
			public PlayerInfo( CpeListInfo p ) {
				Name = p.ListName;
				NameId = p.NameId;
				GroupName = p.GroupName;
				GroupRank = p.GroupRank;
			}
			
			public override string ToString() {
				return NameId + ":" + Name + "(" + GroupName + "," + GroupRank + ")";
			}
		}
		
		PlayerInfoComparer comparer = new PlayerInfoComparer();
		class PlayerInfoComparer : IComparer<PlayerInfo> {
			
			public bool JustComparingGroups = true;
			
			public int Compare( PlayerInfo x, PlayerInfo y ) {
				if( JustComparingGroups ) {
					return x.GroupName.CompareTo( y.GroupName );
				}
				
				int rankOrder = x.GroupRank.CompareTo( y.GroupRank );
				if( rankOrder != 0 ) {
					return rankOrder;
				}
				return x.Name.CompareTo( y.Name );
			}
		}
		
		public override void Init() {
			base.Init();
			Window.CpeListInfoAdded += PlayerListInfoAdded;
			Window.CpeListInfoRemoved += PlayerListInfoRemoved;
			Window.CpeListInfoChanged += PlayerListInfoChanged;
		}
		
		public override void Dispose() {
			base.Dispose();
			Window.CpeListInfoAdded -= PlayerListInfoAdded;
			Window.CpeListInfoChanged -= PlayerListInfoChanged;
			Window.CpeListInfoRemoved -= PlayerListInfoRemoved;
		}
		
		void PlayerListInfoChanged( object sender, IdEventArgs e ) {
			for( int i = 0; i < namesCount; i++ ) {
				PlayerInfo pInfo = info[i];
				if( pInfo.NameId == e.Id ) {
					Texture tex = textures[i];
					GraphicsApi.DeleteTexture( ref tex );
					AddPlayerInfo( Window.CpePlayersList[e.Id], i );
					SortPlayerInfo();
					return;
				}
			}
		}

		void PlayerListInfoRemoved( object sender, IdEventArgs e ) {
			for( int i = 0; i < namesCount; i++ ) {
				PlayerInfo pInfo = info[i];
				if( pInfo.NameId == e.Id ) {
					Texture tex = textures[i];
					GraphicsApi.DeleteTexture( ref tex );
					RemoveItemAt( textures, i );
					RemoveItemAt( info, i );
					namesCount--;
					columns = (int)Math.Ceiling( (double)namesCount / namesPerColumn );
					SortPlayerInfo();
					return;
				}
			}
		}

		void PlayerListInfoAdded( object sender, IdEventArgs e ) {
			AddPlayerInfo( Window.CpePlayersList[e.Id], -1 );
			columns = (int)Math.Ceiling( (double)namesCount / namesPerColumn );
			SortPlayerInfo();
		}

		protected override void CreateInitialPlayerInfo() {
			for( int i = 0; i < Window.CpePlayersList.Length; i++ ) {
				CpeListInfo player = Window.CpePlayersList[i];
				if( player != null ) {
					AddPlayerInfo( player, -1 );
				}
			}
		}
		
		void AddPlayerInfo( CpeListInfo player, int index ) {
			List<DrawTextArgs> parts = Utils2D.SplitText( GraphicsApi, player.ListName, true );
			Size size = Utils2D.MeasureSize( parts, font, true );
			Texture tex = Utils2D.MakeTextTexture( parts, font, size, 0, 0 );
			if( index < 0 ) {
				info[namesCount] = new PlayerInfo( player );
				textures[namesCount] = tex;
				namesCount++;
			} else {
				info[index] = new PlayerInfo( player );
				textures[index] = tex;
			}
		}
		
		protected override void SortInfoList() {
			if( namesCount  == 0 ) return;
			// Sort the list into groups
			comparer.JustComparingGroups = true;
			Array.Sort( info, textures, 0, namesCount, comparer );
			
			// Sort the entries in each group
			comparer.JustComparingGroups = false;
			int index = 0;
			while( index < namesCount ) {
				int count = GetGroupCount( index );
				Array.Sort( info, textures, index, count, comparer );
				index += count;
			}
		}
		
		int GetGroupCount( int startIndex ) {
			string group = info[startIndex].GroupName;
			int count = 0;
			while( startIndex < namesCount && info[startIndex++].GroupName == group ) {
				count++;
			}
			return count;
		}
	}
}