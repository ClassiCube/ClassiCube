// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Map;
using BlockID = System.UInt16;

namespace ClassicalSharp {

	public delegate void IdEventFunc(byte id);
	public delegate void ColCodeEventFunc(char code);
	public delegate void LoadingEventFunc(float progress);
	public delegate void EnvVarEventFunc(EnvVar envVar);
	public delegate void ChatEventFunc(ref string text, MessageType type);
	public delegate void TextureEventFunc(string name, byte[] data);	
	public delegate void BlockChangedEventFunc(Vector3I coords, BlockID old, BlockID now);
	
	public static class Events {
		
		/// <summary> Raised when the terrain atlas ("terrain.png") is changed. </summary>
		public static event EmptyEventFunc TerrainAtlasChanged;
		public static void RaiseTerrainAtlasChanged() { Raise(TerrainAtlasChanged); }
		
		/// <summary> Raised when the texture pack is changed. </summary>
		public static event EmptyEventFunc TexturePackChanged;
		public static void RaiseTexturePackChanged() { Raise(TexturePackChanged); }
		
		/// <summary> Raised when a texture is changed. (such as "terrain", "rain") </summary>
		public static event TextureEventFunc TextureChanged;
		public static void RaiseTextureChanged(string name, byte[] data) {
			if (TextureChanged != null) TextureChanged(name, data); 
		}
		
		
		/// <summary> Raised when the user changed their view/fog distance. </summary>
		public static event EmptyEventFunc ViewDistanceChanged;
		public static void RaiseViewDistanceChanged() { Raise(ViewDistanceChanged); }
		
		/// <summary> Raised when the projection matrix changes. </summary>
		public static event EmptyEventFunc ProjectionChanged;
		public static void RaiseProjectionChanged() { Raise(ProjectionChanged); }

		/// <summary> Raised when the graphics context is lost. </summary>
		public static event EmptyEventFunc ContextLost;
		public static void RaiseContextLost() { Raise(ContextLost); }
		
		/// <summary> Raised when the graphics context is regained/recreated. </summary>
		public static event EmptyEventFunc ContextRecreated;
		public static void RaiseContextRecreated() { Raise(ContextRecreated); }
		
		
		/// <summary> Raised when the held block is changed by the user or by CPE. </summary>
		public static event EmptyEventFunc HeldBlockChanged;
		public static void RaiseHeldBlockChanged() { Raise(HeldBlockChanged); }
		
		/// <summary> Raised when the block permissions(can place or delete a block) for the player change. </summary>
		public static event EmptyEventFunc BlockPermissionsChanged;
		public static void RaiseBlockPermissionsChanged() { Raise(BlockPermissionsChanged); }
		
		/// <summary> Raised when a block definition is changed. </summary>
		public static event EmptyEventFunc BlockDefinitionChanged;
		public static void RaiseBlockDefinitionChanged() { Raise(BlockDefinitionChanged); }
		
		
		/// <summary> Raised when message is being added to chat. </summary>
		public static event ChatEventFunc ChatReceived;	
		public static void RaiseChatReceived(ref string text, MessageType type) { 
			if (ChatReceived != null) ChatReceived(ref text, type); 
		}
		
		/// <summary> Raised when user sends a message. </summary>
		public static event ChatEventFunc ChatSending;
		public static void RaiseChatSending(ref string text) { 
			if (ChatSending != null) ChatSending(ref text, MessageType.Normal); 
		}
		
		/// <summary> Raised when the user changes chat font to arial or back to bitmapped font,
		/// also raised when the bitmapped font changes. </summary>
		public static event EmptyEventFunc ChatFontChanged;
		public static void RaiseChatFontChanged() { Raise(ChatFontChanged); }
		
		/// <summary> Raised when the colour codes usable by the player changes. </summary>
		public static event ColCodeEventFunc ColCodeChanged;
		public static void RaiseColCodeChanged(char code) {
			if (ColCodeChanged != null) ColCodeChanged(code);
		}
		
		
		/// <summary> Raised when the hack permissions of the player changes. </summary>
		public static event EmptyEventFunc HackPermissionsChanged;
		public static void RaiseHackPermissionsChanged() { Raise(HackPermissionsChanged); }
		//BlockChanging event_here; // ??
		
		/// <summary> Raised when the user changes a block in the world. </summary>
		public static event BlockChangedEventFunc BlockChanged;
		public static void RaiseBlockChanged(Vector3I coords, BlockID old, BlockID now) { 
			if (BlockChanged != null) BlockChanged(coords, old, now);
		}
		
		
		/// <summary> Raised when the player joins and begins loading a new world. </summary>
		public static event EmptyEventFunc OnNewMap;
		public static void RaiseOnNewMap() { Raise(OnNewMap); }
		
		/// <summary> Raised when a portion of the world is read and decompressed, or generated. </summary>
		public static event LoadingEventFunc Loading;
		public static void RaiseLoading(float progress) { if (Loading != null) Loading(progress); }
		
		/// <summary> Raised when new world has finished loading and the player can now interact with it. </summary>
		public static event EmptyEventFunc OnNewMapLoaded;
		public static void RaiseOnNewMapLoaded() { Raise(OnNewMapLoaded); }		
		
		/// <summary> Raised when an environment variable of the world is changed by the user, CPE, or WoM config. </summary>
		public static event EnvVarEventFunc EnvVariableChanged;
		public static void RaiseEnvVariableChanged(EnvVar envVar) {
			if (EnvVariableChanged != null) EnvVariableChanged(envVar);
		}
	
	
		/// <summary> Raised when an entity is spawned in the current world. </summary>
		public static event IdEventFunc EntityAdded;
		public static void RaiseEntityAdded(byte id) { Raise(EntityAdded, id); }
		
		/// <summary> Raised when an entity is despawned from the current world. </summary>
		public static event IdEventFunc EntityRemoved;
		public static void RaiseEntityRemoved(byte id) { Raise(EntityRemoved, id); }
		
		
		/// <summary> Raised when a tab list entry is created. </summary>
		public static event IdEventFunc TabListEntryAdded;
		public static void RaiseTabEntryAdded(byte id) { Raise(TabListEntryAdded, id); }
		
		/// <summary> Raised when a tab list entry is modified. </summary>
		public static event IdEventFunc TabListEntryChanged;
		public static void RaiseTabListEntryChanged(byte id) { Raise(TabListEntryChanged, id); }
		
		/// <summary> Raised when a tab list entry is removed. </summary>
		public static event IdEventFunc TabListEntryRemoved;
		public static void RaiseTabEntryRemoved(byte id) { Raise(TabListEntryRemoved, id); }
		
		
		static void Raise(EmptyEventFunc handler) { if (handler != null) handler(); }		
		static void Raise(IdEventFunc handler, byte id) { if (handler != null) handler(id); }
	}
}
