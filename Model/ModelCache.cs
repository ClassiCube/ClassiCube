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
			EntityNameVb = window.Graphics.CreateDynamicVb( VertexPos3fTex2f.Size, 4 );
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
				model = cache["humanoid"]; // fallback to default
				cache[modelName] = cache["humanoid"];
			}
			return model;
		}
		
		public void AddModel( IModel model ) {
			string key = model.ModelName.ToLowerInvariant();
			if( !cache.ContainsKey( key ) ) {
				cache[key] = model;
			}
		}
		
		public void Dispose() {
			foreach( var entry in cache ) {
				entry.Value.Dispose();
			}
			window.Graphics.DeleteVb( EntityNameVb );
		}
	}
}
