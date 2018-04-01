// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Events;
using ClassicalSharp.Renderers;
using OpenTK;
using BlockID = System.UInt16;
using BlockRaw = System.Byte;

namespace ClassicalSharp.Map {
	
	/// <summary> Represents a fixed size map of blocks. Stores the raw block data,
	/// heightmap, dimensions and various metadata such as environment settings. </summary>
	public sealed class World {

		public BlockRaw[] blocks, blocks2;
		public int Width, Height, Length, MaxX, MaxY, MaxZ, OneY;
		public bool HasBlocks;
		
		/// <summary> Contains the environment metadata for this world. </summary>
		public WorldEnv Env;
		
		/// <summary> Unique uuid/guid of this particular world. </summary>
		public Guid Uuid;
		
		/// <summary> Current terrain.png or texture pack url of this map. </summary>
		public string TextureUrl = null;
		
		public World(Game game) {
			Env = new WorldEnv(game);
		}

		/// <summary> Resets all of the properties to their defaults and raises the 'OnNewMap' event. </summary>
		public void Reset() {
			Env.Reset();
			Width = 0; Height = 0; Length = 0;
			MaxX = 0;  MaxY = 0;   MaxZ = 0;
			OneY = 0;
			
			blocks = null;
			blocks2 = null;
			Uuid = Guid.NewGuid();
			HasBlocks = false;
		}
		
		/// <summary> Updates the underlying block array, and dimensions of this map. </summary>
		public void SetNewMap(BlockRaw[] blocksRaw, int width, int height, int length) {
			Width  = width;  MaxX = width  - 1;
			Height = height; MaxY = height - 1;
			Length = length; MaxZ = length - 1;
			OneY = width * length;
			
			blocks = blocksRaw;
			if (blocks.Length == 0) blocks = null;
			blocks2 = blocks;
			HasBlocks = blocks != null;
			
			if (blocksRaw.Length != (width * height * length)) {
				throw new InvalidOperationException("Blocks array length does not match volume of map.");
			}
			
			if (Env.EdgeHeight == -1)  Env.EdgeHeight  = height / 2;
			if (Env.CloudHeight == -1) Env.CloudHeight = height + 2;
		}
		
		/// <summary> Sets the block at the given world coordinates without bounds checking. </summary>
		public void SetBlock(int x, int y, int z, BlockID blockId) {
			int i = (y * Length + z) * Width + x;
			blocks[i] = (BlockRaw)blockId;
			if (blocks == blocks2) return;
			blocks2[i] = (BlockRaw)(blockId >> 8);
		}
		
		/// <summary> Returns the block at the given world coordinates without bounds checking. </summary>
		public BlockID GetBlock(int x, int y, int z) {
			int i = (y * Length + z) * Width + x;
			#if !ONLY_8BIT
			return (BlockID)((blocks[i] | (blocks2[i] << 8)) & BlockInfo.IDMask);
			#else
			return blocks[i];
			#endif
		}
		
		/// <summary> Returns the block at the given world coordinates without bounds checking. </summary>
		public BlockID GetBlock(Vector3I p) {
			int i = (p.Y * Length + p.Z) * Width + p.X;
			#if !ONLY_8BIT
			return (BlockID)((blocks[i] | (blocks2[i] << 8)) & BlockInfo.IDMask);
			#else
			return blocks[i];
			#endif
		}
		
		/// <summary> Returns the block at the given world coordinates with bounds checking,
		/// returning 0 is the coordinates were outside the map. </summary>
		public BlockID SafeGetBlock(Vector3I p) {
			return IsValidPos(p.X, p.Y, p.Z) ? GetBlock(p) : Block.Air;
		}
		
		/// <summary> Returns whether the given world coordinates are contained
		/// within the dimensions of the map. </summary>
		public bool IsValidPos(int x, int y, int z) {
			return x >= 0 && y >= 0 && z >= 0 &&
				x < Width && y < Height && z < Length;
		}
		
		/// <summary> Returns whether the given world coordinates are contained
		/// within the dimensions of the map. </summary>
		public bool IsValidPos(Vector3I p) {
			return p.X >= 0 && p.Y >= 0 && p.Z >= 0 &&
				p.X < Width && p.Y < Height && p.Z < Length;
		}
		
		public BlockID GetPhysicsBlock(int x, int y, int z) {
			if (x < 0 || x >= Width || z < 0 || z >= Length || y < 0) return Block.Bedrock;			
			if (y >= Height) return Block.Air;
			
			int i = (y * Length + z) * Width + x;
			#if !ONLY_8BIT
			return (BlockID)((blocks[i] | (blocks2[i] << 8)) & BlockInfo.IDMask);
			#else
			return blocks[i];
			#endif
		}
	}
}