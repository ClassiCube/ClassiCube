using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public sealed class NormalPlayerListWidget : PlayerListWidget {
		
		public NormalPlayerListWidget( Game window, Font font ) : base( window, font ) {
		}
		
		PlayerInfo[] info = new PlayerInfo[256];		
		class PlayerInfo {
			
			public string Name;
			public byte PlayerId;
			
			public PlayerInfo( Player p ) {
				Name = p.DisplayName;
				PlayerId = p.ID;
			}
		}
		
		public override void Init() {
			base.Init();
			Window.EntityAdded += PlayerSpawned;
			Window.EntityRemoved += PlayerDespawned;
		}
		
		public override void Dispose() {
			base.Dispose();
			Window.EntityAdded -= PlayerSpawned;
			Window.EntityRemoved -= PlayerDespawned;
		}
		
		void PlayerSpawned( object sender, IdEventArgs e ) {
			Player player = Window.Players[e.Id];
			AddPlayerInfo( player );
			columns = (int)Math.Ceiling( (double)namesCount / namesPerColumn );
			SortPlayerInfo();
		}

		protected override void CreateInitialPlayerInfo() {
			for( int i = 0; i < Window.Players.MaxCount; i++ ) {
				Player player = Window.Players[i];
				if( player != null ) {
					AddPlayerInfo( player );
				}
			}
		}
		
		void AddPlayerInfo( Player player ) {
			DrawTextArgs args = new DrawTextArgs( GraphicsApi, player.DisplayName, true );
			Texture tex = Utils2D.MakeTextTexture( font, 0, 0, ref args );
			info[namesCount] = new PlayerInfo( player );
			textures[namesCount] = tex;
			namesCount++;
		}
		
		void PlayerDespawned( object sender, IdEventArgs e ) {
			for( int i = 0; i < namesCount; i++ ) {
				PlayerInfo pInfo = info[i];
				if( pInfo.PlayerId == e.Id ) {
					Texture tex = textures[i];
					GraphicsApi.DeleteTexture( ref tex );
					RemoveItemAt( info, i );
					RemoveItemAt( textures, i );
					namesCount--;
					columns = (int)Math.Ceiling( (double)namesCount / namesPerColumn );
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