// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp.Events {

	public class UserEvents {
		
		/// <summary> Raised when the user changes a block in the world. </summary>
		public event EventHandler<BlockChangedEventArgs> BlockChanged;
		internal void RaiseBlockChanged( Vector3I coords, byte old, byte block ) {
			blockArgs.Coords = coords; blockArgs.OldBlock = old; blockArgs.Block = block; 
			Raise( BlockChanged, blockArgs );
		}
	
		BlockChangedEventArgs blockArgs = new BlockChangedEventArgs();
		protected void Raise( EventHandler handler ) {
			if( handler != null )
				handler( this, EventArgs.Empty );
		}
		
		protected void Raise<T>( EventHandler<T> handler, T args ) where T : EventArgs {
			if( handler != null )
				handler( this, args );
		}
	}
	
	public sealed class BlockChangedEventArgs : EventArgs {
		
		/// <summary> Location within the world the block was updated at. </summary>
		public Vector3I Coords;
		
		/// <summary> Block ID that was at the given location before.  </summary>
		public byte OldBlock;
		
		/// <summary> Block ID that is now at the given location. </summary>
		public byte Block;
	}
}
