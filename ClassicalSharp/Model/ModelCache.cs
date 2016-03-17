using System;
using System.Collections.Generic;
using System.IO;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp.Model {

	public class ModelCache : IDisposable {
		
		Game game;
		IGraphicsApi api;
		public ModelCache( Game window ) {
			this.game = window;
			api = game.Graphics;		
		}
		
		public void InitCache() {
			vertices = new VertexPos3fTex2fCol4b[24 * 12];
			vb = api.CreateDynamicVb( VertexFormat.Pos3fTex2fCol4b, vertices.Length );
			cache["humanoid"] = new HumanoidModel( game );
			cache["human"] = cache["humanoid"];
		}
		
		internal int vb;
		internal VertexPos3fTex2fCol4b[] vertices;
		Dictionary<string, IModel> cache = new Dictionary<string, IModel>();
		internal int ChickenTexId, CreeperTexId, PigTexId, SheepTexId,
		SkeletonTexId, SpiderTexId, ZombieTexId, SheepFurTexId, HumanoidTexId;
		
		public IModel GetModel( string modelName ) {
			IModel model;
			byte blockId;
			if( Byte.TryParse( modelName, out blockId ) )
				modelName = "block";
			
			if( !cache.TryGetValue( modelName, out model ) ) {
				model = InitModel( modelName );
				if( model == null ) model = cache["humanoid"]; // fallback to default
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
			return null;
		}
		
		public void Dispose() {
			foreach( var entry in cache ) {
				entry.Value.Dispose();
			}
			api.DeleteDynamicVb( vb );
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
	}
}
