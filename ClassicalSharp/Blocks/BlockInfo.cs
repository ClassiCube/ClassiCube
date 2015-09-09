using System;

namespace ClassicalSharp {
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public partial class BlockInfo {
		
		internal bool[] isTransparent = new bool[BlocksCount];
		internal bool[] isTranslucent = new bool[BlocksCount];
		internal bool[] isOpaque = new bool[BlocksCount];
		internal bool[] isSprite = new bool[BlocksCount];
		internal bool[] isLiquid = new bool[BlocksCount];
		internal float[] heights = new float[BlocksCount];
		internal bool[] blocksLight = new bool[BlocksCount];
		internal bool[] emitsLight = new bool[BlocksCount];
		internal string[] names = new string[BlocksCount];
		internal FastColour[] fogColours = new FastColour[BlocksCount];
		internal float[] fogDensities = new float[BlocksCount];
		
		public const byte MaxDefinedCpeBlock = (byte)Block.StoneBrick;
		public const int CpeBlocksCount = MaxDefinedCpeBlock + 1;
		public const byte MaxDefinedBlock = byte.MaxValue;
		public const int BlocksCount = MaxDefinedBlock + 1;
		
		public void Init() {
			for( int tile = 1; tile < BlocksCount; tile++ ) {
				heights[tile] = 1f;
				blocksLight[tile] = true;
				isOpaque[tile] = true;
			}
			for( int i = 0; i < CpeBlocksCount; i++ ) {
				names[i] = Enum.GetName( typeof( Block ), (byte)i );
			}
			for( int i = CpeBlocksCount; i < BlocksCount; i++ ) {
				names[i] = "Invalid";
			}
			
			fogDensities[(byte)Block.StillWater] = 0.1f;
			fogColours[(byte)Block.StillWater] = new FastColour( 5, 5, 51 );
			fogDensities[(byte)Block.Water] = 0.1f;
			fogColours[(byte)Block.Water] = new FastColour( 5, 5, 51 );
			fogDensities[(byte)Block.StillLava] = 2f;
			fogColours[(byte)Block.StillLava] = new FastColour( 153, 25, 0 );
			fogDensities[(byte)Block.Lava] = 2f;
			fogColours[(byte)Block.Lava] = new FastColour( 153, 25, 0 );
			
			SetupTextures();
			
			SetBlockHeight( Block.Slab, 8/16f );
			SetBlockHeight( Block.CobblestoneSlab, 8/16f );
			SetBlockHeight( Block.Snow, 2/16f );			
			MarkTranslucent( Block.StillWater ); MarkTranslucent( Block.Water ); 
			MarkTranslucent( Block.Ice );
			MarkTransparent( Block.Glass ); MarkTransparent( Block.Leaves ); 
			MarkTransparent( Block.Slab ); MarkTransparent( Block.Snow ); 
			MarkTransparent( Block.CobblestoneSlab );
			MarkSprite( Block.Rose ); MarkSprite( Block.Sapling ); 
			MarkSprite( Block.Dandelion ); MarkSprite( Block.BrownMushroom ); 
			MarkSprite( Block.RedMushroom ); MarkSprite( Block.Rope ); 
			MarkSprite( Block.Fire );
			SetIsLiquid( Block.StillWater ); SetIsLiquid( Block.Water );
			SetIsLiquid( Block.StillLava ); SetIsLiquid( Block.Lava );					
			SetEmitsLight( Block.Lava, true ); SetEmitsLight( Block.StillLava, true );
			SetupCullingCache();
		}
		
		public void SetDefaultBlockPermissions( bool[] canPlace, bool[] canDelete ) {
			for( int tile = (int)Block.Stone; tile <= (int)Block.Obsidian; tile++ ) {
				canPlace[tile] = true;
				canDelete[tile] = true;
			}
			canPlace[(int)Block.Grass] = false;
			canPlace[(int)Block.Lava] = false;
			canPlace[(int)Block.Water] = false;
			canPlace[(int)Block.StillLava] = false;
			canPlace[(int)Block.StillWater] = false;
			canPlace[(int)Block.Bedrock] = false;
			
			canDelete[(int)Block.Bedrock] = false;
			canDelete[(int)Block.Lava] = false;
			canDelete[(int)Block.Water] = false;
			canDelete[(int)Block.StillWater] = false;
			canDelete[(int)Block.StillLava] = false;
		}
		
		void MarkTransparent( Block id ) {
			isTransparent[(int)id] = true;
			blocksLight[(int)id] = false;
			isOpaque[(int)id] = false;			
		}
		
		void MarkSprite( Block id ) {
			isSprite[(int)id] = true;
			isTransparent[(int)id] = true;
			blocksLight[(int)id] = false;
			isOpaque[(int)id] = false;
		}
		
		void MarkTranslucent( Block id ) {
			isTranslucent[(int)id] = true;
			isOpaque[(int)id] = false;
		}
		
		void SetIsLiquid( Block id ) {
			isLiquid[(int)id] = true;
		}
		
		void SetBlockHeight( Block id, float height ) {
			heights[(int)id] = height;
		}
		
		void SetBlocksLight( Block id, bool blocks ) {
			blocksLight[(int)id] = blocks;
		}
		
		void SetEmitsLight( Block id, bool emits ) {
			emitsLight[(int)id] = emits;
		}
		
		/// <summary> Gets whether the given block id is opaque/not see through. </summary>
		public bool IsOpaque( byte id ) {
			return isOpaque[id];
		}
		
		/// <summary> Gets whether the given block id is opaque/not see through, and occupies a full block. </summary>
		public bool IsFullAndOpaque( byte id ) {
			return isOpaque[id] && heights[id] == 1;
		}
		
		/// <summary> Gets whether the given block id is transparent/fully see through. </summary>
		/// <remarks> Alpha values are treated as either 'fully see through' or 'fully solid'. </remarks>
		public bool IsTransparent( byte id ) {
			return isTransparent[id];
		}
		
		/// <summary> Gets the tile height of the given block id. </summary>
		public float BlockHeight( byte id ) {
			return heights[id];
		}
		
		/// <summary> Gets whether the given block id is translucent/partially see through. </summary>
		/// <remarks>Colour values are blended into both the transparent and opaque blocks behind them. </remarks>
		public bool IsTranslucent( byte id ) {
			return isTranslucent[id];
		}
		
		/// <summary> Gets whether the given block blocks sunlight. </summary>
		public bool BlocksLight( byte id ) {
			return blocksLight[id];
		}
		
		/// <summary> Gets whether the given block id is a sprite. (flowers, saplings, fire, etc) </summary>
		public bool IsSprite( byte id ) {
			return isSprite[id];
		}
		
		/// <summary> Gets whether the given block id is a liquid. (water or lava) </summary>
		public bool IsLiquid( byte id ) {
			return isLiquid[id];
		}
		
		public bool EmitsLight( byte id ) {
			return emitsLight[id];
		}
		
		public float FogDensity( byte id ) {
			return fogDensities[id];
		}
		
		public FastColour FogColour( byte id ) {
			return fogColours[id];
		}
		
		public string GetName( byte id ) {
			return names[id];
		}
		
		public void ResetBlockInfo( byte id ) {
			isTransparent[id] = false;
			isTranslucent[id] = false;
			isOpaque[id] = true;
			isSprite[id] = false;
			isLiquid[id] = false;
			heights[id] = 1;
			blocksLight[id] = true;
			emitsLight[id] = true;
			names[id] = "Invalid";
			fogColours[id] = default( FastColour );
			fogDensities[id] = 0;
			SetAll( 0, (Block)id );
			SetupCullingCache();
		}
	}
}