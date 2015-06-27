// Copyright (c) 2009-2011 Dino Chiesa and Microsoft Corporation.
// All rights reserved.
// This code module is part of DotNetZip, a zipfile class library.

// This code is licensed under the Microsoft Public License.
// See the file License.txt for the license details.
// More info on: http://dotnetzip.codeplex.com

using System;
using Interop=System.Runtime.InteropServices;

namespace Ionic.Zlib
{
    public sealed class Adler
    {
        // largest prime smaller than 65536
        private static readonly uint BASE = 65521;
        // NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1
        private static readonly int NMAX = 5552;

        public static uint Adler32(uint adler, byte[] buf, int index, int len)
        {
            if (buf == null)
                return 1;

            uint s1 = (uint) (adler & 0xffff);
            uint s2 = (uint) ((adler >> 16) & 0xffff);

            while (len > 0)
            {
                int k = len < NMAX ? len : NMAX;
                len -= k;
                while (k >= 16)
                {
                    //s1 += (buf[index++] & 0xff); s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    s1 += buf[index++]; s2 += s1;
                    k -= 16;
                }
                if (k != 0) {
                    do {
                        s1 += buf[index++];
                        s2 += s1;
                    }
                    while (--k != 0);
                }
                s1 %= BASE;
                s2 %= BASE;
            }
            return (uint)((s2 << 16) | s1);
        }
    }
}