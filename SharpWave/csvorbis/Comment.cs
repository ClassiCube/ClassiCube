/* csvorbis
 * Copyright (C) 2000 ymnk, JCraft,Inc.
 *
 * Written by: 2000 ymnk<ymnk@jcraft.com>
 * Ported to C# from JOrbis by: Mark Crichton <crichton@gimp.org>
 *
 * Thanks go to the JOrbis team, for licencing the code under the
 * LGPL, making my job a lot easier.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
   
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

using System;
using System.Text;
using csogg;

namespace csvorbis
{
	public class Comment
	{
		// unlimited user comment fields.  libvorbis writes 'libvorbis'
		// whatever vendor is set to in encode
		public byte[][] user_comments;
		public int comments;
		public byte[] vendor;

		public void init()
		{
			user_comments = null;
			comments = 0;
			vendor = null;
		}

		internal int unpack(csBuffer opb)
		{
			int vendorlen = opb.read(32);
			vendor = new byte[vendorlen + 1];
			opb.read(vendor, vendorlen);
			comments = opb.read(32);
			user_comments = new byte[comments + 1][];
			
			for(int i = 0; i < comments; i++) {
				int len = opb.read(32);
				user_comments[i] = new byte[len+1];
				opb.read(user_comments[i], len);
			}
			
			opb.read(1);
			return(0);
		}
		
		internal void clear()
		{
			user_comments = null;
			vendor = null;
		}

		public string getVendor()
		{
			return Encoding.UTF8.GetString(vendor);
		}

		public string getComment(int i)
		{
			Encoding AE = Encoding.UTF8;
			if(comments <= i)return null;
			return Encoding.UTF8.GetString(user_comments[i]);
		}

		public string toString()
		{
			String sum = "Vendor: " + getVendor();
			for(int i = 0; i < comments; i++)
				sum = sum + "\nComment: " + getComment(i);
			sum = sum + "\n";
			return sum;
		}
	}
}
