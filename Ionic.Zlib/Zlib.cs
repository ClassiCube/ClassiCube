// Copyright (c) 2009-2011 Dino Chiesa and Microsoft Corporation.
// All rights reserved.
// This code module is part of DotNetZip, a zipfile class library.

// This code is licensed under the Microsoft Public License.
// See the file License.txt for the license details.
// More info on: http://dotnetzip.codeplex.com

using System;

namespace Ionic.Zlib
{

    /// <summary>
    /// Describes how to flush the current deflate operation.
    /// </summary>
    /// <remarks>
    /// The different FlushType values are useful when using a Deflate in a streaming application.
    /// </remarks>
    public enum FlushType
    {
        /// <summary>No flush at all.</summary>
        None = 0,

        /// <summary>Closes the current block, but doesn't flush it to
        /// the output. Used internally only in hypothetical
        /// scenarios.  This was supposed to be removed by Zlib, but it is
        /// still in use in some edge cases.
        /// </summary>
        Partial,

        /// <summary>
        /// Use this during compression to specify that all pending output should be
        /// flushed to the output buffer and the output should be aligned on a byte
        /// boundary.  You might use this in a streaming communication scenario, so that
        /// the decompressor can get all input data available so far.  When using this
        /// with a ZlibCodec, <c>AvailableBytesIn</c> will be zero after the call if
        /// enough output space has been provided before the call.  Flushing will
        /// degrade compression and so it should be used only when necessary.
        /// </summary>
        Sync,

        /// <summary>
        /// Use this during compression to specify that all output should be flushed, as
        /// with <c>FlushType.Sync</c>, but also, the compression state should be reset
        /// so that decompression can restart from this point if previous compressed
        /// data has been damaged or if random access is desired.  Using
        /// <c>FlushType.Full</c> too often can significantly degrade the compression.
        /// </summary>
        Full,

        /// <summary>Signals the end of the compression/decompression stream.</summary>
        Finish,
    }


    /// <summary>
    /// The compression level to be used when using a DeflateStream or ZlibStream with CompressionMode.Compress.
    /// </summary>
    public enum CompressionLevel
    {
        /// <summary>
        /// None means that the data will be simply stored, with no change at all.
        /// If you are producing ZIPs for use on Mac OSX, be aware that archives produced with CompressionLevel.None
        /// cannot be opened with the default zip reader. Use a different CompressionLevel.
        /// </summary>
        None= 0,
        /// <summary>
        /// Same as None.
        /// </summary>
        Level0 = 0,

        /// <summary>
        /// The fastest but least effective compression.
        /// </summary>
        BestSpeed = 1,

        /// <summary>
        /// A synonym for BestSpeed.
        /// </summary>
        Level1 = 1,

        /// <summary>
        /// A little slower, but better, than level 1.
        /// </summary>
        Level2 = 2,

        /// <summary>
        /// A little slower, but better, than level 2.
        /// </summary>
        Level3 = 3,

        /// <summary>
        /// A little slower, but better, than level 3.
        /// </summary>
        Level4 = 4,

        /// <summary>
        /// A little slower than level 4, but with better compression.
        /// </summary>
        Level5 = 5,

        /// <summary>
        /// The default compression level, with a good balance of speed and compression efficiency.
        /// </summary>
        Default = 6,
        /// <summary>
        /// A synonym for Default.
        /// </summary>
        Level6 = 6,

        /// <summary>
        /// Pretty good compression!
        /// </summary>
        Level7 = 7,

        /// <summary>
        ///  Better compression than Level7!
        /// </summary>
        Level8 = 8,

        /// <summary>
        /// The "best" compression, where best means greatest reduction in size of the input data stream.
        /// This is also the slowest compression.
        /// </summary>
        BestCompression = 9,

        /// <summary>
        /// A synonym for BestCompression.
        /// </summary>
        Level9 = 9,
    }

    /// <summary>
    /// Describes options for how the compression algorithm is executed.  Different strategies
    /// work better on different sorts of data.  The strategy parameter can affect the compression
    /// ratio and the speed of compression but not the correctness of the compresssion.
    /// </summary>
    public enum CompressionStrategy
    {
        /// <summary>
        /// The default strategy is probably the best for normal data.
        /// </summary>
        Default = 0,

