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
			return cache["humanoid"];		
		}
		
		public void Dispose() {
			foreach( var entry in cache ) {
				entry.Value.Dispose();
			}
		}
	}
}
