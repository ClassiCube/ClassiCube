// Copyright 2009-2012 Matvei Stefarov <me@matvei.org>
using System.Threading;

namespace ClassicalSharp.Util {
	
    /// <summary> A lightweight queue implementation with lock-free/concurrent operations. </summary>
    /// <typeparam name="T"> Payload type </typeparam>
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
        public int Length;

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
            Pointer tempHead;

            // Keep trying until deque is done
            bool bDequeNotDone = true;
            while( bDequeNotDone ) {
                // read head
                tempHead = head;

                // read tail
                Pointer tempTail = tail;

                // read next
                Pointer next = tempHead.Ptr.Next;

                // Are head, tail, and next consistent?
                if( tempHead.Count != head.Count || tempHead.Ptr != head.Ptr ) continue;

                // is tail falling behind
                if( tempHead.Ptr == tempTail.Ptr ) {
                    // is the queue empty?
                    if( null == next.Ptr ) {
                        // queue is empty cannnot dequeue
                        return false;
                    }

                    // Tail is falling behind. try to advance it
                    CompareAndSwap( ref tail, tempTail, new Pointer( next.Ptr, tempTail.Count + 1 ) );

                } else { // No need to deal with tail
                    // read value before CAS otherwise another deque might try to free the next node
                    t = next.Ptr.Value;

                    // try to swing the head to the next node
                    if( CompareAndSwap( ref head, tempHead, new Pointer( next.Ptr, tempHead.Count + 1 ) ) ) {
                        bDequeNotDone = false;
                    }
                }
            }
            Interlocked.Decrement( ref Length );
            // dispose of head.ptr
            return true;
        }


        public void Enqueue( T t ) {
            // Allocate a new node from the free list
            Node node = new Node { Value = t };

            // copy enqueued value into node

            // keep trying until Enqueue is done
            bool bEnqueueNotDone = true;

            while( bEnqueueNotDone ) {
                // read Tail.ptr and Tail.count together
                Pointer tempTail = tail;

                // read next ptr and next count together
                Pointer next = tempTail.Ptr.Next;

                // are tail and next consistent
                if( tempTail.Count == tail.Count && tempTail.Ptr == tail.Ptr ) {
                    // was tail pointing to the last node?
                    if( null == next.Ptr ) {
                        if( CompareAndSwap( ref tempTail.Ptr.Next, next, new Pointer( node, next.Count + 1 ) ) ) {
                            bEnqueueNotDone = false;
                        } // endif

                    } // endif
                    else // tail was not pointing to last node
                    {
                        // try to swing Tail to the next node
                        CompareAndSwap( ref tail, tempTail, new Pointer( next.Ptr, tempTail.Count + 1 ) );
                    }

                } // endif

            } // endloop
            Interlocked.Increment( ref Length );
        }


        public void Clear() {
            T t = default( T );
            while( Dequeue( ref t ) ) { }
        }
    }
}