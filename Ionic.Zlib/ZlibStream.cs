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

    public class ZlibStream : Stream
    {
        internal ZlibBaseStream _baseStream;
        bool _disposed;

        public ZlibStream(Stream stream, CompressionMode mode)
            : this(stream, mode, CompressionLevel.Default, false) {
        }

        public ZlibStream(Stream stream, CompressionMode mode, CompressionLevel level)
            : this(stream, mode, level, false) {
        }

        public ZlibStream(Stream stream, CompressionMode mode, bool leaveOpen)
            : this(stream, mode, CompressionLevel.Default, leaveOpen) {
        }

        public ZlibStream(Stream stream, CompressionMode mode, CompressionLevel level, bool leaveOpen) {
            _baseStream = new ZlibBaseStream(stream, mode, level, ZlibStreamFlavor.ZLIB, leaveOpen);
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
                        this._baseStream.Close();
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
                if (_disposed) throw new ObjectDisposedException("ZlibStream");
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
                if (_disposed) throw new ObjectDisposedException("ZlibStream");
                return _baseStream._stream.CanWrite;
            }
        }

        public override void Flush()
        {
            if (_disposed) throw new ObjectDisposedException("ZlibStream");
            _baseStream.Flush();
        }

        public override long Length
        {
            get { throw new NotSupportedException(); }
        }

        public override long Position
        {
            get
            {
                if (this._baseStream._streamMode == Ionic.Zlib.ZlibBaseStream.StreamMode.Writer)
                    return this._baseStream._z.TotalBytesOut;
                if (this._baseStream._streamMode == Ionic.Zlib.ZlibBaseStream.StreamMode.Reader)
                    return this._baseStream._z.TotalBytesIn;
                return 0;
            }

            set { throw new NotSupportedException(); }
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
                if (_disposed) throw new ObjectDisposedException("ZlibStream");
            return _baseStream.Read(buffer, offset, count);
        }

        public override long Seek(long offset, SeekOrigin origin)  {
            throw new NotSupportedException();
        }

        public override void SetLength(long value) {
            throw new NotSupportedException();
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
                if (_disposed) throw new ObjectDisposedException("ZlibStream");
            _baseStream.Write(buffer, offset, count);
        }

        public static byte[] CompressBuffer(byte[] b)
        {
            using (var ms = new MemoryStream())
            {
                Stream compressor =  new ZlibStream( ms, CompressionMode.Compress, CompressionLevel.BestCompression );
                ZlibBaseStream.CompressBuffer(b, compressor);
                return ms.ToArray();
            }
        }

        public static byte[] UncompressBuffer(byte[] compressed)
        {
            using (var input = new MemoryStream(compressed))
            {
                Stream decompressor =  new ZlibStream( input, CompressionMode.Decompress );
                return ZlibBaseStream.UncompressBuffer(compressed, decompressor);
            }
        }

    }
}