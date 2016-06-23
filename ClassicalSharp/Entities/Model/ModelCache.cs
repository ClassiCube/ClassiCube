// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.IO;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp.Model {

	public class ModelCache : IDisposable {
		
		Game game;
		IGraphicsApi api;
		public ModelCache( Game window ) {
			this.game = window;
			api = game.Graphics;
		}
		#if FALSE
		public CustomModel[] CustomModels = new CustomModel[256];
		#endif
		
		public void InitCache() {
			vertices = new VertexP3fT2fC4b[24 * 12];
			vb = api.CreateDynamicVb( VertexFormat.P3fT2fC4b, vertices.Length );
			IModel model = new HumanoidModel( game );
			model.CreateParts();
			cache["humanoid"] = model;
			cache["human"] = cache["humanoid"];
			game.Events.TextureChanged += TextureChanged;
		}
		
		internal int vb;
		internal VertexP3fT2fC4b[] vertices;
		Dictionary<string, IModel> cache = new Dictionary<string, IModel>();
		internal int ChickenTexId, CreeperTexId, PigTexId, SheepTexId,
		SkeletonTexId, SpiderTexId, ZombieTexId, SheepFurTexId, HumanoidTexId;
		
		public IModel GetModel( string modelName ) {
			if( modelName == "block" ) return cache["humanoid"];
			IModel model;
			byte blockId;
			if( Byte.TryParse( modelName, out blockId ) )
				modelName = "block";
			
			if( !cache.TryGetValue( modelName, out model ) ) {
				model = InitModel( modelName );
				if( model != null ) model.CreateParts();
				else model = cache["humanoid"]; // fallback to default
				cache[modelName] = model;
			}
			return model;
		}
		
		IModel InitModel( string modelName ) {
			if( modelName == "chicken" ) return new ChickenModel( game );
			else if( modelName == "creeper" ) return new CreeperModel( game );
			else if( modelName == "pig" ) return new PigModel( game );
			else if( modelName == "sheep" ) return new SheepModel( game );
			else if( modelName == "skeleton" ) return new SkeletonModel( game );
			else if( modelName == "spider" ) return new SpiderModel( game );
			else if( modelName == "zombie" ) return new ZombieModel( game );
			else if( modelName == "block" ) return new BlockModel( game );
			else if( modelName == "chibi" ) return new ChibiModel( game );
			else if( modelName == "giant" ) return new GiantModel( game );
			return null;
		}
		
		public void Dispose() {
			foreach( var entry in cache )
				entry.Value.Dispose();
			api.DeleteDynamicVb( vb );
			game.Events.TextureChanged -= TextureChanged;
			
			api.DeleteTexture( ref ChickenTexId );
			api.DeleteTexture( ref CreeperTexId );
			api.DeleteTexture( ref PigTexId );
			api.DeleteTexture( ref SheepTexId );
			api.DeleteTexture( ref SkeletonTexId );
			api.DeleteTexture( ref SpiderTexId );
			api.DeleteTexture( ref ZombieTexId );
			api.DeleteTexture( ref SheepFurTexId );
			api.DeleteTexture( ref HumanoidTexId );
		}
		
		void TextureChanged( object sender, TextureEventArgs e ) {
			switch( e.Name ) {
				case "chicken.png":
					game.UpdateTexture( ref ChickenTexId, e.Name, e.Data, false ); break;
				case "creeper.png":
					game.UpdateTexture( ref CreeperTexId, e.Name, e.Data, false ); break;
				case "pig.png":
					game.UpdateTexture( ref PigTexId, e.Name, e.Data, false ); break;
				case "sheep.png":
					game.UpdateTexture( ref SheepTexId, e.Name, e.Data, false ); break;
				case "skeleton.png":
					game.UpdateTexture( ref SkeletonTexId, e.Name, e.Data, false ); break;
				case "spider.png":
					game.UpdateTexture( ref SpiderTexId, e.Name, e.Data, false ); break;
				case "zombie.png":
					game.UpdateTexture( ref ZombieTexId, e.Name, e.Data, false ); break;
				case "sheep_fur.png":
					game.UpdateTexture( ref SheepFurTexId, e.Name, e.Data, false ); break;
				case "char.png":
					game.UpdateTexture( ref HumanoidTexId, e.Name, e.Data, true ); break;
			}
		}
	}
}
