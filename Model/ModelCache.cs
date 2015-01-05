using System;
using System.Collections.Generic;

namespace ClassicalSharp.Model {

	public class ModelCache {
		
		Game window;
		public ModelCache( Game window ) {
			this.window = window;
			cache["humanoid"] = new PlayerModel( window );
		}
		
		Dictionary<string, IModel> cache = new Dictionary<string, IModel>();
		public IModel GetModel( string modelName ) {
			IModel model;
			byte blockId;
			if( Byte.TryParse( modelName, out blockId ) ) {
				if( !cache.TryGetValue( "block", out model ) ) {
					model = new BlockModel( window );
					cache["block"] = model;
				}			
				return model;
			} else {
				if( !cache.TryGetValue( modelName, out model ) ) {
					model = InitModel( modelName );
					if( model != null ) {
						cache[modelName] = model;
					} else {
						model = cache["humanoid"]; // fallback to default
					}
				}
				return model;
			}			
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
			}
			return null;
		}
		
		public void Dispose() {
			foreach( var entry in cache ) {
				entry.Value.Dispose();
			}
		}
	}
}
