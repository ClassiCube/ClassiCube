// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

namespace ClassicalSharp.Network {
    public static class PingList {
        
        public struct PingEntry {
            public DateTime TimeSent, TimeReceived;
            public ushort Data;
            public double Latency { get {
                    // Half, because received->reply time is actually twice time it takes to send data
                    return (TimeReceived - TimeSent).TotalMilliseconds * 0.5;
                } }
        }
        
        public static PingEntry[] Entries = new PingEntry[10];
        
        
        public static ushort NextTwoWayPingData() {
            // Find free ping slot
            for (int i = 0; i < Entries.Length; i++) {
                if (Entries[i].TimeSent.Ticks != 0) continue;
                
                ushort prev = i > 0 ? Entries[i - 1].Data : (ushort)0;
                return SetTwoWayPing(i, prev);
            }
            
            // Remove oldest ping slot
            for (int i = 0; i < Entries.Length - 1; i++) {
                Entries[i] = Entries[i + 1];
            }
            int j = Entries.Length - 1;
            return SetTwoWayPing(j, Entries[j].Data);
        }
        
        static ushort SetTwoWayPing(int i, ushort prev) {
            Entries[i].Data = (ushort)(prev + 1);
            Entries[i].TimeSent = DateTime.UtcNow;
            return (ushort)(prev + 1);
        }
        
        public static void Update(ushort data) {
            for (int i = 0; i < Entries.Length; i++ ) {
                if (Entries[i].Data != data) continue;
                Entries[i].TimeReceived = DateTime.UtcNow;
                return;
            }
        }
        
        
        /// <summary> Gets best ping in milliseconds, or 0 if no ping measures. </summary>
        public static double BestPingMilliseconds() {
            double totalMs = 100000000;
            foreach (PingEntry ping in Entries) {
                if (ping.TimeSent.Ticks == 0 || ping.TimeReceived.Ticks == 0) continue;
                totalMs = Math.Min(totalMs, ping.Latency);
            }
            return totalMs;
        }
        
        /// <summary> Gets average ping in milliseconds, or 0 if no ping measures. </summary>
        public static double AveragePingMilliseconds() {
            double totalMs = 0;
            int measures = 0;
            foreach (PingEntry ping in Entries) {
                if (ping.TimeSent.Ticks == 0 || ping.TimeReceived.Ticks == 0) continue;
                totalMs += ping.Latency; measures++;
            }
            return measures == 0 ? 0 : (totalMs / measures);
        }
        
        /// <summary> Gets worst ping in milliseconds, or 0 if no ping measures. </summary>
        public static double WorstPingMilliseconds() {
            double totalMs = 0;
            foreach (PingEntry ping in Entries) {
                if (ping.TimeSent.Ticks == 0 || ping.TimeReceived.Ticks == 0) continue;
                totalMs = Math.Max(totalMs, ping.Latency);
            }
            return totalMs;
        }
        
        public static string Format() {
            return String.Format("Lowest ping {0}ms, average {1} ms, highest {2}ms",
                                 (int)BestPingMilliseconds(),
                                 (int)AveragePingMilliseconds(),
                                 (int)WorstPingMilliseconds());
        }
    }
}
