// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;

namespace ClassicalSharp.Map {

	/// <summary> Exports and/or imports a world and metadata associated 
	/// with to and/or from a particular file format. </summary>
	public abstract class IMapFileFormat {
		
		/// <summary> Whether a world can be exported to a file in this format. </summary>
		public virtual bool SupportsLoading { get { return false; } }
		
		/// <summary> Whether a world can be imported from a file in this format. </summary>
		public virtual bool SupportsSaving { get { return false; } }
		
		/// <summary> Replaces the current world from a stream that contains a world in this format. </summary>
		public virtual byte[] Load( Stream stream, Game game, out int width, out int height, out int length ) {
			width = 0; height = 0; length = 0;
			return null;
		}
		
		/// <summary> Exports the current world to a file in this format. </summary>
		public virtual void Save( Stream stream, Game game ) {		
		}
	}
}
