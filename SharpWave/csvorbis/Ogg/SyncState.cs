/* csogg
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

namespace csogg
{
	public class SyncState
	{
		public byte[] data;
		int storage;
		int fill;
		int returned;

		int unsynced;
		int headerbytes;
		int bodybytes;

		public int clear()
		{
			data=null;
			return(0);
		}

		// !!!!!!!!!!!!
		//  byte[] buffer(int size){
		public int buffer(int size)
		{
			// first, clear out any space that has been previously returned
			if(returned!=0)
			{
				fill-=returned;
				if(fill>0)
				{
					Array.Copy(data, returned, data, 0, fill);
				}
				returned=0;
			}

			if(size>storage-fill)
			{
				// We need to extend the internal buffer
				int newsize=size+fill+4096; // an extra page to be nice
				if(data!=null)
				{
					byte[] foo=new byte[newsize];
					Array.Copy(data, 0, foo, 0, data.Length);
					data=foo;
				}
				else
				{
					data=new byte[newsize];
				}
				storage=newsize;
			}

			// expose a segment at least as large as requested at the fill mark
			//    return((char *)oy->data+oy->fill);
			//    return(data);
			return(fill);
		}

		public int wrote(int bytes)
		{
			if(fill+bytes>storage)return(-1);
			fill+=bytes;
			return(0);
		}

		// sync the stream.  This is meant to be useful for finding page
		// boundaries.
		//
		// return values for this:
		// -n) skipped n bytes
		//  0) page not ready; more data (no bytes skipped)
		//  n) page synced at current location; page length n bytes
		private Page pageseek_p=new Page();
		private  byte[] chksum=new byte[4];
		public int pageseek(Page og)
		{
			int page=returned;
			int next;
			int bytes=fill-returned;
			
			if(headerbytes==0)
			{
				int _headerbytes,i;
				if(bytes<27)return(0); // not enough for a header
				
				/* verify capture pattern */
				//!!!!!!!!!!!
				if(data[page]!='O' || data[page+1]!='g' || data[page+2]!='g' || data[page+3]!='S')
				{
					headerbytes=0;
					bodybytes=0;
					
					// search for possible capture
					next=0;
					for(int ii=0; ii<bytes-1; ii++)
					{
						if(data[page+1+ii]=='O'){next=page+1+ii; break;}
					}
					//next=memchr(page+1,'O',bytes-1);
					if(next==0) next=fill;

					returned=next;
					return(-(next-page));
				}
				_headerbytes=(data[page+26]&0xff)+27;
				if(bytes<_headerbytes)return(0); // not enough for header + seg table
				
				// count up body length in the segment table
				
				for(i=0;i<(data[page+26]&0xff);i++)
				{
					bodybytes+=(data[page+27+i]&0xff);
				}
				headerbytes=_headerbytes;
			}
			
			if(bodybytes+headerbytes>bytes)return(0);
			
			// The whole test page is buffered.  Verify the checksum
			lock(chksum)
			{
				// Grab the checksum bytes, set the header field to zero
				
				Array.Copy(data, page+22, chksum, 0, 4);
				data[page+22]=0;
				data[page+23]=0;
				data[page+24]=0;
				data[page+25]=0;
				
				// set up a temp page struct and recompute the checksum
				Page log=pageseek_p;
				log.header_base=data;
				log.header=page;
				log.header_len=headerbytes;

				log.body_base=data;
				log.body=page+headerbytes;
				log.body_len=bodybytes;
				log.checksum();

				// Compare
				if(chksum[0]!=data[page+22] ||
				   chksum[1]!=data[page+23] ||
				   chksum[2]!=data[page+24] ||
				   chksum[3]!=data[page+25])
				{
					// D'oh.  Mismatch! Corrupt page (or miscapture and not a page at all)
					// replace the computed checksum with the one actually read in
					Array.Copy(chksum, 0, data, page+22, 4);
					// Bad checksum. Lose sync */

					headerbytes=0;
					bodybytes=0;
					// search for possible capture
					next=0;
					for(int ii=0; ii<bytes-1; ii++)
					{
						if(data[page+1+ii]=='O'){next=page+1+ii; break;}
					}
					//next=memchr(page+1,'O',bytes-1);
					if(next==0) next=fill;
					returned=next;
					return(-(next-page));
				}
			}
			
			// yes, have a whole page all ready to go
			{
				page=returned;

				if(og!=null)
				{
					og.header_base=data;
					og.header=page;
					og.header_len=headerbytes;
					og.body_base=data;
					og.body=page+headerbytes;
					og.body_len=bodybytes;
				}

				unsynced=0;
				returned+=(bytes=headerbytes+bodybytes);
				headerbytes=0;
				bodybytes=0;
				return(bytes);
			}
		}


		// sync the stream and get a page.  Keep trying until we find a page.
		// Supress 'sync errors' after reporting the first.
		//
		// return values:
		//  -1) recapture (hole in data)
		//   0) need more data
		//   1) page returned
		//
		// Returns pointers into buffered data; invalidated by next call to
		// _stream, _clear, _init, or _buffer

		public int pageout(Page og)
		{
			// all we need to do is verify a page at the head of the stream
			// buffer.  If it doesn't verify, we look for the next potential
			// frame

			while(true)
			{
				int ret=pageseek(og);
				if(ret>0)
				{
					// have a page
					return(1);
				}
				if(ret==0)
				{
					// need more data
					return(0);
				}
				
				// head did not start a synced page... skipped some bytes
				if(unsynced==0)
				{
					unsynced=1;
					return(-1);
				}
				// loop. keep looking
			}
		}

		// clear things to an initial state.  Good to call, eg, before seeking
		public int reset()
		{
			fill=0;
			returned=0;
			unsynced=0;
			headerbytes=0;
			bodybytes=0;
			return(0);
		}
		public void init(){}

		public SyncState()
		{
			// No constructor needed
		}
	}
}
