using System;
using System.Collections.Generic;
using System.IO;
using csogg;
using csvorbis;
using SharpWave.Containers;

namespace SharpWave.Codecs.Vorbis {
	
	public sealed class OggContainer : IMediaContainer {
		
		public OggContainer( Stream source ) : base( source ) {
		}
		
		public override void ReadMetadata() {
			// this would be a good place to read vorbis headers
		}
		
		public override ICodec GetAudioCodec() {
			return new VorbisCodec();
		}
	}
	
	public sealed class VorbisCodec : ICodec {
		public string Name { get { return "Xiph.org Vorbis"; } }
		
		public VorbisCodec() {
			chunk = new AudioChunk();
		}

		AudioChunk chunk, rawChunk;
		Stream input;
		byte[] rawPcm;
		int rawIndex;
		
		SyncState oy = new SyncState(); // sync and verify incoming physical bitstream
		StreamState os = new StreamState(); // take physical pages, weld into a logical stream of packets
		Page og = new Page(); // one Ogg bitstream page.  Vorbis packets are inside
		Packet op = new Packet(); // one raw packet of data for decode

		Info vi = new Info();  // struct that stores all the static vorbis bitstream settings
		Comment vc = new Comment(); // struct that stores all the bitstream user comments
		DspState vd = new DspState(); // central working state for the packet->PCM decoder
		Block vb = new Block(); // local working space for packet->PCM decode

		byte[] buffer;
		int bytes = 0;
		
		public AudioFormat ReadHeader(Stream source) {
			input = source;
			oy.init(); // Now we can read pages

			// grab some data at the head of the stream.  We want the first page
			// (which is guaranteed to be small and only contain the Vorbis
			// stream initial header) We need the first page to get the stream serialno.

			// submit a 4k block to libvorbis' Ogg layer
			int index = oy.buffer(4096);
			buffer = oy.data;
			bytes = input.Read(buffer, index, 4096);
			oy.wrote(bytes);

			// Get the first page.
			if (oy.pageout(og) != 1) {
				// have we simply run out of data?  If so, we're done.
				if (bytes < 4096) return default(AudioFormat);
			}

			// Get the serial number and set up the rest of decode.
			// serialno first; use it to set up a logical stream
			os.init(og.serialno());

			// extract the initial header from the first page and verify that the
			// Ogg bitstream is in fact Vorbis data

			// I handle the initial header first instead of just having the code
			// read all three Vorbis headers at once because reading the initial
			// header is an easy way to identify a Vorbis bitstream and it's
			// useful to see that functionality seperated out.

			vi.init();
			vc.init();
			os.pagein(og);
			os.packetout(op);
			vi.synthesis_headerin(vc, op);

			// At this point, we're sure we're Vorbis.  We've set up the logical
			// (Ogg) bitstream decoder.  Get the comment and codebook headers and
			// set up the Vorbis decoder

			// The next two packets in order are the comment and codebook headers.
			// They're likely large and may span multiple pages.  Thus we reead
			// and submit data until we get our two packets, watching that no
			// pages are missing.  If a page is missing, error out; losing a
			// header page is the only place where missing data is fatal. */

			int i = 0;
			while (i < 2) {
				while (i < 2) {
					int result = oy.pageout(og);
					if (result == 0) break; // Need more data
					// Don't complain about missing or corrupt data yet.  We'll
					// catch it at the packet output phase

					if (result == 1) {
						os.pagein(og); // we can ignore any errors here
						// as they'll also become apparent
						// at packetout
						while (i < 2) {
							result = os.packetout(op);
							if (result == 0) break;
							vi.synthesis_headerin(vc, op);
							i++;
						}
					}
				}
				
				// no harm in not checking before adding more
				index = oy.buffer(4096);
				buffer = oy.data;
				bytes = input.Read(buffer, index, 4096);
				oy.wrote(bytes);
			}

			// OK, got and parsed all three headers. Initialize the Vorbis
			//  packet->PCM decoder.
			vd.synthesis_init(vi); // central decode state
			vb.init(vd);           // local state for most of the decode
			
			AudioFormat format;
			format.Channels = vi.channels;
			format.BitsPerSample = 16;
			format.SampleRate = vi.rate;
			return format;
		}
		
