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
		Bitmap animBmp;
		FastBitmap animsBuffer;
		List<AnimationData> animations = new List<AnimationData>();
		bool validated = false, useLavaAnim = false, useWaterAnim = false;
		
		void IGameComponent.Init(Game game) {
			this.game = game;
			game.Events.TexturePackChanged += TexturePackChanged;
			game.Events.TextureChanged += TextureChanged;
		}

		void IGameComponent.Ready(Game game) { }
		void IGameComponent.Reset(Game game) { }
		void IGameComponent.OnNewMap(Game game) { }
		void IGameComponent.OnNewMapLoaded(Game game) { }
		
		void TexturePackChanged(object sender, EventArgs e) {
			Clear();
			useLavaAnim = IsDefaultZip();
			useWaterAnim = useLavaAnim;
		}
		
		void TextureChanged(object sender, TextureEventArgs e) {
			if (e.Name == "animations.png" || e.Name == "animation.png") {
				animBmp = Platform.ReadBmp(game.Drawer2D, e.Data);
				animsBuffer = new FastBitmap(animBmp, true, true);
			} else if (e.Name == "animations.txt" || e.Name == "animation.txt") {
				MemoryStream stream = new MemoryStream(e.Data);
				StreamReader reader = new StreamReader(stream, false);
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
				int size = Math.Min(Atlas2D.TileSize, 64);
				DrawAnimation(null, 30, size);
			}			
			if (useWaterAnim) {
				int size = Math.Min(Atlas2D.TileSize, 64);
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
		void ReadAnimationsDescription(StreamReader reader) {
			string line;
			while ((line = reader.ReadLine()) != null) {
				if (line.Length == 0 || line[0] == '#') continue;
				
				string[] parts = line.Split(' ');
				if (parts.Length < 7) {
					game.Chat.Add("&cNot enough arguments for animation: " + line); continue;
				}
				
				byte tileX, tileY;
				if (!Byte.TryParse(parts[0], out tileX) || !Byte.TryParse(parts[1], out tileY)
				   || tileX >= Atlas2D.TilesPerRow || tileY >= Atlas2D.MaxRowsCount) {
					game.Chat.Add("&cInvalid animation tile coords: " + line); continue;
				}
				
				int frameX, frameY;
				if (!Int32.TryParse(parts[2], out frameX) || !Int32.TryParse(parts[3], out frameY)
				   || frameX < 0 || frameY < 0) {
					game.Chat.Add("&cInvalid animation coords: " + line); continue;
				}
				
				int frameSize, statesCount, tickDelay;
				if (!Int32.TryParse(parts[4], out frameSize) || !Int32.TryParse(parts[5], out statesCount) ||
				   !Int32.TryParse(parts[6], out tickDelay) || frameSize < 0 || statesCount < 0 || tickDelay < 0) {
					game.Chat.Add("&cInvalid animation: " + line); continue;
				}
				
				DefineAnimation(tileX, tileY, frameX, frameY, frameSize, statesCount, tickDelay);
			}
		}
		
		/// <summary> Adds an animation described by the arguments to the list of animations
		/// that are applied to the terrain atlas. </summary>
		void DefineAnimation(int tileX, int tileY, int frameX, int frameY, int frameSize, int statesNum, int tickDelay) {
			AnimationData data = new AnimationData();
			data.TexLoc = tileY * Atlas2D.TilesPerRow + tileX;
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
			
			int texLoc = data.TexLoc;
			if (texLoc == 30 && useLavaAnim)  return;
			if (texLoc == 14 && useWaterAnim) return;
			DrawAnimation(data, texLoc, data.FrameSize);
		}
		
		unsafe void DrawAnimation(AnimationData data, int texLoc, int size) {
			if (size <= 128) {
				byte* temp = stackalloc byte[size * size * 4];
				DrawAnimationCore(data, texLoc, size, temp);
			} else {
				// cannot allocate memory on the stack for very big animation.png frames
				byte[] temp = new byte[size * size * 4];
				fixed (byte* ptr = temp) {
					DrawAnimationCore(data, texLoc, size, ptr);
				}
			}
		}
		
		unsafe void DrawAnimationCore(AnimationData data, int texLoc, int size, byte* temp) {
			int index_1D = Atlas1D.Get1DIndex(texLoc);
			int rowId_1D = Atlas1D.Get1DRowId(texLoc);
			animPart.SetData(size, size, size * 4, (IntPtr)temp, false);
			
			if (data == null) {
				if (texLoc == 30) {
					lavaAnim.Tick((int*)temp, size);
				} else if (texLoc == 14) {
					waterAnim.Tick((int*)temp, size);
				}
			} else {
				FastBitmap.MovePortion(data.FrameX + data.State * size, 
				                       data.FrameY, 0, 0, animsBuffer, animPart, size);
			}
			
			int dstY = rowId_1D * Atlas2D.TileSize;
			game.Graphics.UpdateTexturePart(Atlas1D.TexIds[index_1D], 0, dstY, animPart, game.Graphics.Mipmaps);
		}
		
		bool IsDefaultZip() {
			if (game.World.TextureUrl != null) return false;
			string texPack = Options.Get(OptionsKey.DefaultTexturePack, "default.zip");
			return texPack == "default.zip";
		}
		
		/// <summary> Disposes the atlas bitmap that contains animation frames, and clears
		/// the list of defined animations. </summary>
		void Clear() {
			animations.Clear();
			
			if (animBmp == null) return;
			animsBuffer.Dispose(); animsBuffer = null;
			animBmp.Dispose(); animBmp = null;
			validated = false;
		}
		
		void IDisposable.Dispose() {
			Clear();
			game.Events.TextureChanged -= TextureChanged;
			game.Events.TexturePackChanged -= TexturePackChanged;
		}
		
		class AnimationData {
			public int TexLoc;     // Tile (not pixel) coordinates in terrain.png
			public int FrameX, FrameY; // Top left pixel coordinates of start frame in animatons.png
			public int FrameSize; // Size of each frame in pixel coordinates
			public int State; // Current animation frame index
			public int StatesCount; // Total number of animation frames
			public int Tick, TickDelay;
		}
		
		const string format = "&cSome of the animation frames for tile ({0}, {1}) are at coordinates outside animations.png";
		const string terrainFormat = "&cAnimation frames for tile ({0}, {1}) are bigger than the size of a tile in terrain.png";
		void ValidateAnimations() {
			validated = true;
			for (int i = animations.Count - 1; i >= 0; i--) {
				AnimationData a = animations[i];
				int tileX = a.TexLoc % Atlas2D.TilesPerRow, tileY = a.TexLoc / Atlas2D.TilesPerRow;
				
				if (a.FrameSize > Atlas2D.TileSize || tileY >= Atlas2D.RowsCount) {
					game.Chat.Add(String.Format(terrainFormat, tileX, tileY));
					animations.RemoveAt(i);
					continue;
				}
				
				int maxY = a.FrameY + a.FrameSize;
				int maxX = a.FrameX + a.FrameSize * a.StatesCount;
				if (maxX <= animsBuffer.Width && maxY <= animsBuffer.Height) continue;
				
				game.Chat.Add(String.Format(format, tileX, tileY));
				animations.RemoveAt(i);
			}
		}
	}
}