        /// <summary>
        /// The <c>Filtered</c> strategy is intended to be used most effectively with data produced by a
        /// filter or predictor.  By this definition, filtered data consists mostly of small
        /// values with a somewhat random distribution.  In this case, the compression algorithm
        /// is tuned to compress them better.  The effect of <c>Filtered</c> is to force more Huffman
        /// coding and less string matching; it is a half-step between <c>Default</c> and <c>HuffmanOnly</c>.
        /// </summary>
        Filtered = 1,

        /// <summary>
        /// Using <c>HuffmanOnly</c> will force the compressor to do Huffman encoding only, with no
        /// string matching.
        /// </summary>
        HuffmanOnly = 2,
    }


    /// <summary>
    /// An enum to specify the direction of transcoding - whether to compress or decompress.
    /// </summary>
    public enum CompressionMode
    {
        /// <summary>
        /// Used to specify that the stream should compress the data.
        /// </summary>
        Compress= 0,
        /// <summary>
        /// Used to specify that the stream should decompress the data.
        /// </summary>
        Decompress = 1,
    }

    public class ZlibException : System.Exception
    {
        public ZlibException() : base()
        {
        }

        public ZlibException(string s) : base(s) {
        }
    }


    internal class SharedUtils
    {
        public static int URShift(int number, int bits)
        {
            return (int)((uint)number >> bits);
        }
    }

    internal static class InternalConstants
    {
        internal static readonly int MAX_BITS     = 15;
        internal static readonly int BL_CODES     = 19;
        internal static readonly int D_CODES      = 30;
        internal static readonly int LITERALS     = 256;
        internal static readonly int LENGTH_CODES = 29;
        internal static readonly int L_CODES      = (LITERALS + 1 + LENGTH_CODES);

        // Bit length codes must not exceed MAX_BL_BITS bits
        internal static readonly int MAX_BL_BITS  = 7;

        // repeat previous bit length 3-6 times (2 bits of repeat count)
        internal static readonly int REP_3_6      = 16;

        // repeat a zero length 3-10 times  (3 bits of repeat count)
        internal static readonly int REPZ_3_10    = 17;

        // repeat a zero length 11-138 times  (7 bits of repeat count)
        internal static readonly int REPZ_11_138  = 18;

    }

