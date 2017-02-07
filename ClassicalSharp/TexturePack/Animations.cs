// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Textures {

	/// <summary> Contains and describes the various animations applied to the terrain atlas. </summary>
	public class Animations : IGameComponent {
		
		Game game;
		IGraphicsApi gfx;
		Bitmap animBmp;
		FastBitmap animsBuffer;
		List<AnimationData> animations = new List<AnimationData>();
		bool validated = false, useLavaAnim = false, useWaterAnim = false;
		
		public void Init(Game game) {
			this.game = game;
			gfx = game.Graphics;
			game.Events.TexturePackChanged += TexturePackChanged;
			game.Events.TextureChanged += TextureChanged;
		}

		public void Ready(Game game) { }
		public void Reset(Game game) { }
		public void OnNewMap(Game game) { }
		public void OnNewMapLoaded(Game game) { }
		
		void TexturePackChanged(object sender, EventArgs e) {
			Clear();
			useLavaAnim = IsDefaultZip();
			useWaterAnim = useLavaAnim;
		}
		
		void TextureChanged(object sender, TextureEventArgs e) {
			if (e.Name == "animations.png" || e.Name == "animation.png") {
				animBmp = Platform.ReadBmp32Bpp(game.Drawer2D, e.Data);
				animsBuffer = new FastBitmap(animBmp, true, true);
			} else if (e.Name == "animations.txt" || e.Name == "animation.txt") {
				MemoryStream stream = new MemoryStream(e.Data);
				StreamReader reader = new StreamReader(stream);
				ReadAnimationsDescription(reader);
			} else if (e.Name == "uselavaanim") {
				useLavaAnim = true;
			} else if (e.Name == "usewateranim") {
				useWaterAnim = true;
			}
		}
		
		/// <summary> Runs through all animations and if necessary updates the terrain atlas. </summary>
		public void Tick(ScheduledTask task) {
			if (useLavaAnim) {
				int size = Math.Min(game.TerrainAtlas.elementSize, 64);
				DrawAnimation(null, 30, size);
			}			
			if (useWaterAnim) {
				int size = Math.Min(game.TerrainAtlas.elementSize, 64);
				DrawAnimation(null, 14, size);
			}
			
			if (animations.Count == 0) return;			
			if (animsBuffer == null) {
				game.Chat.Add("&cCurrent texture pack specifies it uses animations,");
				game.Chat.Add("&cbut is missing animations.png");
				animations.Clear();
				return;
			}
			if (!validated) ValidateAnimations();			
			
			for (int i = 0; i < animations.Count; i++)
				ApplyAnimation(animations[i]);
		}
		
		/// <summary> Reads a text file that contains a number of lines, with each line describing:<br/>
		/// 1) the target tile in the terrain atlas  2) the start location of animation frames<br/>
		/// 3) the size of each animation frame      4) the number of animation frames<br/>
		/// 5) the delay between advancing animation frames. </summary>
		public void ReadAnimationsDescription(StreamReader reader) {
			string line;
			while ((line = reader.ReadLine()) != null) {
				if (line.Length == 0 || line[0] == '#') continue;
				
				string[] parts = line.Split(' ');
				if (parts.Length < 7) {
					Utils.LogDebug("Not enough arguments for animation: {0}", line); continue;
				}
				
				byte tileX, tileY;
				if (!Byte.TryParse(parts[0], out tileX) || !Byte.TryParse(parts[1], out tileY)
				   || tileX >= 16 || tileY >= 16) {
					Utils.LogDebug("Invalid animation tile coordinates: {0}", line); continue;
				}
				
				int frameX, frameY;
				if (!Int32.TryParse(parts[2], out frameX) || !Int32.TryParse(parts[3], out frameY)
				   || frameX < 0 || frameY < 0) {
					Utils.LogDebug("Invalid animation coordinates: {0}", line); continue;
				}
				
				int frameSize, statesCount, tickDelay;
				if (!Int32.TryParse(parts[4], out frameSize) || !Int32.TryParse(parts[5], out statesCount) ||
				   !Int32.TryParse(parts[6], out tickDelay) || frameSize < 0 || statesCount < 0 || tickDelay < 0) {
					Utils.LogDebug("Invalid animation: {0}", line); continue;
				}
				
				DefineAnimation(tileX, tileY, frameX, frameY, frameSize, statesCount, tickDelay);
			}
		}
		
		/// <summary> Adds an animation described by the arguments to the list of animations
		/// that are applied to the terrain atlas. </summary>
		public void DefineAnimation(int tileX, int tileY, int frameX, int frameY, int frameSize,
		                            int statesNum, int tickDelay) {
			AnimationData data = new AnimationData();
			data.TileX = tileX; data.TileY = tileY;
			data.FrameX = frameX; data.FrameY = frameY;
			data.FrameSize = frameSize; data.StatesCount = statesNum;
			
			data.TickDelay = tickDelay;
			animations.Add(data);
		}
		
		FastBitmap animPart = new FastBitmap();
		LavaAnimation lavaAnim = new LavaAnimation();
		WaterAnimation waterAnim = new WaterAnimation();
		unsafe void ApplyAnimation(AnimationData data) {
			data.Tick--;
			if (data.Tick >= 0) return;
			data.State++;
			data.State %= data.StatesCount;
			data.Tick = data.TickDelay;
			
			int texId = (data.TileY << 4) | data.TileX;
			if (texId == 30 && useLavaAnim) return;
			if (texId == 14 && useWaterAnim) return;
			DrawAnimation(data, texId, data.FrameSize);
		}
		
		unsafe void DrawAnimation(AnimationData data, int texId, int size) {
			TerrainAtlas1D atlas = game.TerrainAtlas1D;			
			int index = atlas.Get1DIndex(texId);
			int rowNum = atlas.Get1DRowId(texId);			
			byte* temp = stackalloc byte[size * size * 4];
			animPart.SetData(size, size, size * 4, (IntPtr)temp, false);
			
			if (data == null) {
				if (texId == 30) {
					lavaAnim.Tick((int*)temp, size);
				} else if (texId == 14) {
					waterAnim.Tick((int*)temp, size);
				}
			} else {
				FastBitmap.MovePortion(data.FrameX + data.State * size, 
				                       data.FrameY, 0, 0, animsBuffer, animPart, size);
			}
			gfx.UpdateTexturePart(atlas.TexIds[index], 0, rowNum * game.TerrainAtlas.elementSize, animPart);
		}
		
		bool IsDefaultZip() {
			if (game.World.TextureUrl != null) return false;
			string texPack = Options.Get(OptionsKey.DefaultTexturePack);
			return texPack == null || texPack == "default.zip";
		}
		
		/// <summary> Disposes the atlas bitmap that contains animation frames, and clears
		/// the list of defined animations. </summary>
		public void Clear() {
			animations.Clear();
			
			if (animBmp == null) return;
			animsBuffer.Dispose(); animsBuffer = null;
			animBmp.Dispose(); animBmp = null;
			validated = false;
		}
		
		public void Dispose() {
			Clear();
			game.Events.TextureChanged -= TextureChanged;
			game.Events.TexturePackChanged -= TexturePackChanged;
		}
		
		class AnimationData {
			public int TileX, TileY; // Tile (not pixel) coordinates in terrain.png
			public int FrameX, FrameY; // Top left pixel coordinates of start frame in animatons.png
			public int FrameSize; // Size of each frame in pixel coordinates
			public int State; // Current animation frame index
			public int StatesCount; // Total number of animation frames
			public int Tick, TickDelay;
		}
		
		const string format = "&cOne of the animation frames for tile ({0}, {1}) " +
			"is at coordinates outside animations.png";
		void ValidateAnimations() {
			validated = true;
			for (int i = animations.Count - 1; i >= 0; i--) {
				AnimationData a = animations[i];
				int maxY = a.FrameY + a.FrameSize;
				int maxX = a.FrameX + a.FrameSize * a.StatesCount;
				if (maxX <= animsBuffer.Width && maxY <= animsBuffer.Height)
					continue;
				
				game.Chat.Add(String.Format(format, a.TileX, a.TileY));
				animations.RemoveAt(i);
			}
		}
	}
}
