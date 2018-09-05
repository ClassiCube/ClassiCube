// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Map;
using BlockID = System.UInt16;

namespace ClassicalSharp {

	public static class Events {
		
		/// <summary> Raised when the terrain atlas ("terrain.png") is changed. </summary>
		public static event EventHandler TerrainAtlasChanged;
		public static void RaiseTerrainAtlasChanged() { Raise(TerrainAtlasChanged); }
		
		/// <summary> Raised when the texture pack is changed. </summary>
		public static event EventHandler TexturePackChanged;
		public static void RaiseTexturePackChanged() { Raise(TexturePackChanged); }
		
		/// <summary> Raised when a texture is changed. (such as "terrain", "rain") </summary>
		public static event EventHandler<TextureEventArgs> TextureChanged;
		public static void RaiseTextureChanged(string name, byte[] data) {
			texArgs.Name = name; texArgs.Data = data; Raise(TextureChanged, texArgs); }
		
		
		/// <summary> Raised when the user changed their view/fog distance. </summary>
		public static event EventHandler ViewDistanceChanged;
		public static void RaiseViewDistanceChanged() { Raise(ViewDistanceChanged); }
		
		/// <summary> Raised when the projection matrix changes. </summary>
		public static event EventHandler ProjectionChanged;
		public static void RaiseProjectionChanged() { Raise(ProjectionChanged); }
		
		
		/// <summary> Raised when the held block is changed by the user or by CPE. </summary>
		public static event EventHandler HeldBlockChanged;
		public static void RaiseHeldBlockChanged() { Raise(HeldBlockChanged); }
		
		/// <summary> Raised when the block permissions(can place or delete a block) for the player change. </summary>
		public static event EventHandler BlockPermissionsChanged;
		public static void RaiseBlockPermissionsChanged() { Raise(BlockPermissionsChanged); }
		
		/// <summary> Raised when a block definition is changed. </summary>
		public static event EventHandler BlockDefinitionChanged;
		public static void RaiseBlockDefinitionChanged() { Raise(BlockDefinitionChanged); }
		
		
		/// <summary> Raised when message is being added to chat. </summary>
		public static event EventHandler<ChatEventArgs> ChatReceived;	
		public static void RaiseChatReceived(ref string text, MessageType type) { 
			chatArgs.Type = type; chatArgs.Text = text; 
			Raise(ChatReceived, chatArgs); text = chatArgs.Text; }
		
		/// <summary> Raised when user sends a message. </summary>
		public static event EventHandler<ChatEventArgs> ChatSending;
		public static void RaiseChatSending(ref string text) { 
			chatArgs.Type = 0; chatArgs.Text = text; 
			Raise(ChatSending, chatArgs); text = chatArgs.Text; }
		
		/// <summary> Raised when the user changes chat font to arial or back to bitmapped font,
		/// also raised when the bitmapped font changes. </summary>
		public static event EventHandler ChatFontChanged;
		public static void RaiseChatFontChanged() { Raise(ChatFontChanged); }
		
		/// <summary> Raised when the colour codes usable by the player changes. </summary>
		public static event EventHandler<ColourCodeEventArgs> ColCodeChanged;
		public static void RaiseColourCodeChanged(char code) {
			colArgs.Code = code; Raise(ColCodeChanged, colArgs); }
		
		
		/// <summary> Raised when the hack permissions of the player changes. </summary>
		public static event EventHandler HackPermissionsChanged;
		public static void RaiseHackPermissionsChanged() { Raise(HackPermissionsChanged); }
		//BlockChanging event_here; // ??
		
		/// <summary> Raised when the user changes a block in the world. </summary>
		public static event EventHandler<BlockChangedEventArgs> BlockChanged;
		public static void RaiseBlockChanged(Vector3I coords, BlockID old, BlockID block) {
			blockArgs.Coords = coords; blockArgs.OldBlock = old; blockArgs.Block = block; 
			Raise(BlockChanged, blockArgs);
		}
		
		
		/// <summary> Raised when the player joins and begins loading a new world. </summary>
		public static event EventHandler OnNewMap;
		public static void RaiseOnNewMap() { Raise(OnNewMap); }
		
