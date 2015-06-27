using System;

namespace ClassicalSharp {
	
	public abstract class BlockInfo {
		
		public BlockInfo( Game window ) {
		}
		
		public abstract byte BlocksCount { get; }
		
		public abstract void Init();
		
		public abstract void SetDefaultBlockPermissions( bool[] canPlace, bool[] canDelete );
		
		/// <summary> Gets whether the given block id is opaque/not see through. </summary>
		public abstract bool IsOpaque( byte id );
		
		/// <summary> Gets whether the given block id is opaque/not see through,
		/// and occupies a full block. </summary>
		public abstract bool IsFullAndOpaque( byte id );
		
		/// <summary> Gets whether the given block id is transparent/fully see through. </summary>
		/// <remarks> Note that these blocks's alpha values are treated as either 'fully see through'
		/// or 'fully solid'. </remarks>
		public abstract bool IsTransparent( byte id );
		
		/// <summary> Gets the tile height of the given block id. </summary>
		public abstract float BlockHeight( byte id );
		
		/// <summary> Gets whether the given block id is translucent/partially see through. </summary>
		/// <remarks> Note that these blocks's colour values are blended into both
		/// the transparent and opaque blocks behind them. </remarks>
		public abstract bool IsTranslucent( byte id );
		
		/// <summary> Gets whether the given block blocks sunlight. </summary>
		public abstract bool BlocksLight( byte id );
		
		/// <summary> Gets whether the given block id is a sprite. <br/>
		/// (flowers, saplings, fire, etc) </summary>
		public abstract bool IsSprite( byte id );
		
		/// <summary> Gets whether the given block id is a liquid. <br/>
		/// (water or lava) </summary>
		public abstract bool IsLiquid( byte id );
		
		public abstract bool IsFaceHidden( byte tile, byte block, int tileSide );
		
		/// <summary> Gets the index in the ***optimised*** 2D terrain atlas for the
		/// texture of the face of the given block. </summary>
		/// <param name="block"> Block ID. </param>
		/// <param name="face">Face of the given block, see TileSide constants. </param>
		/// <returns>The index of the texture within the terrain atlas.</returns>
		public abstract int GetOptimTextureLoc( byte block, int face );
	}
}