// Copyright (c) 2009-2011 Dino Chiesa and Microsoft Corporation.
// All rights reserved.
// This code module is part of DotNetZip, a zipfile class library.

// This code is licensed under the Microsoft Public License.
// See the file License.txt for the license details.
// More info on: http://dotnetzip.codeplex.com


using System;
using System.IO;

namespace Ionic.Zlib
{
    public class GZipStream : Stream
    {      
        private int _headerByteCount;
        internal ZlibBaseStream _baseStream;
        bool _disposed;

        public GZipStream(Stream stream, CompressionMode mode)
            : this(stream, mode, CompressionLevel.Default, false) {
        }

        public GZipStream(Stream stream, CompressionMode mode, CompressionLevel level)
            : this(stream, mode, level, false) {
        }

        public GZipStream(Stream stream, CompressionMode mode, bool leaveOpen)
            : this(stream, mode, CompressionLevel.Default, leaveOpen) {
        }

        public GZipStream(Stream stream, CompressionMode mode, CompressionLevel level, bool leaveOpen) {
            _baseStream = new ZlibBaseStream(stream, mode, level, ZlibStreamFlavor.GZIP, leaveOpen);
        }

        public virtual FlushType FlushMode {
            get { return this._baseStream._flushMode; }
            set {
                if (_disposed) throw new ObjectDisposedException("GZipStream");
                this._baseStream._flushMode = value;
            }
        }

        /// <summary> Returns the total number of bytes input so far.</summary>
        public virtual long TotalIn {
            get { return this._baseStream._z.TotalBytesIn; }
        }

        /// <summary> Returns the total number of bytes output so far.</summary>
        public virtual long TotalOut {
            get { return this._baseStream._z.TotalBytesOut; }
        }

        protected override void Dispose(bool disposing)
        {
            try
            {
                if (!_disposed)
                {
                    if (disposing && (this._baseStream != null))
                    {
                        this._baseStream.Close();
                    }
                    _disposed = true;
                }
            }
            finally
            {
                base.Dispose(disposing);
            }
        }

        public override bool CanRead
        {
            get
            {
                if (_disposed) throw new ObjectDisposedException("GZipStream");
                return _baseStream._stream.CanRead;
            }
        }

        public override bool CanSeek
        {
            get { return false; }
        }

        public override bool CanWrite
        {
            get
            {
                if (_disposed) throw new ObjectDisposedException("GZipStream");
                return _baseStream._stream.CanWrite;
            }
        }

        public override void Flush()
        {
            if (_disposed) throw new ObjectDisposedException("GZipStream");
            _baseStream.Flush();
        }

        /// <summary>
        /// Reading this property always throws a <see cref="NotImplementedException"/>.
        /// </summary>
        public override long Length
        {
            get { throw new NotImplementedException(); }
        }

        public override long Position
        {
            get
            {
                if (this._baseStream._streamMode == Ionic.Zlib.ZlibBaseStream.StreamMode.Writer)
                    return this._baseStream._z.TotalBytesOut + _headerByteCount;
                if (this._baseStream._streamMode == Ionic.Zlib.ZlibBaseStream.StreamMode.Reader)
                    return this._baseStream._z.TotalBytesIn + this._baseStream._gzipHeaderByteCount;
                return 0;
            }

            set { throw new NotImplementedException(); }
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            if (_disposed) throw new ObjectDisposedException("GZipStream");
            return _baseStream.Read(buffer, offset, count);
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new NotImplementedException();
        }

        public override void SetLength(long value)
        {
            throw new NotImplementedException();
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            if (_disposed) throw new ObjectDisposedException("GZipStream");
            if (_baseStream._streamMode == Ionic.Zlib.ZlibBaseStream.StreamMode.Undefined)
            {
                //Console.WriteLine("GZipStream: First write");
                if (_baseStream._wantCompress) {
                    // first write in compression, therefore, emit the GZIP header
                    _headerByteCount = EmitHeader();
                } else {
                    throw new InvalidOperationException();
                }
            }

            _baseStream.Write(buffer, offset, count);
        }

        private int EmitHeader()
        {
            byte[] header = new byte[10];          
            header[0] = 0x1F;  // ID 1
            header[1] = 0x8B;  // ID 2
            header[2] = 8;  // compression method
            header[9] = 0xFF; // OS, 0xFF == unspecified

            _baseStream._stream.Write(header, 0, header.Length);
            return header.Length; // bytes written
        }

        public static byte[] CompressBuffer(byte[] b)
        {
            using (var ms = new MemoryStream())
            {
                Stream compressor = new GZipStream( ms, CompressionMode.Compress, CompressionLevel.BestCompression );
                ZlibBaseStream.CompressBuffer(b, compressor);
                return ms.ToArray();
            }
        }

        public static byte[] UncompressBuffer(byte[] compressed)
        {
            using (var input = new MemoryStream(compressed))
            {
                Stream decompressor = new GZipStream( input, CompressionMode.Decompress );
                return ZlibBaseStream.UncompressBuffer(compressed, decompressor);
            }
        }
    }
}