		public IEnumerable<AudioChunk> StreamData( Stream source ) {
			// the original iterator may not always return enough samples,
			// so we will do our own buffering here.
			
			rawChunk = new AudioChunk();
			foreach( AudioChunk chunk in StreamDataCore( source ) ) {
				if( rawPcm == null )
					InitRaw( chunk );
				if( rawIndex + chunk.Length > rawPcm.Length )
					ResizeRaw( rawIndex + chunk.Length );
				
				Buffer.BlockCopy( chunk.Data, 0, rawPcm, rawIndex, chunk.Length );
				rawIndex += chunk.Length;
				if( rawIndex >= (vi.rate / 4) ) {
					rawChunk.Length = rawIndex;
					rawIndex = 0;
					yield return rawChunk;
				}
			}
			
			rawChunk.Length = rawIndex;
			yield return rawChunk;
			yield break;
		}
		
		void InitRaw(AudioChunk chunk) {
			rawPcm = new byte[vi.rate / 4];
			rawChunk.Data = rawPcm;
			rawChunk.Length = rawPcm.Length;
		}
		
		void ResizeRaw(int newLen) {
			byte[] oldPcm = rawPcm;
			rawPcm = new byte[rawIndex + chunk.Length];
			Buffer.BlockCopy( oldPcm, 0, rawPcm, 0, rawIndex );
			rawChunk.Data = rawPcm;
			rawChunk.Length = rawPcm.Length;
		}
		
		IEnumerable<AudioChunk> StreamDataCore( Stream source ) {
			input = source;
			int convsize = 4096 * 2;
			byte[] convbuffer = new byte[convsize]; // take 8k out of the data segment, not the stack
			convsize = 4096 / vi.channels;
			oy.init(); // Now we can read pages

			int eos = 0;
			// so multiple block decodes can
			// proceed in parallel.  We could init
			// multiple vorbis_block structures for vd here

			float[][] pcm = null;
			int[] _index = new int[vi.channels];
			// The rest is just a straight decode loop until end of stream
			
			while (eos == 0) {
				while (eos == 0) {
					int result = oy.pageout(og);
					if (result == 0) break; // need more data
					
					os.pagein(og); // can safely ignore errors at this point
					while (true) {
						result = os.packetout(op);

						if (result == 0) break; // need more data
						if (result == -1) continue;
						// missing or corrupt data at this page position
						// no reason to complain; already complained above
						
						// we have a packet. Decode it
						int samples;
						if (vb.synthesis(op) == 0) { // test for success!
							vd.synthesis_blockin(vb);
						}

						// **pcm is a multichannel float vector.  In stereo, for
						// example, pcm[0] is left, and pcm[1] is right.  samples is
						// the size of each channel.  Convert the float values
						// (-1.<=range<=1.) to whatever PCM format and write it out

						while ((samples = vd.synthesis_pcmout(ref pcm, _index)) > 0) {
							int bout = (samples < convsize ? samples : convsize);

							// convert floats to 16 bit signed ints (host order) and interleave
							for (int ch = 0; ch < vi.channels; ch++) {
								int ptr = ch * 2;
								int offset = _index[ch];
								float[] chPcm = pcm[ch];
								
								for (int j = 0; j < bout; j++) {
									int val = (int)(chPcm[offset + j] * 32767);
									if (val > 32767) val = 32767;
									if (val < -32768) val = -32768;
									if (val < 0) val = val | 0x8000;
									
									convbuffer[ptr] = (byte)(val);
									convbuffer[ptr + 1] = (byte)((uint)val >> 8);
									ptr += 2 * (vi.channels);
								}
							}
							
							chunk.Data = convbuffer;
							chunk.Length = 2 * vi.channels * bout;
							vd.synthesis_read(bout);
							yield return chunk;
						}
					}
					if (og.eos() != 0) eos = 1;
				}
				
				if (eos == 0) {
					int index = oy.buffer(4096);
					buffer = oy.data;
					bytes = input.Read(buffer, index, 4096);
					oy.wrote(bytes);
					if (bytes == 0) eos = 1;
				}
			}

			// clean up this logical bitstream; before exit we see if we're
			// followed by another [chained]
			os.clear();

			// ogg_page and ogg_packet structs always point to storage in
			// libvorbis.  They're never freed or manipulated directly
			vi.clear();  // must be called last

			// OK, clean up the framer
			oy.clear();
			yield break;
		}
	}
}
