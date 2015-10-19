using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public sealed class NormalPlayerListWidget : PlayerListWidget {
		
		public NormalPlayerListWidget( Game game, Font font ) : base( game, font ) {
			textures = new Texture[256];
		}
		
		PlayerInfo[] info = new PlayerInfo[256];		
		class PlayerInfo {
			
			public string Name;
			public byte PlayerId;
			
			public PlayerInfo( Player p ) {
				Name = Utils.StripColours( p.DisplayName );
				PlayerId = p.ID;
			}
		}
		
		public override void Init() {
			base.Init();
			game.Events.EntityAdded += PlayerSpawned;
			game.Events.EntityRemoved += PlayerDespawned;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Events.EntityAdded -= PlayerSpawned;
			game.Events.EntityRemoved -= PlayerDespawned;
		}
		
		void PlayerSpawned( object sender, IdEventArgs e ) {
			Player player = game.Players[e.Id];
			AddPlayerInfo( player );
			columns = Utils.CeilDiv( namesCount, namesPerColumn );
			SortPlayerInfo();
		}

		protected override void CreateInitialPlayerInfo() {
			for( int i = 0; i < EntityList.MaxCount; i++ ) {
				Player player = game.Players[i];
				if( player != null ) {
					AddPlayerInfo( player );
				}
			}
		}
		
		void AddPlayerInfo( Player player ) {
			DrawTextArgs args = new DrawTextArgs( player.DisplayName, font, true );
			Texture tex = game.Drawer2D.MakeTextTexture( ref args, 0, 0 );
			info[namesCount] = new PlayerInfo( player );
			textures[namesCount] = tex;
			namesCount++;
		}
		
		void PlayerDespawned( object sender, IdEventArgs e ) {
			for( int i = 0; i < namesCount; i++ ) {
				PlayerInfo pInfo = info[i];
				if( pInfo.PlayerId == e.Id ) {
					Texture tex = textures[i];
					graphicsApi.DeleteTexture( ref tex );
					RemoveItemAt( info, i );
					RemoveItemAt( textures, i );
					namesCount--;
					columns = Utils.CeilDiv( namesCount, namesPerColumn );
					SortPlayerInfo();
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