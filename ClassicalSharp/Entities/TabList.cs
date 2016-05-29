using System;

namespace ClassicalSharp.Entities {

	public sealed class TabList : IGameComponent {
		public TabListEntry[] Entries = new TabListEntry[256];
		
		public void Init( Game game ) { }		
		public void Ready( Game game ) { }
		public void OnNewMapLoaded( Game game ) { }
		public void Dispose() { }
		public void OnNewMap( Game game ) { }
		
		public void Reset( Game game ) {
			for( int i = 0; i < Entries.Length; i++ )
				Entries[i] = null;
		}
	}
	
	public sealed class TabListEntry {
		
		public byte NameId;
		
		/// <summary> Unformatted name of the player for autocompletion, etc. </summary>
		/// <remarks> Colour codes are always removed from this. </remarks>
		public string PlayerName;
		
		/// <summary> Formatted name for display in the player list. </summary>
		/// <remarks> Can include colour codes. </remarks>
		public string ListName;
		
		/// <summary> Name of the group this player is in. </summary>
		/// <remarks> Can include colour codes. </remarks>
		public string GroupName;
		
		/// <summary> Player's rank within the group. (0 is highest) </summary>
		/// <remarks> Multiple group members can share the same rank,
		/// so a player's group rank is not a unique identifier. </remarks>
		public byte GroupRank;
		
		public TabListEntry( byte id, string playerName, string listName,
		                    string groupName, byte groupRank ) {
			NameId = id;
			PlayerName = playerName;
			ListName = listName;
			GroupName = groupName;
			GroupRank = groupRank;
		}
	}
}