		/// <summary> Raised when a portion of the world is read and decompressed, or generated. </summary>
		public static event EventHandler<LoadingEventArgs> Loading;
		public static void RaiseLoading(float progress) { loadingArgs.Progress = progress; Raise(Loading, loadingArgs); }
		
		/// <summary> Raised when new world has finished loading and the player can now interact with it. </summary>
		public static event EventHandler OnNewMapLoaded;
		public static void RaiseOnNewMapLoaded() { Raise(OnNewMapLoaded); }		
		
		/// <summary> Raised when an environment variable of the world is changed by the user, CPE, or WoM config. </summary>
		public static event EventHandler<EnvVarEventArgs> EnvVariableChanged;
		public static void RaiseEnvVariableChanged(EnvVar envVar) { envArgs.Var = envVar; Raise(EnvVariableChanged, envArgs); }
	
	
		/// <summary> Raised when an entity is spawned in the current world. </summary>
		public static event EventHandler<IdEventArgs> EntityAdded;
		public static void RaiseEntityAdded(byte id) { idArgs.Id = id; Raise(EntityAdded, idArgs); }
		
		/// <summary> Raised when an entity is despawned from the current world. </summary>
		public static event EventHandler<IdEventArgs> EntityRemoved;
		public static void RaiseEntityRemoved(byte id) { idArgs.Id = id; Raise(EntityRemoved, idArgs); }
		
		
		/// <summary> Raised when a tab list entry is created. </summary>
		public static event EventHandler<IdEventArgs> TabListEntryAdded;
		public static void RaiseTabEntryAdded(byte id) { idArgs.Id = id; Raise(TabListEntryAdded, idArgs); }
		
		/// <summary> Raised when a tab list entry is modified. </summary>
		public static event EventHandler<IdEventArgs> TabListEntryChanged;
		public static void RaiseTabListEntryChanged(byte id) { idArgs.Id = id; Raise(TabListEntryChanged, idArgs); }
		
		/// <summary> Raised when a tab list entry is removed. </summary>
		public static event EventHandler<IdEventArgs> TabListEntryRemoved;
		public static void RaiseTabEntryRemoved(byte id) { idArgs.Id = id; Raise(TabListEntryRemoved, idArgs); }
		
		
		static IdEventArgs idArgs = new IdEventArgs();
		static LoadingEventArgs loadingArgs = new LoadingEventArgs();
		static EnvVarEventArgs envArgs = new EnvVarEventArgs();
		static BlockChangedEventArgs blockArgs = new BlockChangedEventArgs();
		static ChatEventArgs chatArgs = new ChatEventArgs();
		static TextureEventArgs texArgs = new TextureEventArgs();
		static ColourCodeEventArgs colArgs = new ColourCodeEventArgs();
		
		static void Raise(EventHandler handler) {
			if (handler != null) handler(null, EventArgs.Empty);
		}
		
		static void Raise<T>(EventHandler<T> handler, T args) where T : EventArgs {
			if (handler != null) handler(null, args);
		}
	}
	
	public sealed class ColourCodeEventArgs : EventArgs { public char Code; }
	public sealed class LoadingEventArgs : EventArgs { public float Progress; }
	public sealed class EnvVarEventArgs : EventArgs { public EnvVar Var; }
	public sealed class IdEventArgs : EventArgs { public byte Id; }
	
	public sealed class ChatEventArgs : EventArgs {
		public MessageType Type;
		public string Text;
	}
	
	public sealed class TextureEventArgs : EventArgs {
		public string Name;
		public byte[] Data;
	}
	
	public sealed class BlockChangedEventArgs : EventArgs {
		public Vector3I Coords;
		public BlockID OldBlock;
		public BlockID Block;
	}
}
