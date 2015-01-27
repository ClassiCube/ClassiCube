using System;

namespace ClassicalSharp {
	
	public partial class BlockInfo {
			
		bool[] isTransparent = new bool[blocksCount];
		bool[] isTranslucent = new bool[blocksCount];
		bool[] isOpaque = new bool[blocksCount];
		bool[] isSprite = new bool[blocksCount];
		bool[] isLiquid = new bool[blocksCount];
		float[] heights = new float[blocksCount];
		bool[] blocksLight = new bool[blocksCount];	
		const int blocksCount = 256;
		
		public void Init() {
			for( int i = 1; i < blocksCount; i++ ) {
				heights[i] = 1;
				blocksLight[i] = true;
				isOpaque[i] = true;				
			}
			SetupOptimTextures();
			
			SetIsTranslucent( Block.StillWater, Block.Water, Block.Ice );
			SetIsTransparent( Block.Glass, Block.Leaves, Block.Sapling,
			                 Block.RedMushroom, Block.BrownMushroom, Block.Rose,
			                 Block.Dandelion, Block.Slabs );
			SetIsSprite( Block.Rose, Block.Sapling, Block.Dandelion,
			            Block.BrownMushroom, Block.RedMushroom );
			SetBlockHeight( 0.5f, Block.Slabs );
			SetBlocksLight( false, Block.Glass, Block.Leaves, Block.Sapling,
			               Block.RedMushroom, Block.BrownMushroom, Block.Rose,
			               Block.Dandelion );
			SetIsLiquid( Block.StillWater, Block.Water, Block.StillLava, Block.Lava );
					
			SetIsTransparent( Block.Snow, Block.Fire );
			SetBlocksLight( false, Block.Fire );
			SetIsSprite( Block.Fire );
			SetBlockHeight( 0.125f, Block.Snow );
		}
		
		public void SetDefaultBlockPermissions( bool[] canPlace, bool[] canDelete ) {
			for( int i = (int)Block.Stone; i <= (int)Block.Obsidian; i++ ) {
				canPlace[i] = true;
				canDelete[i] = true;
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