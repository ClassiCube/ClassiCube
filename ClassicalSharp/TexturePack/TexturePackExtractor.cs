﻿using System;
using System.Drawing;
using System.IO;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;

namespace ClassicalSharp.TexturePack {
	
	public sealed class TexturePackExtractor {
		
		Game game;
		public void Extract( string path, Game game ) {
			using( Stream fs = new FileStream( path, FileMode.Open, FileAccess.Read, FileShare.Read ) )
				Extract( fs, game );
		}
		
		public void Extract( byte[] data, Game game ) {
			using( Stream fs = new MemoryStream( data ) )
				Extract( fs, game );
		}
		
		void Extract( Stream stream, Game game ) {
			this.game = game;
			game.Animations.Dispose();
			ZipReader reader = new ZipReader();
			reader.ShouldProcessZipEntry = ShouldProcessZipEntry;
			reader.ProcessZipEntry = ProcessZipEntry;
			reader.Extract( stream );
		}
		
		bool ShouldProcessZipEntry( string filename ) {
			return true;
		}
		
		void ProcessZipEntry( string filename, byte[] data, ZipEntry entry ) {
			MemoryStream stream = new MemoryStream( data );
			ModelCache cache = game.ModelCache;
			IGraphicsApi api = game.Graphics;
			switch( filename ) {
				case "terrain.png":
					game.ChangeTerrainAtlas( new Bitmap( stream ) ); break;
				case "mob/chicken.png":
				case "chicken.png":
					UpdateTexture( ref cache.ChickenTexId, stream, false ); break;
				case "mob/creeper.png":
				case "creeper.png":
					UpdateTexture( ref cache.CreeperTexId, stream, false ); break;
				case "mob/pig.png":
				case "pig.png":
					UpdateTexture( ref cache.PigTexId, stream, false ); break;
				case "mob/sheep.png":
				case "sheep.png":
					UpdateTexture( ref cache.SheepTexId, stream, false ); break;
				case "mob/skeleton.png":
				case "skeleton.png":
					UpdateTexture( ref cache.SkeletonTexId, stream, false ); break;
				case "mob/spider.png":
				case "spider.png":
					UpdateTexture( ref cache.SpiderTexId, stream, false ); break;
				case "mob/zombie.png":
				case "zombie.png":
					UpdateTexture( ref cache.ZombieTexId, stream, false ); break;
				case "mob/sheep_fur.png":
				case "sheep_fur.png":
					UpdateTexture( ref cache.SheepFurTexId, stream, false ); break;
				case "char.png":
					UpdateTexture( ref cache.HumanoidTexId, stream, true ); break;
				case "clouds.png":
				case "cloud.png":
					UpdateTexture( ref game.CloudsTextureId, stream, false ); break;
				case "rain.png":
					UpdateTexture( ref game.RainTextureId, stream, false ); break;
				case "snow.png":
					UpdateTexture( ref game.SnowTextureId, stream, false ); break;
				case "animations.png":
				case "animation.png":
					game.Animations.SetAtlas( new Bitmap( stream ) ); break;
				case "animations.txt":
				case "animation.txt":
					StreamReader reader = new StreamReader( stream );
					game.Animations.ReadAnimationsDescription( reader );
					break;
			}
		}
		
		void UpdateTexture( ref int texId, Stream stream, bool setSkinType ) {
			game.Graphics.DeleteTexture( ref texId );
			using( Bitmap bmp = new Bitmap( stream ) ) {
				if( setSkinType )
					game.DefaultPlayerSkinType = Utils.GetSkinType( bmp );
				texId = game.Graphics.CreateTexture( bmp );
			}
		}
	}
}
