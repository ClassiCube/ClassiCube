using System;

namespace ClassicalSharp {
	
	public partial class BlockInfo {
			
		bool[] isTransparent = new bool[BlocksCount];
		bool[] isTranslucent = new bool[BlocksCount];
		bool[] isOpaque = new bool[BlocksCount];
		bool[] isSprite = new bool[BlocksCount];
		bool[] isLiquid = new bool[BlocksCount];
		float[] heights = new float[BlocksCount];
		bool[] blocksLight = new bool[BlocksCount];		
		public const byte MaxDefinedBlock = (byte)Block.StoneBrick;
		public const byte BlocksCount = MaxDefinedBlock + 1;
		
		public void Init() {
			for( int tile = 1; tile < BlocksCount; tile++ ) {
				heights[tile] = 1f;
				blocksLight[tile] = true;
				isOpaque[tile] = true;				
			}
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
		
		/// <summary> Gets whether the given block id is opaque/not see through. </summary>
		public bool IsOpaque( byte id ) {
			return isOpaque[id];
		}
		
		/// <summary> Gets whether the given block id is opaque/not see through,
		/// and occupies a full block. </summary>
		public bool IsFullAndOpaque( byte id ) {
			return isOpaque[id] && heights[id] == 1;
		}
		
		/// <summary> Gets whether the given block id is transparent/fully see through. </summary>
		/// <remarks> Note that these blocks's alpha values are treated as either 'fully see through'
		/// or 'fully solid'. </remarks>
		public bool IsTransparent( byte id ) {
			return isTransparent[id];
		}
		
		/// <summary> Gets the tile height of the given block id. </summary>
		public float BlockHeight( byte id ) {
			return heights[id];
		}
		
		/// <summary> Gets whether the given block id is translucent/partially see through. </summary>
		/// <remarks> Note that these blocks's colour values are blended into both
		/// the transparent and opaque blocks behind them. </remarks>
		public bool IsTranslucent( byte id ) {
			return isTranslucent[id];
		}
		
		/// <summary> Gets whether the given block blocks sunlight. </summary>
		public bool BlocksLight( byte id ) {
			return blocksLight[id];
		}
		
		/// <summary> Gets whether the given block id is a sprite. <br/>
		/// (flowers, saplings, fire, etc) </summary>
		public bool IsSprite( byte id ) {
			return isSprite[id];
		}
		
		/// <summary> Gets whether the given block id is a liquid. <br/>
		/// (water or lava) </summary>
		public bool IsLiquid( byte id ) {
			return isLiquid[id];
		}
	}
}