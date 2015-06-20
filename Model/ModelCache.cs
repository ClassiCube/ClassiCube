using System;
using System.Collections.Generic;
using System.IO;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp.Model {

	public class ModelCache {
		
		Game window;
		public EntityShader Shader;
		public VertexPos3fTex2f[] EntityNameVertices = new VertexPos3fTex2f[4];
		public int EntityNameVb;
		public ModelCache( Game window ) {
			this.window = window;
		}
		
		public void Init() {
			// can't place this in constructor because entity constructors
			// depend on Window.ModelCache not being null.
			Shader = new EntityShader();
			Shader.Init( window.Graphics );
			cache["humanoid"] = new PlayerModel( window );
			EntityNameVb = window.Graphics.CreateDynamicVb( VertexFormat.Pos3fTex2f, 4 );
		}
		
		Dictionary<string, IModel> cache = new Dictionary<string, IModel>();
		public IModel GetModel( string modelName ) {
			IModel model;
			byte blockId;
			if( Byte.TryParse( modelName, out blockId ) ) {
				modelName = "block";
				if( blockId == 0 || blockId > BlockInfo.MaxDefinedBlock )
					return cache["humanoid"];
			}
			
			if( !cache.TryGetValue( modelName, out model ) ) {
				try {
					model = InitModel( modelName );
				} catch( FileNotFoundException ) {
					model = null;
					Utils.LogWarning( modelName + " not found, falling back to human default." );
				}
				if( model == null ) 
					model = cache["humanoid"]; // fallback to default
				cache[modelName] = model;
			}
			return model;
		}
		
		IModel InitModel( string modelName ) {
			if( modelName == "chicken" ) {
				return new ChickenModel( window );
			} else if( modelName == "creeper" ) {
				return new CreeperModel( window );
			} else if( modelName == "pig" ) {
				return new PigModel( window );
			} else if( modelName == "sheep" ) {
				return new SheepModel( window );
			} else if( modelName == "skeleton" ) {
				return new SkeletonModel( window );
			} else if( modelName == "spider" ) {
				return new SpiderModel( window );
			} else if( modelName == "zombie" ) {
				return new ZombieModel( window );
			} else if( modelName == "block" ) {
				return new BlockModel( window );
			}
			return null;
		}
		
		public void Dispose() {
			foreach( var entry in cache ) {
				entry.Value.Dispose();
			}
			window.Graphics.DeleteVb( EntityNameVb );
		}
	}
}
