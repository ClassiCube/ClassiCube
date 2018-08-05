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
using System.Text;

namespace csogg
{
	public class StreamState
	{
		byte[] body_data;    /* bytes from packet bodies */
		int body_storage;    /* storage elements allocated */
		int body_fill;       /* elements stored; fill mark */
		private  int body_returned;   /* elements of fill returned */


		int[] lacing_vals;    /* The values that will go to the segment table */
		long[] granule_vals;  /* pcm_pos values for headers. Not compact
			   this way, but it is simple coupled to the
			   lacing fifo */
		int lacing_storage;
		int lacing_fill;
		int lacing_packet;
		int lacing_returned;

		public int e_o_s;   /* set when we have buffered the last packet in the
			 logical bitstream */
		int serialno;
		int pageno;
		long packetno;      /* sequence number for decode; the framing
                         knows where there's a hole in the data,
                         but we need coupling so that the codec
                         (which is in a seperate abstraction
                         layer) also knows about the gap */
		long granulepos;

		StreamState(int serialno) : this()
		{
			init(serialno);
		}

		public StreamState()
		{
			init();
		}

		void init()
		{
			body_storage=16*1024;
			body_data=new byte[body_storage];
			lacing_storage=1024;
			lacing_vals=new int[lacing_storage];
			granule_vals=new long[lacing_storage];
		}
		public void init(int serialno)
		{
			if(body_data==null){ init(); }
			else
			{
				for(int i=0; i<body_data.Length; i++) body_data[i]=0;
				for(int i=0; i<lacing_vals.Length; i++) lacing_vals[i]=0;
				for(int i=0; i<granule_vals.Length; i++) granule_vals[i]=0;
			}
			this.serialno=serialno;
		}
		public void clear()
		{
			body_data=null;
			lacing_vals=null;
			granule_vals=null;
		}
		
		void destroy()
		{
			clear();
		}
		void body_expand(int needed)
		{
			if(body_storage<=body_fill+needed)
			{
				body_storage+=(needed+1024);
				byte[] foo=new byte[body_storage];
				Array.Copy(body_data, 0, foo, 0, body_data.Length);
				body_data=foo;
			}
		}
		void lacing_expand(int needed)
		{
			if(lacing_storage<=lacing_fill+needed)
			{
				lacing_storage+=(needed+32);
				int[] foo=new int[lacing_storage];
				Array.Copy(lacing_vals, 0, foo, 0, lacing_vals.Length);
				lacing_vals=foo;

				long[] bar=new long[lacing_storage];
				Array.Copy(granule_vals, 0, bar, 0, granule_vals.Length);
				granule_vals=bar;
			}
		}

		/* submit data to the internal buffer of the framing engine */
		public int packetin(Packet op)
		{
			int lacing_val=op.bytes/255+1;

			if(body_returned!=0)
			{
				/* advance packet data according to the body_returned pointer. We
					   had to keep it around to return a pointer into the buffer last
					   call */
				
				body_fill-=body_returned;
				if(body_fill!=0)
				{
					Array.Copy(body_data, body_returned, body_data, 0, body_fill);
				}
				body_returned=0;
			}

			/* make sure we have the buffer storage */
			body_expand(op.bytes);
			lacing_expand(lacing_val);

			/* Copy in the submitted packet.  Yes, the copy is a waste; this is
				   the liability of overly clean abstraction for the time being.  It
				   will actually be fairly easy to eliminate the extra copy in the
				   future */

			Array.Copy(op.packet_base, op.packet, body_data, body_fill, op.bytes);
			body_fill+=op.bytes;
			//System.out.println("add: "+body_fill);

			/* Store lacing vals for this packet */
			int j;
			for(j=0;j<lacing_val-1;j++)
			{
				lacing_vals[lacing_fill+j]=255;
				granule_vals[lacing_fill+j]=granulepos;
			}
			lacing_vals[lacing_fill+j]=(op.bytes)%255;
			granulepos=granule_vals[lacing_fill+j]=op.granulepos;

			/* flag the first segment as the beginning of the packet */
			lacing_vals[lacing_fill]|= 0x100;

			lacing_fill+=lacing_val;

			/* for the sake of completeness */
			packetno++;

			if(op.e_o_s!=0)e_o_s=1;
			return(0);
		}

