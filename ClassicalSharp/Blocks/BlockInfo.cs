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
			
			SetupOptimTextures();
			
			SetIsTranslucent( Block.StillWater, Block.Water );
			SetIsTransparent( Block.Glass, Block.Leaves, Block.Sapling,
			                 Block.RedMushroom, Block.BrownMushroom, Block.Rose,
			                 Block.Dandelion, Block.Slab );
			SetIsSprite( Block.Rose, Block.Sapling, Block.Dandelion,
			            Block.BrownMushroom, Block.RedMushroom );
			SetBlockHeight( 8 / 16f, Block.Slab );
			SetBlocksLight( false, Block.Glass, Block.Leaves, Block.Sapling,
			               Block.RedMushroom, Block.BrownMushroom, Block.Rose,
			               Block.Dandelion );
			SetIsLiquid( Block.StillWater, Block.Water, Block.StillLava, Block.Lava );
					
			SetIsTransparent( Block.Snow, Block.Fire, Block.Rope, Block.CobblestoneSlab );
			SetBlocksLight( false, Block.Fire, Block.Rope );
			SetIsTranslucent( Block.Ice );
			SetIsSprite( Block.Rope, Block.Fire );
			SetBlockHeight( 8 / 16f, Block.CobblestoneSlab );
			SetBlockHeight( 2 / 16f, Block.Snow );
			SetEmitsLight( true, Block.Lava, Block.StillLava );
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
		
		void SetIsTransparent( params Block[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				isTransparent[(int)ids[i]] = true;
				isOpaque[(int)ids[i]] = false;
			}
		}
		
		void SetIsSprite( params Block[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				isSprite[(int)ids[i]] = true;
			}
		}
		
		void SetIsTranslucent( params Block[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				isTranslucent[(int)ids[i]] = true;
				isOpaque[(int)ids[i]] = false;
			}
		}
		
		void SetIsLiquid( params Block[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				isLiquid[(int)ids[i]] = true;
			}
		}
		
		void SetBlockHeight( float height, params Block[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				heights[(int)ids[i]] = height;
			}
		}
		
		void SetBlocksLight( bool blocks, params Block[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				blocksLight[(int)ids[i]] = blocks;
			}
		}
		
		void SetEmitsLight( bool emits, params Block[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				emitsLight[(int)ids[i]] = emits;
			}
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
	}
}