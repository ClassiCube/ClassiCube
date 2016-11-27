// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp.Events {

	public abstract class EventsBase {

		protected void Raise(EventHandler handler) {
			if (handler == null) return;
			handler(this, EventArgs.Empty);
		}
		
		protected void Raise<T>(EventHandler<T> handler, T args) where T : EventArgs {
			if (handler == null) return;
			handler(this, args);
		}
	}
	
	public class OtherEvents : EventsBase {
		
		/// <summary> Raised when the terrain atlas ("terrain.png") is changed. </summary>
		public event EventHandler TerrainAtlasChanged;
		internal void RaiseTerrainAtlasChanged() { Raise(TerrainAtlasChanged); }
		
		/// <summary> Raised when the texture pack is changed. </summary>
		public event EventHandler TexturePackChanged;
		internal void RaiseTexturePackChanged() { Raise(TexturePackChanged); }
		
		/// <summary> Raised when a texture is changed. (such as "terrain", "rain") </summary>
		public event EventHandler<TextureEventArgs> TextureChanged;
		internal void RaiseTextureChanged(string name, byte[] data) {
			texArgs.Name = name; texArgs.Data = data; Raise(TextureChanged, texArgs); }
		
		/// <summary> Raised when the user changed their view/fog distance. </summary>
		public event EventHandler ViewDistanceChanged;
		internal void RaiseViewDistanceChanged() { Raise(ViewDistanceChanged); }
		
		/// <summary> Raised when the held block is changed by the user or by CPE. </summary>
		public event EventHandler HeldBlockChanged;
		internal void RaiseHeldBlockChanged() { Raise(HeldBlockChanged); }
		
		/// <summary> Raised when the block permissions(can place or delete a block) for the player change. </summary>
		public event EventHandler BlockPermissionsChanged;
		internal void RaiseBlockPermissionsChanged() { Raise(BlockPermissionsChanged); }
		
		/// <summary> Raised when a block definition is changed. </summary>
		public event EventHandler BlockDefinitionChanged;
		internal void RaiseBlockDefinitionChanged() { Raise(BlockDefinitionChanged); }
		
		/// <summary> Raised when the server or a client-side command sends a message. </summary>
		public event EventHandler<ChatEventArgs> ChatReceived;	
		internal void RaiseChatReceived(string text, MessageType type) { 
			chatArgs.Type = type; chatArgs.Text = text; Raise(ChatReceived, chatArgs); }
		
		/// <summary> Raised when the user changes chat font to arial or back to bitmapped font,
		/// also raised when the bitmapped font changes. </summary>
		public event EventHandler ChatFontChanged;
		internal void RaiseChatFontChanged() { Raise(ChatFontChanged); }
		
		
		/// <summary> Raised when the hack permissions of the player changes. </summary>
		public event EventHandler HackPermissionsChanged;
		internal void RaiseHackPermissionsChanged() { Raise(HackPermissionsChanged); }
		
		/// <summary> Raised when the colour codes usable by the player changes. </summary>
		public event EventHandler<ColourCodeEventArgs> ColourCodeChanged;
		internal void RaiseColourCodeChanged(char code) {
			colArgs.Code = code; Raise(ColourCodeChanged, colArgs); }
		
		/// <summary> Raised when the projection matrix changes. </summary>
		public event EventHandler ProjectionChanged;
		internal void RaiseProjectionChanged() { Raise(ProjectionChanged); }
	
		ChatEventArgs chatArgs = new ChatEventArgs();
		TextureEventArgs texArgs = new TextureEventArgs();
		ColourCodeEventArgs colArgs = new ColourCodeEventArgs();
	}
	
	public sealed class ChatEventArgs : EventArgs {
		
		/// <summary> Where this chat message should appear on the screen. </summary>
		public MessageType Type;
		
		/// <summary> Raw text of the message (including colour codes), 
		/// with code page 437 indices converted to their unicode representations. </summary>
		public string Text;
	}
	
	public sealed class ColourCodeEventArgs : EventArgs {
		
		/// <summary> ASCII colour code that was changed. </summary>
		public char Code;
	}
	
	public sealed class TextureEventArgs : EventArgs {
		
		/// <summary> Location of the file within a texture pack, without a directory. (e.g. "snow.png") </summary>
		public string Name;
		
		/// <summary> Raw data of the file. </summary>
		public byte[] Data;
	}
}
