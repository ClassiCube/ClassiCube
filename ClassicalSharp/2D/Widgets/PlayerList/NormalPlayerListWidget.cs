// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;

namespace ClassicalSharp.Gui {
	
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
			game.EntityEvents.Added += PlayerSpawned;
			game.EntityEvents.Removed += PlayerDespawned;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.EntityEvents.Added -= PlayerSpawned;
			game.EntityEvents.Removed -= PlayerDespawned;
		}
		
		void PlayerSpawned( object sender, IdEventArgs e ) {
			Player player = game.Players[e.Id];
			AddPlayerInfo( player );
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
		
		public override string GetNameUnder( int mouseX, int mouseY ) {
			for( int i = 0; i < namesCount; i++ ) {
				Texture texture = textures[i];
				if( texture.IsValid && texture.Bounds.Contains( mouseX, mouseY ) )
					return Utils.StripColours( info[i].Name );
			}
			return null;
		}
		
		void AddPlayerInfo( Player player ) {
			DrawTextArgs args = new DrawTextArgs( player.DisplayName, font, true );
			Texture tex = game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );
			game.Drawer2D.ReducePadding( ref tex, Utils.Floor( font.Size ), 3 );
			
			info[namesCount] = new PlayerInfo( player );
			textures[namesCount] = tex;
			namesCount++;
		}
		
		void PlayerDespawned( object sender, IdEventArgs e ) {
			for( int i = 0; i < namesCount; i++ ) {
				PlayerInfo pInfo = info[i];
				if( pInfo.PlayerId == e.Id ) {
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