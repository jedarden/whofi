using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Panopticon
{
    /// <summary>
    /// This class contains a dictionary where elements are sorted under the insertion and update order in FIFO fashion
    /// </summary>
    /// <typeparam name="K"></typeparam>
    /// <typeparam name="V"></typeparam>
    class ConcurrentDictionaryStack<K, V>:IDisposable where V:class
    {
        class LinkedNode<T>
        {
            internal LinkedNode<T> next=null;
            internal LinkedNode<T> prec=null;//if null, is first
            internal T value;
        }
        LinkedNode<V> first = null;
        LinkedNode<V> last = null;
        ConcurrentDictionary<K, LinkedNode<V>> mapper = new ConcurrentDictionary<K, LinkedNode<V>>();
        ReaderWriterLockSlim locker = new ReaderWriterLockSlim();
        private void shiftPositionInternal(LinkedNode<V> item)
        {
            if (first == null) // if first not set, set item as first
                first = item;
            else if (first == item && item.next != null) // if this is first, and there is one after, set the one after as first
                first = item.next;
            if (item.prec == null && item.next == null && last!=item) // a new element. Put last
            {
                item.prec = last;
                if (last != null)
                    last.next = item;
                last = item;
            }
            else if (last == item) // this is already last. Do nothing
            { } 
            else //this was already in the chain, shift
            {
                if (item.prec != null)
                    item.prec.next = item.next;
                if (item.next != null)
                    item.next.prec = item.prec;
                item.next = null;
                item.prec = last;
                if (last != null)
                    last.next = item;
                last = item;
            }
        }
        /// <summary>
        /// Insert or update the value for the given key
        /// </summary>
        /// <param name="k">Key</param>
        /// <param name="v">Value</param>
        /// <param name="updater">Function to call to update the value if it already exists in the map. It is call as updater(existingValue, givenValue)</param>
        /// <returns>The final value inserted or updated</returns>
        public V upsert(K k, V v, Func<V,V,V> updater)
        {
            LinkedNode<V> item;
            locker.EnterWriteLock();
            if (!mapper.TryGetValue(k, out item))
            {
                item = new LinkedNode<V>();
                item.value = v;
                item.next = null;
                item.prec = null;
                item = mapper.GetOrAdd(k, item);
            }
            else
               item.value=updater(item.value, v);
            shiftPositionInternal(item);            
            locker.ExitWriteLock();
            return item.value;
        }
        /// <summary>
        /// Insert or update the value for a given key, remove if from the structure if it respects some condition
        /// </summary>
        /// <param name="k">Key</param>
        /// <param name="v">Value</param>
        /// <param name="updater">Function to call to update the value if it already exists in the map. It is call as updater(existingValue, givenValue)</param>
        /// <param name="condition">Function that, given the value, returns if must be removed or not</param>
        /// <param name="updateditem">The item, updated</param>
        /// <returns>True if the value has been removed from the data structure, false otherwise</returns>
        public bool upsertAndConditionallyRemove(K k, V v, Func<V, V, V> updater, Func<V, Boolean> condition, out V updateditem)
        {
            LinkedNode<V> item;
            bool retval;
            locker.EnterWriteLock();
            if (!mapper.TryGetValue(k, out item))
            {
                item = new LinkedNode<V>();
                item.value = v;
                item.next = null;
                item.prec = null;
                item = mapper.GetOrAdd(k, item);
            }
            else
                item.value = updater(item.value, v);
            retval = condition(item.value);
            if (retval)
                removeInternal(k);
            else
                shiftPositionInternal(item);
            locker.ExitWriteLock();
            updateditem = item.value;
            return retval;
        }
        public bool getKey(K k, out V v)
        {
            LinkedNode<V> n;
            bool retval=mapper.TryGetValue(k,out n);
            if(retval)
                v=n.value;
            else
                v=null;
            return retval;
        }
        /// <summary>
        /// Return the value at the tail of the queue (the oldest inserted or updated) without removing it
        /// </summary>
        /// <returns></returns>
        public V peek()
        {
            V retval;
            locker.EnterReadLock();
            if (first != null)
                retval = first.value;
            else
                retval = null;
            locker.ExitReadLock();
            return retval;
        }
        private V removeInternal(K k)
        {
            LinkedNode<V> n;
            V retval = null;
            if (mapper.TryGetValue(k, out n))
            {
                mapper.TryRemove(k, out n);
                retval = n.value;
                if(n.prec!=null||n.next!=null)
                {
                    if (n.prec == null)
                        first = n.next;
                    else
                        n.prec.next = n.next;
                    if (n.next == null)
                        last = n.prec;
                    else
                        n.next.prec = n.prec;
                }
                else if(first==n&&last==n)
                {
                    first = null;
                    last = null;
                }                
            }
            return retval;
        }
        public V remove(K k)
        {
            locker.EnterWriteLock();
            V retval = removeInternal(k);            
            locker.ExitWriteLock();
            return retval;
        }
        /// <summary>
        /// Return the value at the tail of the queue (the oldest inserted or updated) and remove it
        /// </summary>
        /// <param name="keyExtractor">Function that, given the value stored in the structure, returns the key under which is saved</param>
        /// <returns></returns>
        public V pop(Func<V, K> keyExtractor)
        {
            locker.EnterWriteLock();
            LinkedNode<V> n = first;
            if (n == null)
                return null;
            V retval = removeInternal(keyExtractor(n.value));
            locker.ExitWriteLock();
            return retval;
        }
        /// <summary>
        /// Peeks the value at the tail of the queue (the oldest inserted or updated) and removes it if a function applied to the value returns a true condition.
        /// </summary>
        /// <param name="condition">Function that, given the value, returns if must be removed or not</param>
        /// <param name="keyExtractor">Function that, given the value stored in the structure, returns the key under which is saved</param>
        /// <param name="peekedValue">Value peeked</param>
        /// <returns>If the item has been removed</returns>
        public bool popConditional(Func<V,Boolean> condition, Func<V,K> keyExtractor, out V peekedValue)
        {
            bool removed=false;
            locker.EnterWriteLock();
            if (first != null)
            {
                peekedValue = first.value;
                if (condition(peekedValue))
                {
                    removed = removeInternal(keyExtractor(peekedValue)) != null ? true : false;
                }
            }
            else
                peekedValue = null;
            locker.ExitWriteLock();
            return removed;
        }
        public ICollection<V> getAll()
        {
            List<V> allElements = new List<V>();
            foreach (LinkedNode<V> val in mapper.Values)
                allElements.Add(val.value);
            return allElements;
        }

        public void Dispose()
        {
            locker.Dispose();
        }
    }
}
