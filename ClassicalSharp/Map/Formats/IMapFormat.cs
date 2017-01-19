// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.IO;

namespace ClassicalSharp.Map {

	/// <summary> Imports a world and metadata associated 
	/// with it from a particular file format. </summary>
	public interface IMapFormatImporter {
		
		/// <summary> Replaces the current world from a stream that contains a world in this format. </summary>
		byte[] Load(Stream stream, Game game, out int width, out int height, out int length);
	}
	
	/// <summary> Exports the current world and metadata associated 
	/// with it to a particular file format. </summary>
	public interface IMapFormatExporter {
		
		/// <summary> Exports the current world to a file in this format. </summary>
		void Save(Stream stream, Game game);
	}
}
