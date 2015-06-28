// Copyright 2009-2012 Matvei Stefarov <me@matvei.org>
using System.Threading;

namespace PluginBeta173 {

	public sealed class ConcurrentQueue<T> {
		sealed class Node {
			public T Value;
			public Pointer Next;
		}

		struct Pointer {
			public long Count;
			public Node Ptr;

			public Pointer( Node node, long c ) {
				Ptr = node;
				Count = c;
			}
		}
		Pointer head, tail;

		public ConcurrentQueue() {
			Node node = new Node();
			head.Ptr = tail.Ptr = node;
		}

		static bool CompareAndSwap( ref Pointer destination, Pointer compared, Pointer exchange ) {
			if( compared.Ptr == Interlocked.CompareExchange( ref destination.Ptr, exchange.Ptr, compared.Ptr ) ) {
				Interlocked.Exchange( ref destination.Count, exchange.Count );
				return true;
			}
			return false;
		}

		public bool Dequeue( ref T t ) {			
			while( true ) {
				Pointer tempHead = head;
				Pointer tempTail = tail;
				Pointer next = tempHead.Ptr.Next;

				// Are head, tail, and next consistent?
				if( tempHead.Count != head.Count || tempHead.Ptr != head.Ptr ) continue;

				// is tail falling behind
				if( tempHead.Ptr == tempTail.Ptr ) {
					// is the queue empty?
					if( next.Ptr == null ) {				
						return false; // queue is empty cannnot dequeue
					}
					// Tail is falling behind. try to advance it
					CompareAndSwap( ref tail, tempTail, new Pointer( next.Ptr, tempTail.Count + 1 ) );
				} else { // No need to deal with tail
					// read value before CAS otherwise another deque might try to free the next node
					t = next.Ptr.Value;

					// try to swing the head to the next node
					if( CompareAndSwap( ref head, tempHead, new Pointer( next.Ptr, tempHead.Count + 1 ) ) ) {
						return true;
					}
				}
			}
		}

		public void Enqueue( T t ) {
			Node node = new Node();
			node.Value = t;

			while( true ) {
				Pointer tempTail = tail;
				Pointer next = tempTail.Ptr.Next;

				// are tail and next consistent
				if( tempTail.Count == tail.Count && tempTail.Ptr == tail.Ptr ) {
					if( null == next.Ptr ) { // was tail pointing to the last node?
						if( CompareAndSwap( ref tempTail.Ptr.Next, next, new Pointer( node, next.Count + 1 ) ) ) {
							return;
						}
					} else { // tail was not pointing to last node
						// try to swing Tail to the next node
						CompareAndSwap( ref tail, tempTail, new Pointer( next.Ptr, tempTail.Count + 1 ) );
					}
				}
			}
		}
	}
}