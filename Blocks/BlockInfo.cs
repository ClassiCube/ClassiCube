using System;
using ClassicalSharp.Blocks.Model;

namespace ClassicalSharp {
	
	public partial class BlockInfo {
		
		bool[] isTransparent = new bool[blocksCount];
		bool[] isTranslucent = new bool[blocksCount];
		bool[] isOpaque = new bool[blocksCount];
		bool[] isSprite = new bool[blocksCount];
		bool[] isLiquid = new bool[blocksCount];
		float[] heights = new float[blocksCount];
		float[] hardness = new float[blocksCount];
		bool[] blocksLight = new bool[blocksCount];
		IBlockModel[] models = new IBlockModel[blocksCount];
		const int blocksCount = 256;
		
		public void Init( TextureAtlas2D atlas, Game game ) {
			for( int i = 1; i < blocksCount; i++ ) {
				heights[i] = 1;
				blocksLight[i] = true;
				isOpaque[i] = true;
			}
			SetupOptimTextures();
			
			SetIsTranslucent( BlockId.StillWater, BlockId.Water, BlockId.Ice );
			SetIsTransparent( BlockId.Glass, BlockId.Leaves, BlockId.Slabs, BlockId.Snow,
			                 BlockId.MonsterSpawner, BlockId.Bed, BlockId.Farmland, BlockId.Fence,
			                 BlockId.Rails );
			SetIsSprite( BlockId.Rose, BlockId.Sapling, BlockId.Dandelion,
			            BlockId.BrownMushroom, BlockId.RedMushroom, BlockId.Torch,
			            BlockId.DeadShrubs, BlockId.TallGrass, BlockId.Fire, BlockId.RedstoneWire,
			            BlockId.RedstoneTorchOn, BlockId.RedstoneTorchOff, BlockId.SugarCane,
			            BlockId.Cobweb, BlockId.Lever );
			
			SetBlocksLight( false, BlockId.Glass, BlockId.Leaves, BlockId.Sapling,
			               BlockId.RedMushroom, BlockId.BrownMushroom, BlockId.Rose,
			               BlockId.Dandelion, BlockId.Fire );
			SetIsLiquid( BlockId.StillWater, BlockId.Water, BlockId.StillLava, BlockId.Lava );
			SetBlockHeight( 8 / 16f, BlockId.Slabs );
			SetBlockHeight( 2 / 16f, BlockId.Snow );
			SetBlockHeight( 9 / 16f, BlockId.Bed );
			SetBlockHeight( 3 / 16f, BlockId.Trapdoor );
			SetBlockHeight( 15 / 16f, BlockId.Farmland );
			SetupCullingCache();
			MakeColours();
			
			models[(byte)BlockId.Seeds] = new SeedsModel( game, (byte)BlockId.Seeds );
			models[(byte)BlockId.Grass] = new GrassCubeModel( game, (byte)BlockId.Grass );
			models[(byte)BlockId.Fence] = new FenceModel( game, (byte)BlockId.Fence );
			models[(byte)BlockId.Torch] = new TorchModel( game, (byte)BlockId.Torch );
			models[(byte)BlockId.RedstoneTorchOff] = new TorchModel( game, (byte)BlockId.RedstoneTorchOff );
			models[(byte)BlockId.RedstoneTorchOn] = new TorchModel( game, (byte)BlockId.RedstoneTorchOn );
			models[(byte)BlockId.Lever] = new LeverModel( game, (byte)BlockId.Lever );
			models[(byte)BlockId.Rails] = new RailsModel( game, (byte)BlockId.Rails );
			models[(byte)BlockId.Cactus] = new CactusModel( game, (byte)BlockId.Cactus );
			models[(byte)BlockId.TallGrass] = new BiomeColouredModel( new SpriteModel( game, (byte)BlockId.TallGrass ) );
			models[(byte)BlockId.Leaves] = new BiomeColouredModel( new CubeModel( game, (byte)BlockId.Leaves ) );
			models[(byte)BlockId.Sapling] = new SaplingModel( game );
			models[(byte)BlockId.Water] = new FluidModel( game, (byte)BlockId.Water );
			models[(byte)BlockId.StillWater] = new FluidModel( game, (byte)BlockId.StillWater );
			models[(byte)BlockId.Lava] = new FluidModel( game, (byte)BlockId.Lava );
			models[(byte)BlockId.StillLava] = new FluidModel( game, (byte)BlockId.StillLava );
			
			for( byte id = 1; id <= 96; id++ ) {
				if( models[id] != null ) continue;
				if( IsSprite( id ) ) {
					models[id] = new SpriteModel( game, id );
				} else {
					models[id] = new CubeModel( game, id );
				}
			}
			SetHardnesses(
				inf, 1.5f, 0.6f, 0.5f, 2f, 2f, 0f, 1E10f, inf, inf, inf, inf,
				0.5f, 0.6f, 3f, 3f, 3f, 2f, 0.2f, 0.6f, 0.3f, 3f, 3f, 3.5f, 0.8f,
				0.8f, 0.2f, 0.7f, 0.7f, 0.5f, 4f, 0f, 0f, 0.5f, 0.5f, 0.8f, 0f,
				0f, 0f, 0f, 0f, 3f, 5f, 2f, 2f, 2f, 0f, 1.5f, 2f, 10f, 0f, 0f,
				5f, 2f, 2.5f, 0f, 3f, 5f, 2.5f, 0f, 0.6f, 3.5f, 3.5f, 1f, 3f,
				0.4f, 0.7f, 2f, 1f, 0.5f, 0.5f, 5f, 0.5f, 3f, 3f, 0f, 0f, 0.5f,
				0.1f, 0.5f, 0.2f, 0.4f, 0.6f, 0f, 2f, 2f, 1f, 0.4f, 0.5f, 0.3f,
				1E10f, 1f, 0.5f, 0f, 0f, 0f, 3f );
		}
		const float inf = float.PositiveInfinity;
		
		void SetIsTransparent( params BlockId[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				isTransparent[(int)ids[i]] = true;
				isOpaque[(int)ids[i]] = false;
			}
		}
		
		void SetIsSprite( params BlockId[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				isSprite[(int)ids[i]] = true;
				isTransparent[(int)ids[i]] = true;
				isOpaque[(int)ids[i]] = false;
			}
		}
		
		void SetIsTranslucent( params BlockId[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				isTranslucent[(int)ids[i]] = true;
				isOpaque[(int)ids[i]] = false;
			}
		}
		
		void SetIsLiquid( params BlockId[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				isLiquid[(int)ids[i]] = true;
			}
		}
		
		void SetBlockHeight( float height, params BlockId[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				heights[(int)ids[i]] = height;
			}
		}
		
		void SetHardnesses( params float[] hardnesses ) {
			for( int i = 0; i < hardnesses.Length; i++ ) {
				hardness[i] = hardnesses[i];
			}
		}
		
		void SetBlocksLight( bool blocks, params BlockId[] ids ) {
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
		
		public float Hardness( byte id ) {
			return hardness[id];
		}
		
		public bool CanPick( byte id ) {
			return id != 0;
			return hardness[id] != inf;
		}
		
		public IBlockModel GetModel( byte id ) {
			return models[id];
		}
	}
}