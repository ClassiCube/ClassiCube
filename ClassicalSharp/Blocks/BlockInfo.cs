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
		
		public CollideType[] Collide = new CollideType[BlocksCount];
		
		public float[] SpeedMultiplier = new float[BlocksCount];
		
		public bool[] CullWithNeighbours = new bool[BlocksCount];
		
		public byte[] LightOffset = new byte[BlocksCount];
		
		public uint[] DefinedCustomBlocks = new uint[BlocksCount >> 5];
		
		public const byte MaxOriginalBlock = Block.Obsidian;
		public const int OriginalCount = MaxOriginalBlock + 1;
		public const byte MaxCpeBlock = Block.StoneBrick;
		public const int CpeCount = MaxCpeBlock + 1;
		public const byte MaxDefinedBlock = byte.MaxValue;
		public const int BlocksCount = MaxDefinedBlock + 1;
		
		public void Reset( Game game ) {
			// Reset properties
			for( int i = 0; i < IsTransparent.Length; i++ ) IsTransparent[i] = false;
			for( int i = 0; i < IsTranslucent.Length; i++ ) IsTranslucent[i] = false;
			for( int i = 0; i < IsOpaque.Length; i++ ) IsOpaque[i] = false;
			for( int i = 0; i < IsOpaqueY.Length; i++ ) IsOpaqueY[i] = false;
			for( int i = 0; i < IsSprite.Length; i++ ) IsSprite[i] = false;
			for( int i = 0; i < IsLiquid.Length; i++ ) IsLiquid[i] = false;
			for( int i = 0; i < BlocksLight.Length; i++ ) BlocksLight[i] = false;
			for( int i = 0; i < FullBright.Length; i++ ) FullBright[i] = false;
			for( int i = 0; i < Name.Length; i++ ) Name[i] = "Invalid";
			for( int i = 0; i < FogColour.Length; i++ ) FogColour[i] = default(FastColour);
			for( int i = 0; i < FogDensity.Length; i++ ) FogDensity[i] = 0;
			for( int i = 0; i < Collide.Length; i++ ) Collide[i] = CollideType.WalkThrough;
			for( int i = 0; i < SpeedMultiplier.Length; i++ ) SpeedMultiplier[i] = 0;
			for( int i = 0; i < CullWithNeighbours.Length; i++ ) CullWithNeighbours[i] = false;
			for( int i = 0; i < LightOffset.Length; i++ ) LightOffset[i] = 0;
			for( int i = 0; i < DefinedCustomBlocks.Length; i++ ) DefinedCustomBlocks[i] = 0;			
			// Reset textures
			texId = 0;
			for( int i = 0; i < textures.Length; i++ ) textures[i] = 0;
			// Reset culling
			for( int i = 0; i < hidden.Length; i++ ) hidden[i] = 0;
			for( int i = 0; i < CanStretch.Length; i++ ) CanStretch[i] = false;
			for( int i = 0; i < IsAir.Length; i++ ) IsAir[i] = false;
			// Reset bounds
			for( int i = 0; i < MinBB.Length; i++ ) MinBB[i] = Vector3.Zero;
			for( int i = 0; i < MaxBB.Length; i++ ) MaxBB[i] = Vector3.One;
			// Reset sounds
			for( int i = 0; i < DigSounds.Length; i++ ) DigSounds[i] = SoundType.None;
			for( int i = 0; i < StepSounds.Length; i++ ) StepSounds[i] = SoundType.None;
			Init();
			
			// TODO: Make this part of TerrainAtlas2D maybe?
			using( FastBitmap fastBmp = new FastBitmap( game.TerrainAtlas.AtlasBitmap, true, true ) )
				RecalculateSpriteBB( fastBmp );
		}
		
		public void Init() {
			for( int block = 1; block < BlocksCount; block++ ) {
				MaxBB[block].Y = 1;
				BlocksLight[block] = true;
				IsOpaque[block] = true;
				IsOpaqueY[block] = true;
				Collide[block] = CollideType.Solid;
				SpeedMultiplier[block] = 1;
				CullWithNeighbours[block] = true;
			}
			for( int block = 0; block < BlocksCount; block++ )
				Name[block] = "Invalid";
			MakeNormalNames();
			
			FogDensity[Block.StillWater] = 0.1f;
			FogColour[Block.StillWater] = new FastColour( 5, 5, 51 );
			FogDensity[Block.Water] = 0.1f;
			FogColour[Block.Water] = new FastColour( 5, 5, 51 );
			FogDensity[Block.StillLava] = 2f;
			FogColour[Block.StillLava] = new FastColour( 153, 25, 0 );
			FogDensity[Block.Lava] = 2f;
			FogColour[Block.Lava] = new FastColour( 153, 25, 0 );
			Collide[Block.Snow] = CollideType.WalkThrough;
			SpeedMultiplier[0] = 1f;
			CullWithNeighbours[Block.Leaves] = false;
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
			
			IsOpaqueY[Block.Slab] = true;
			IsOpaqueY[Block.CobblestoneSlab] = true;
			IsOpaqueY[Block.Snow] = true;

			InitBoundingBoxes();
			InitSounds();
			SetupCullingCache();
			InitLightOffsets();
		}

		public void SetDefaultBlockPermissions( InventoryPermissions canPlace, InventoryPermissions canDelete ) {
			for( int block = Block.Stone; block <= BlockInfo.MaxOriginalBlock; block++ ) {
				canPlace[block] = true;
				canDelete[block] = true;
			}
			canPlace[Block.Lava] = false;
			canPlace[Block.Water] = false;
			canPlace[Block.StillLava] = false;
			canPlace[Block.StillWater] = false;
			canPlace[Block.Bedrock] = false;
			
			canDelete[Block.Bedrock] = false;
			canDelete[Block.Lava] = false;
			canDelete[Block.Water] = false;
			canDelete[Block.StillWater] = false;
			canDelete[Block.StillLava] = false;
		}
		
		void MarkTransparent( byte id, bool blocks ) {
			IsTransparent[id] = true;
			BlocksLight[id] = blocks;
			IsOpaque[id] = false;
			IsOpaqueY[id] = false;
		}
		
		void MarkSprite( byte id ) {
			IsSprite[id] = true;
			IsTransparent[id] = true;
			BlocksLight[id] = false;
			IsOpaque[id] = false;
			IsOpaqueY[id] = false;
			Collide[id] = CollideType.WalkThrough;
		}
		
		void MarkTranslucent( byte id ) {
			IsTranslucent[id] = true;
			IsOpaque[id] = false;
			IsOpaqueY[id] = false;
		}
		
		void SetIsLiquid( byte id ) {
			IsLiquid[id] = true;
			Collide[id] = CollideType.SwimThrough;
		}
		
		void SetBlockHeight( byte id, float height ) {
			MaxBB[id].Y = height;
		}
		
		void SetFullBright( byte id, bool emits ) {
			FullBright[id] = emits;
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
			Collide[id] = CollideType.Solid;
			SpeedMultiplier[id] = 1;
			SetAll( 0, id );
			if( updateCulling )
				SetupCullingCache( id );
			MinBB[id] = Vector3.Zero;
			MaxBB[id] = Vector3.One;
			StepSounds[id] = SoundType.None;
			DigSounds[id] = SoundType.None;
		}
		
		void MakeNormalNames() {
			StringBuffer buffer = new StringBuffer( 64 );
			int start = 0;
			string value = Block.Names;
			
			for( int i = 0; i < BlockInfo.CpeCount; i++ ) {
				int next = value.IndexOf( ' ', start );
				if( next == -1 ) next = value.Length;
				
				buffer.Clear();
				int index = 0;
				SplitUppercase( buffer, value, start, next - start, ref index );
				Name[i] = buffer.ToString();
				start = next + 1;
			}
		}
		
		static void SplitUppercase( StringBuffer buffer, string value,
		                           int start, int len, ref int index ) {
			for( int i = start; i < start + len; i++ ) {
				char c = value[i];
				bool upper = Char.IsUpper( c ) && i > start;
				bool nextLower = i < value.Length - 1 && !Char.IsUpper( value[i + 1] );
				
				if( upper && nextLower ) {
					buffer.Append( ref index, ' ' );
					buffer.Append( ref index, Char.ToLower( c ) );
				} else {
					buffer.Append( ref index, c );
				}				
			}
		}
	}
	
	public enum CollideType : byte {
		WalkThrough, // i.e. gas or sprite
		SwimThrough, // i.e. liquid
		Solid,       // i.e. solid
	}
}