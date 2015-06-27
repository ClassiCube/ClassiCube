// Copyright (c) 2011 Dino Chiesa.
// All rights reserved.
// This code module is part of DotNetZip, a zipfile class library.

// This code is licensed under the Microsoft Public License.
// See the file License.txt for the license details.
// More info on: http://dotnetzip.codeplex.com

using System;

namespace Ionic.Zlib
{
	public class CRC32
	{
		public long TotalBytesRead {
			get { return _TotalBytesRead; }
		}
		
		public int Crc32Result {
			get { return (Int32)~_register; }
		}

		public void SlurpBlock(byte[] block, int offset, int count) {
			if (block == null)
				throw new Exception("The data buffer must not be null.");

			// bzip algorithm
			for(int i = 0; i < count; i++) {
				byte b = block[offset + i];
				uint temp = (_register & 0x000000FF) ^ b;
				_register = (_register >> 8) ^ crc32Table[temp];
			}
			_TotalBytesRead += count;
		}

		private void GenerateLookupTable() {
			for( uint i = 0; i < 256; i++ ) {
				uint dwCrc = i;
				for (byte j = 8; j > 0; j--) {
					if ((dwCrc & 1) == 1) {
						dwCrc = (dwCrc >> 1) ^ dwPolynomial;
					} else {
						dwCrc >>= 1;
					}
				}
				crc32Table[i] = dwCrc;
			}
		}

		public CRC32() : this(0xEDB88320) {
		}

		public CRC32(uint polynomial) {
			this.dwPolynomial = polynomial;
			this.GenerateLookupTable();
		}

		public void Reset() {
			_register = 0xFFFFFFFFU;
		}

		uint dwPolynomial;
		long _TotalBytesRead;
		uint[] crc32Table = new uint[256];
		uint _register = 0xFFFFFFFFU;
	}
}