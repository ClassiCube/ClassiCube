using System;
using System.IO;

namespace ClassicalSharp {

	public abstract class IMapFile {
		
		public virtual bool SupportsLoading { 
			get { return false; } 
		}
		
		public virtual bool SupportsSaving { 
			get { return false; } 
		}
		
		public virtual byte[] Load( Stream stream, Game game, out int width, out int height, out int length ) {
			width = 0;
			height = 0;
			length = 0;
			return null;
		}
		
		public virtual void Save( Stream stream, Game game ) {		
		}
	}
}
