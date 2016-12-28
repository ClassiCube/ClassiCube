// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Security.Cryptography;

namespace Launcher {
	
	public static class Secure {
		
		public static string Encode(string decoded, string key) {
			if (String.IsNullOrEmpty(decoded) || String.IsNullOrEmpty(key)) return "";
			
			byte[] data = new byte[decoded.Length];
			for (int i = 0; i < decoded.Length; i++)
				data[i] = (byte)decoded[i];
			
			try {
				byte[] v2 = ProtectedData.Protect(data, null, DataProtectionScope.CurrentUser);
				return Convert.ToBase64String(v2);
			} catch {
				// XORs data. *NOT* very secure, only designed to prevent reading from options.txt.
				for (int i = 0; i < data.Length; i++)
					data[i] = (byte)(data[i] ^ key[i % key.Length] ^ 0x43);
				return Convert.ToBase64String(data);
			}
		}
		
		public static string Decode(string encoded, string key) {
			if (String.IsNullOrEmpty(encoded) || String.IsNullOrEmpty(key)) return "";
			
			byte[] data;
			try {
				data = Convert.FromBase64String(encoded);
			} catch (FormatException) {
				return "";
			}
			
			try {
				data = ProtectedData.Unprotect(data, null, DataProtectionScope.CurrentUser);
				char[] c = new char[data.Length];
				for (int i = 0; i < c.Length; i++)
					c[i] = (char)data[i];
				return new String(c);
			} catch {
				if (encoded.Length > 64 || data.Length > 64) return "";
				
				char[] c = new char[data.Length];
				for (int i = 0; i < c.Length; i++)
					c[i] = (char)(data[i] ^ key[i % key.Length] ^ 0x43);
				return new String(c);
			}
		}
	}
}
