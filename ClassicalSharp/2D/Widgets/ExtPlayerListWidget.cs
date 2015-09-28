using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public class ExtPlayerListWidget : PlayerListWidget {
		
		public ExtPlayerListWidget( Game game, Font font ) : base( game, font ) {
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
			game.CpeListInfoAdded += PlayerListInfoAdded;
			game.CpeListInfoRemoved += PlayerListInfoRemoved;
			game.CpeListInfoChanged += PlayerListInfoChanged;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.CpeListInfoAdded -= PlayerListInfoAdded;
			game.CpeListInfoChanged -= PlayerListInfoChanged;
			game.CpeListInfoRemoved -= PlayerListInfoRemoved;
		}
		
		void PlayerListInfoChanged( object sender, IdEventArgs e ) {
			for( int i = 0; i < namesCount; i++ ) {
				PlayerInfo pInfo = info[i];
				if( pInfo.NameId == e.Id ) {
					Texture tex = textures[i];
					graphicsApi.DeleteTexture( ref tex );
					AddPlayerInfo( game.CpePlayersList[e.Id], i );
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
					graphicsApi.DeleteTexture( ref tex );
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
			AddPlayerInfo( game.CpePlayersList[e.Id], -1 );
			columns = (int)Math.Ceiling( (double)namesCount / namesPerColumn );
			SortPlayerInfo();
		}

		protected override void CreateInitialPlayerInfo() {
			for( int i = 0; i < game.CpePlayersList.Length; i++ ) {
				CpeListInfo player = game.CpePlayersList[i];
				if( player != null ) {
					AddPlayerInfo( player, -1 );
				}
			}
		}
		
		void AddPlayerInfo( CpeListInfo player, int index ) {
			DrawTextArgs args = new DrawTextArgs(player.ListName, true );
			Texture tex = game.Drawer2D.MakeTextTexture( font, 0, 0, ref args );
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