		public int packetout(Packet op)
		{

			/* The last part of decode. We have the stream broken into packet
				   segments.  Now we need to group them into packets (or return the
				   out of sync markers) */

			int ptr=lacing_returned;

			if(lacing_packet<=ptr)
			{
				return(0);
			}

			if((lacing_vals[ptr]&0x400)!=0)
			{
				/* We lost sync here; let the app know */
				lacing_returned++;

				/* we need to tell the codec there's a gap; it might need to
					   handle previous packet dependencies. */
				packetno++;
				return(-1);
			}

			/* Gather the whole packet. We'll have no holes or a partial packet */
			{
				int size=lacing_vals[ptr]&0xff;
				int bytes=0;

				op.packet_base=body_data;
				op.packet=body_returned;
				op.e_o_s=lacing_vals[ptr]&0x200; /* last packet of the stream? */
				op.b_o_s=lacing_vals[ptr]&0x100; /* first packet of the stream? */
				bytes+=size;

				while(size==255)
				{
					int val=lacing_vals[++ptr];
					size=val&0xff;
					if((val&0x200)!=0)op.e_o_s=0x200;
					bytes+=size;
				}

				op.packetno=packetno;
				op.granulepos=granule_vals[ptr];
				op.bytes=bytes;

				//System.out.println(this+" # body_returned="+body_returned);
				body_returned+=bytes;
				//System.out.println(this+"## body_returned="+body_returned);

				lacing_returned=ptr+1;
			}
			packetno++;
			return(1);
		}


		// add the incoming page to the stream state; we decompose the page
		// into packet segments here as well.
		public int pagein(Page og)
		{
			byte[] header_base=og.header_base;
			int header=og.header;
			byte[] body_base=og.body_base;
			int body=og.body;
			int bodysize=og.body_len;
			int segptr=0;

			int version=og.version();
			int continued=og.continued();
			int bos=og.bos();
			int eos=og.eos();
			long granulepos=og.granulepos();
			int _serialno=og.serialno();
			int _pageno=og.pageno();
			int segments=header_base[header+26]&0xff;

			// clean up 'returned data'
			{
				int lr=lacing_returned;
				int br=body_returned;

				// body data

				//System.out.println("br="+br+", body_fill="+body_fill);

				if(br!=0)
				{
					body_fill-=br;
					if(body_fill!=0)
					{
						Array.Copy(body_data, br, body_data, 0, body_fill);
					}
					body_returned=0;
				}

				//System.out.println("?? br="+br+", body_fill="+body_fill+" body_returned="+body_returned);

				if(lr!=0)
				{
					// segment table
					if((lacing_fill-lr)!=0)
					{
						Array.Copy(lacing_vals, lr, lacing_vals, 0, lacing_fill-lr);
						Array.Copy(granule_vals, lr, granule_vals, 0, lacing_fill-lr);
					}
					lacing_fill-=lr;
					lacing_packet-=lr;
					lacing_returned=0;
				}
			}

			// check the serial number
			if(_serialno!=serialno)return(-1);
			if(version>0)return(-1);

			lacing_expand(segments+1);

			// are we in sequence?
			if(_pageno!=pageno)
			{
				int i;

				// unroll previous partial packet (if any)
				for(i=lacing_packet;i<lacing_fill;i++)
				{
					body_fill-=lacing_vals[i]&0xff;
					//System.out.println("??");
				}
				lacing_fill=lacing_packet;

				// make a note of dropped data in segment table
				if(pageno!=-1)
				{
					lacing_vals[lacing_fill++]=0x400;
					lacing_packet++;
				}

				// are we a 'continued packet' page?  If so, we'll need to skip
				// some segments
				if(continued!=0)
				{
					bos=0;
					for(;segptr<segments;segptr++)
					{
						int val=(header_base[header+27+segptr]&0xff);
						body+=val;
						bodysize-=val;
						if(val<255)
						{
							segptr++;
							break;
						}
					}
				}
			}

			//System.out.println("bodysize="+bodysize);

			if(bodysize!=0)
			{
				body_expand(bodysize);
				Array.Copy(body_base, body, body_data, body_fill, bodysize);
				body_fill+=bodysize;
			}

			//System.out.println("bodyfill="+body_fill);

			{
				int saved=-1;
				while(segptr<segments)
				{
					int val=(header_base[header+27+segptr]&0xff);
					lacing_vals[lacing_fill]=val;
					granule_vals[lacing_fill]=-1;
					
					if(bos!=0)
					{
						lacing_vals[lacing_fill]|=0x100;
						bos=0;
					}
					
					if(val<255)saved=lacing_fill;
					
					lacing_fill++;
					segptr++;
					
					if(val<255)lacing_packet=lacing_fill;
				}
				
				/* set the granulepos on the last pcmval of the last full packet */
				if(saved!=-1)
				{
					granule_vals[saved]=granulepos;
				}
			}

			if(eos!=0)
			{
				e_o_s=1;
				if(lacing_fill>0)
					lacing_vals[lacing_fill-1]|=0x200;
			}

			pageno=_pageno+1;
			return(0);
		}
	}
}
