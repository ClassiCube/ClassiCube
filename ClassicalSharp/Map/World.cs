﻿// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Events;
using ClassicalSharp.Renderers;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Map {
	
	/// <summary> Represents a fixed size map of blocks. Stores the raw block data,
	/// heightmap, dimensions and various metadata such as environment settings. </summary>
	public sealed partial class World {

		Game game;
		BlockInfo info;
		public BlockID[] blocks;
		public int Width, Height, Length;
		
		/// <summary> Contains the environment metadata for this world. </summary>
		public WorldEnv Env;
		
		/// <summary> Unique uuid/guid of this particular world. </summary>
		public Guid Uuid;
		
		/// <summary> Whether this map is empty. </summary>
		public bool IsNotLoaded = true;
		
		/// <summary> Current terrain.png or texture pack url of this map. </summary>
		public string TextureUrl = null;
		
		public World(Game game) {
			Env = new WorldEnv(game);
			this.game = game;
			info = game.BlockInfo;
		}

		/// <summary> Resets all of the properties to their defaults and raises the 'OnNewMap' event. </summary>
		public void Reset() {
			Env.Reset();
			Width = Height = Length = 0;
			IsNotLoaded = true;
			
			Uuid = Guid.NewGuid();
			game.WorldEvents.RaiseOnNewMap();
		}
		
		/// <summary> Updates the underlying block array, heightmap, and dimensions of this map. </summary>
		public void SetNewMap(BlockID[] blocks, int width, int height, int length) {
			this.blocks = blocks;
			this.Width = width;
			this.Height = height;
			this.Length = length;
			IsNotLoaded = width == 0 || length == 0 || height == 0;
			if (blocks.Length != (width * height * length))
				throw new InvalidOperationException("Blocks array length does not match volume of map.");
			
			if (Env.EdgeHeight == -1) Env.EdgeHeight = height / 2;
			if (Env.CloudHeight == -1) Env.CloudHeight = height + 2;
		}
		
		/// <summary> Sets the block at the given world coordinates without bounds checking. </summary>
		public void SetBlock(int x, int y, int z, BlockID blockId) {
			blocks[(y * Length + z) * Width + x] = blockId;
		}
		
		/// <summary> Sets the block at the given world coordinates without bounds checking,
		/// and also recalculates the heightmap for the given (x,z) column.	</summary>
		public void SetBlock(Vector3I p, BlockID blockId) {
			blocks[(p.Y * Length + p.Z) * Width + p.X] = blockId;
		}
		
		/// <summary> Returns the block at the given world coordinates without bounds checking. </summary>
		public BlockID GetBlock(int x, int y, int z) {
			return blocks[(y * Length + z) * Width + x];
		}
		
		/// <summary> Returns the block at the given world coordinates without bounds checking. </summary>
		public BlockID GetBlock(Vector3I p) {
			return blocks[(p.Y * Length + p.Z) * Width + p.X];
		}
		
		/// <summary> Returns the block at the given world coordinates with bounds checking,
		/// returning 0 is the coordinates were outside the map. </summary>
		public BlockID SafeGetBlock(int x, int y, int z) {
			return IsValidPos(x, y, z) ?
				blocks[(y * Length + z) * Width + x] : Block.Air;
		}
		
		/// <summary> Returns the block at the given world coordinates with bounds checking,
		/// returning 0 is the coordinates were outside the map. </summary>
		public BlockID SafeGetBlock(Vector3I p) {
			return IsValidPos(p.X, p.Y, p.Z) ?
				blocks[(p.Y * Length + p.Z) * Width + p.X] : Block.Air;
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
		
		/// <summary> Unpacks the given index into the map's block array into its original world coordinates. </summary>
		public Vector3I GetCoords(int index) {
			if (index < 0 || index >= blocks.Length)
				return new Vector3I(-1);
			
			int x = index % Width;
			int y = index / (Width * Length);
			int z = (index / Width) % Length;
			return new Vector3I(x, y, z);
		}
		
		public BlockID GetPhysicsBlock(int x, int y, int z) {
			if (x < 0 || x >= Width || z < 0 || z >= Length || y < 0) return Block.Bedrock;			
			if (y >= Height) return Block.Air;
			return blocks[(y * Length + z) * Width + x];
		}
	}
}