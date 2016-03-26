// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.Events;

namespace ClassicalSharp.Gui {
	
	public class ExtPlayerListWidget : PlayerListWidget {
		
		public ExtPlayerListWidget( Game game, Font font ) : base( game, font ) {
			textures = new Texture[512];
			titleFont = new Font( game.FontName, 12, FontStyle.Underline );
		}
		
		PlayerInfo[] info = new PlayerInfo[512];
		class PlayerInfo {
			
			public string ListName;
			public string PlayerName;
			public string GroupName;
			public byte GroupRank;
			public byte NameId;
			public bool IsGroup = false;
			
			public PlayerInfo( CpeListInfo p ) {
				ListName = Utils.StripColours( p.ListName );
				PlayerName = Utils.StripColours( p.PlayerName );
				NameId = p.NameId;
				GroupName = p.GroupName;
				GroupRank = p.GroupRank;
			}
			
			public PlayerInfo( string groupName ) {
				GroupName = groupName;
				IsGroup = true;
			}
		}
		
		PlayerInfoComparer comparer = new PlayerInfoComparer();
		class PlayerInfoComparer : IComparer<PlayerInfo> {
			
			public bool JustComparingGroups = true;
			
			public int Compare( PlayerInfo x, PlayerInfo y ) {
				if( JustComparingGroups )
					return x.GroupName.CompareTo( y.GroupName );
				
				int rankOrder = x.GroupRank.CompareTo( y.GroupRank );
				return rankOrder != 0 ? rankOrder :
					x.ListName.CompareTo( y.ListName );
			}
		}
		
		Font titleFont;
		public override void Init() {
			base.Init();
			game.EntityEvents.CpeListInfoAdded += PlayerListInfoAdded;
			game.EntityEvents.CpeListInfoRemoved += PlayerListInfoRemoved;
			game.EntityEvents.CpeListInfoChanged += PlayerListInfoChanged;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.EntityEvents.CpeListInfoAdded -= PlayerListInfoAdded;
			game.EntityEvents.CpeListInfoChanged -= PlayerListInfoChanged;
			game.EntityEvents.CpeListInfoRemoved -= PlayerListInfoRemoved;
			titleFont.Dispose();
		}
		
		void PlayerListInfoChanged( object sender, IdEventArgs e ) {
			for( int i = 0; i < namesCount; i++ ) {
				PlayerInfo pInfo = info[i];
				if( !pInfo.IsGroup && pInfo.NameId == e.Id ) {
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
				if( !pInfo.IsGroup && pInfo.NameId == e.Id ) {
					RemoveInfoAt( info, i );
					return;
				}
			}
		}

		void PlayerListInfoAdded( object sender, IdEventArgs e ) {
			AddPlayerInfo( game.CpePlayersList[e.Id], -1 );
			columns = Utils.CeilDiv( namesCount, namesPerColumn );
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
		
		public override string GetNameUnder( int mouseX, int mouseY ) {
			for( int i = 0; i < namesCount; i++ ) {
				Texture texture = textures[i];
				if( texture.IsValid && texture.Bounds.Contains( mouseX, mouseY )
				   && info[i].PlayerName != null )
					return Utils.StripColours( info[i].PlayerName );
			}
			return null;
		}
		
		void AddPlayerInfo( CpeListInfo player, int index ) {
			DrawTextArgs args = new DrawTextArgs( player.ListName, font, true );
			Texture tex = game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );
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
			if( namesCount == 0 ) return;
			
			// Sort the list into groups
			for( int i = 0; i < namesCount; i++ ) {
				if( info[i].IsGroup ) DeleteGroup( ref i );
			}
			comparer.JustComparingGroups = true;
			Array.Sort( info, textures, 0, namesCount, comparer );
			
			// Sort the entries in each group
			comparer.JustComparingGroups = false;
			int index = 0;
			while( index < namesCount ) {
				AddGroup( info[index].GroupName, ref index );
				int count = GetGroupCount( index );
				Array.Sort( info, textures, index, count, comparer );
				index += count;
			}
		}
		
		void DeleteGroup( ref int i ) {
			graphicsApi.DeleteTexture( ref textures[i] );
			RemoveItemAt( info, i );
			RemoveItemAt( textures, i );
			
			namesCount--;
			i--;
		}
		
		void AddGroup( string group, ref int index ) {
			DrawTextArgs args = new DrawTextArgs( group, titleFont, true );
			Texture tex = game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );
			PlayerInfo pInfo = new PlayerInfo( group );
			
			PushDown( info, index, pInfo );
			PushDown( textures, index, tex );
			index++;
			namesCount++;
		}
		
		void PushDown<T>( T[] array, int index, T value ) {
			for( int i = array.Length - 1; i > index; i-- ) {
				array[i] = array[i - 1];
			}
			array[index] = value;
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