    internal sealed class StaticTree
    {
        internal static readonly short[] lengthAndLiteralsTreeCodes = new short[] {
            12, 8, 140, 8, 76, 8, 204, 8, 44, 8, 172, 8, 108, 8, 236, 8,
            28, 8, 156, 8, 92, 8, 220, 8, 60, 8, 188, 8, 124, 8, 252, 8,
             2, 8, 130, 8, 66, 8, 194, 8, 34, 8, 162, 8, 98, 8, 226, 8,
            18, 8, 146, 8, 82, 8, 210, 8, 50, 8, 178, 8, 114, 8, 242, 8,
            10, 8, 138, 8, 74, 8, 202, 8, 42, 8, 170, 8, 106, 8, 234, 8,
            26, 8, 154, 8, 90, 8, 218, 8, 58, 8, 186, 8, 122, 8, 250, 8,
             6, 8, 134, 8, 70, 8, 198, 8, 38, 8, 166, 8, 102, 8, 230, 8,
            22, 8, 150, 8, 86, 8, 214, 8, 54, 8, 182, 8, 118, 8, 246, 8,
            14, 8, 142, 8, 78, 8, 206, 8, 46, 8, 174, 8, 110, 8, 238, 8,
            30, 8, 158, 8, 94, 8, 222, 8, 62, 8, 190, 8, 126, 8, 254, 8,
             1, 8, 129, 8, 65, 8, 193, 8, 33, 8, 161, 8, 97, 8, 225, 8,
            17, 8, 145, 8, 81, 8, 209, 8, 49, 8, 177, 8, 113, 8, 241, 8,
             9, 8, 137, 8, 73, 8, 201, 8, 41, 8, 169, 8, 105, 8, 233, 8,
            25, 8, 153, 8, 89, 8, 217, 8, 57, 8, 185, 8, 121, 8, 249, 8,
             5, 8, 133, 8, 69, 8, 197, 8, 37, 8, 165, 8, 101, 8, 229, 8,
            21, 8, 149, 8, 85, 8, 213, 8, 53, 8, 181, 8, 117, 8, 245, 8,
            13, 8, 141, 8, 77, 8, 205, 8, 45, 8, 173, 8, 109, 8, 237, 8,
            29, 8, 157, 8, 93, 8, 221, 8, 61, 8, 189, 8, 125, 8, 253, 8,
            19, 9, 275, 9, 147, 9, 403, 9, 83, 9, 339, 9, 211, 9, 467, 9,
            51, 9, 307, 9, 179, 9, 435, 9, 115, 9, 371, 9, 243, 9, 499, 9,
            11, 9, 267, 9, 139, 9, 395, 9, 75, 9, 331, 9, 203, 9, 459, 9,
            43, 9, 299, 9, 171, 9, 427, 9, 107, 9, 363, 9, 235, 9, 491, 9,
            27, 9, 283, 9, 155, 9, 411, 9, 91, 9, 347, 9, 219, 9, 475, 9,
            59, 9, 315, 9, 187, 9, 443, 9, 123, 9, 379, 9, 251, 9, 507, 9,
             7, 9, 263, 9, 135, 9, 391, 9, 71, 9, 327, 9, 199, 9, 455, 9,
            39, 9, 295, 9, 167, 9, 423, 9, 103, 9, 359, 9, 231, 9, 487, 9,
            23, 9, 279, 9, 151, 9, 407, 9, 87, 9, 343, 9, 215, 9, 471, 9,
            55, 9, 311, 9, 183, 9, 439, 9, 119, 9, 375, 9, 247, 9, 503, 9,
            15, 9, 271, 9, 143, 9, 399, 9, 79, 9, 335, 9, 207, 9, 463, 9,
            47, 9, 303, 9, 175, 9, 431, 9, 111, 9, 367, 9, 239, 9, 495, 9,
            31, 9, 287, 9, 159, 9, 415, 9, 95, 9, 351, 9, 223, 9, 479, 9,
            63, 9, 319, 9, 191, 9, 447, 9, 127, 9, 383, 9, 255, 9, 511, 9,
             0, 7, 64, 7, 32, 7, 96, 7, 16, 7, 80, 7, 48, 7, 112, 7,
             8, 7, 72, 7, 40, 7, 104, 7, 24, 7, 88, 7, 56, 7, 120, 7,
             4, 7, 68, 7, 36, 7, 100, 7, 20, 7, 84, 7, 52, 7, 116, 7,
             3, 8, 131, 8, 67, 8, 195, 8, 35, 8, 163, 8, 99, 8, 227, 8
        };

        internal static readonly short[] distTreeCodes = new short[] {
            0, 5, 16, 5, 8, 5, 24, 5, 4, 5, 20, 5, 12, 5, 28, 5,
            2, 5, 18, 5, 10, 5, 26, 5, 6, 5, 22, 5, 14, 5, 30, 5,
            1, 5, 17, 5, 9, 5, 25, 5, 5, 5, 21, 5, 13, 5, 29, 5,
            3, 5, 19, 5, 11, 5, 27, 5, 7, 5, 23, 5 };

        internal static readonly StaticTree Literals;
        internal static readonly StaticTree Distances;
        internal static readonly StaticTree BitLengths;

        internal short[] treeCodes; // static tree or null
        internal int[] extraBits;   // extra bits for each code or null
        internal int extraBase;     // base index for extra_bits
        internal int elems;         // max number of elements in the tree
        internal int maxLength;     // max bit length for the codes

        private StaticTree(short[] treeCodes, int[] extraBits, int extraBase, int elems, int maxLength)
        {
            this.treeCodes = treeCodes;
            this.extraBits = extraBits;
            this.extraBase = extraBase;
            this.elems = elems;
            this.maxLength = maxLength;
        }
        static StaticTree()
        {
            Literals = new StaticTree(lengthAndLiteralsTreeCodes, Tree.ExtraLengthBits, InternalConstants.LITERALS + 1, InternalConstants.L_CODES, InternalConstants.MAX_BITS);
            Distances = new StaticTree(distTreeCodes, Tree.ExtraDistanceBits, 0, InternalConstants.D_CODES, InternalConstants.MAX_BITS);
            BitLengths = new StaticTree(null, Tree.extra_blbits, 0, InternalConstants.BL_CODES, InternalConstants.MAX_BL_BITS);
        }
    }
}