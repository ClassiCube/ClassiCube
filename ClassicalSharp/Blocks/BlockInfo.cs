// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public partial class BlockInfo {
		
		/// <summary> Gets whether the given block id is transparent/fully see through. </summary>
		/// <remarks> Alpha values are treated as either 'fully see through' or 'fully solid'. </remarks>
		public bool[] IsTransparent = new bool[BlocksCount];
		
		/// <summary> Gets whether the given block id is translucent/partially see through. </summary>
		/// <remarks>Colour values are blended into both the transparent and opaque blocks behind them. </remarks>
		public bool[] IsTranslucent = new bool[BlocksCount];
		
		/// <summary> Gets whether the given block id is opaque/not partially see through. </summary>
		public bool[] IsOpaque = new bool[BlocksCount];
		
		/// <summary> Gets whether the given block id is opaque/not partially see through on the y axis. </summary>
		public bool[] IsOpaqueY = new bool[BlocksCount];
		
		/// <summary> Gets whether the given block id is a sprite. (e.g. flowers, saplings, fire) </summary>
		public bool[] IsSprite = new bool[BlocksCount];
		
		/// <summary> Gets whether the given block id is a liquid. (e.g. water and lava) </summary>
		public bool[] IsLiquid = new bool[BlocksCount];
		
		/// <summary> Gets whether the given block blocks sunlight. </summary>
		public bool[] BlocksLight = new bool[BlocksCount];
		
		/// <summary> Gets whether the given block should draw all it faces with a full white colour component. </summary>
		public bool[] FullBright = new bool[BlocksCount];
		
		public string[] Name = new string[BlocksCount];
		
		/// <summary> Gets the custom fog colour that should be used when the player is standing within this block.
		/// Note that this is only used for exponential fog mode. </summary>
		public FastColour[] FogColour = new FastColour[BlocksCount];
		
		public float[] FogDensity = new float[BlocksCount];
		
		public BlockCollideType[] CollideType = new BlockCollideType[BlocksCount];
		
		public float[] SpeedMultiplier = new float[BlocksCount];
		
		public bool[] CullWithNeighbours = new bool[BlocksCount];
		
		public byte[] LightOffset = new byte[BlocksCount];
		
		public uint[] DefinedCustomBlocks = new uint[BlocksCount >> 5];
		
		public const byte MaxDefinedOriginalBlock = (byte)Block.Obsidian;
		public const int OriginalBlocksCount = MaxDefinedOriginalBlock + 1;
		public const byte MaxDefinedCpeBlock = (byte)Block.StoneBrick;
		public const int CpeBlocksCount = MaxDefinedCpeBlock + 1;
		public const byte MaxDefinedBlock = byte.MaxValue;
		public const int BlocksCount = MaxDefinedBlock + 1;
		
		public void Init() {
			for( int tile = 1; tile < BlocksCount; tile++ ) {
				MaxBB[tile].Y = 1;
				BlocksLight[tile] = true;
				IsOpaque[tile] = true;
				IsOpaqueY[tile] = true;
				CollideType[tile] = BlockCollideType.Solid;
				SpeedMultiplier[tile] = 1;
				CullWithNeighbours[tile] = true;
			}
			for( int block = 0; block < BlocksCount; block++ )
				Name[block] = "Invalid";
			
			FogDensity[(byte)Block.StillWater] = 0.1f;
			FogColour[(byte)Block.StillWater] = new FastColour( 5, 5, 51 );
			FogDensity[(byte)Block.Water] = 0.1f;
			FogColour[(byte)Block.Water] = new FastColour( 5, 5, 51 );
			FogDensity[(byte)Block.StillLava] = 2f;
			FogColour[(byte)Block.StillLava] = new FastColour( 153, 25, 0 );
			FogDensity[(byte)Block.Lava] = 2f;
			FogColour[(byte)Block.Lava] = new FastColour( 153, 25, 0 );
			CollideType[(byte)Block.Snow] = BlockCollideType.WalkThrough;
			SpeedMultiplier[0] = 1f;
			CullWithNeighbours[(byte)Block.Leaves] = false;
			SetupTextures();
			
			SetBlockHeight( Block.Slab, 8/16f );
			SetBlockHeight( Block.CobblestoneSlab, 8/16f );
			SetBlockHeight( Block.Snow, 2/16f );
			MarkTranslucent( Block.StillWater ); MarkTranslucent( Block.Water );
			MarkTranslucent( Block.Ice );
			MarkTransparent( Block.Glass, false ); MarkTransparent( Block.Leaves, false );
			MarkTransparent( Block.Slab, true ); MarkTransparent( Block.Snow, true );
			MarkTransparent( Block.CobblestoneSlab, true );
			MarkSprite( Block.Rose ); MarkSprite( Block.Sapling );
			MarkSprite( Block.Dandelion ); MarkSprite( Block.BrownMushroom );
			MarkSprite( Block.RedMushroom ); MarkSprite( Block.Rope );
			MarkSprite( Block.Fire );
			SetIsLiquid( Block.StillWater ); SetIsLiquid( Block.Water );
			SetIsLiquid( Block.StillLava ); SetIsLiquid( Block.Lava );
			SetFullBright( Block.Lava, true ); SetFullBright( Block.StillLava, true );
			SetFullBright( Block.Magma, true ); SetFullBright( Block.Fire, true );
			
			IsOpaqueY[(byte)Block.Slab] = true;
			IsOpaqueY[(byte)Block.CobblestoneSlab] = true;
			IsOpaqueY[(byte)Block.Snow] = true;

			InitBoundingBoxes();
			InitSounds();			
			SetupCullingCache();
			InitLightOffsets();
		}

		public void SetDefaultBlockPermissions( InventoryPermissions canPlace, InventoryPermissions canDelete ) {
			for( int tile = (int)Block.Stone; tile <= (int)Block.Obsidian; tile++ ) {
				canPlace[tile] = true;
				canDelete[tile] = true;
			}
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
		
		void MarkTransparent( Block id, bool blocks ) {
			IsTransparent[(int)id] = true;
			BlocksLight[(int)id] = blocks;
			IsOpaque[(int)id] = false;
			IsOpaqueY[(int)id] = false;
		}
		
		void MarkSprite( Block id ) {
			IsSprite[(int)id] = true;
			IsTransparent[(int)id] = true;
			BlocksLight[(int)id] = false;
			IsOpaque[(int)id] = false;
			IsOpaqueY[(int)id] = false;
			CollideType[(int)id] = BlockCollideType.WalkThrough;
		}
		
		void MarkTranslucent( Block id ) {
			IsTranslucent[(int)id] = true;
			IsOpaque[(int)id] = false;
			IsOpaqueY[(int)id] = false;
		}
		
		void SetIsLiquid( Block id ) {
			IsLiquid[(int)id] = true;
			CollideType[(int)id] = BlockCollideType.SwimThrough;
		}
		
		void SetBlockHeight( Block id, float height ) {
			MaxBB[(int)id].Y = height;
		}
		
		void SetFullBright( Block id, bool emits ) {
			FullBright[(int)id] = emits;
		}
		
		public void ResetBlockInfo( byte id, bool updateCulling ) {
			DefinedCustomBlocks[id >> 5] &= ~(1u << (id & 0x1F));
			IsTransparent[id] = false;
			IsTranslucent[id] = false;
			IsOpaque[id] = true;
			IsSprite[id] = false;
			IsLiquid[id] = false;
			BlocksLight[id] = true;
			FullBright[id] = false;
			CullWithNeighbours[id] = true;
			IsAir[id] = false;
			
			Name[id] = "Invalid";
			FogColour[id] = default( FastColour );
			FogDensity[id] = 0;
			CollideType[id] = BlockCollideType.Solid;
			SpeedMultiplier[id] = 1;
			SetAll( 0, (Block)id );
			if( updateCulling )
				SetupCullingCache( id );
			MinBB[id] = Vector3.Zero;
			MaxBB[id] = Vector3.One;
			StepSounds[id] = SoundType.None;
			DigSounds[id] = SoundType.None;
		}
	}
	
	public enum BlockCollideType : byte {
		WalkThrough, // i.e. gas or sprite
		SwimThrough, // i.e. liquid
		Solid,       // i.e. solid
	}
}