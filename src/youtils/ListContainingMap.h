// Copyright (C) 2016 iNuron NV
//
// This file is part of Open vStorage Open Source Edition (OSE),
// as available from
//
//      http://www.openvstorage.org and
//      http://www.openvstorage.com.
//
// This file is free software; you can redistribute it and/or modify it
// under the terms of the GNU Affero General Public License v3 (GNU AGPLv3)
// as published by the Free Software Foundation, in version 3 as it comes in
// the LICENSE.txt file of the Open vStorage OSE distribution.
// Open vStorage is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY of any kind.

#ifndef LISTCONTAININGMAP_H_
#define LISTCONTAININGMAP_H_

// TODO maybe this needs to live in utilities

#include <map>
#include <algorithm>
#include <stdexcept>
#include <iosfwd>
#include <cassert>
#include "Logging.h"


namespace fungi {

    /**
     * Mix of an std::map and an std::list.
     *
     * By doing the list management manually we avoid
     * searching a linked list when removing.
     *
     * Examples of usage can be found in tests/gtestListContainingMap.cpp
     *
     * Please note that due to the Data being used by value and a sentinel being used
     * in the list, there needs to be a default constructor on the Data type.
     */

    template<typename Key, typename Data, typename Compare = std::less<Key> >
        class ListContainingMap {

    private:

	/**
	 * private Entry type, not exposed externally, use the Iterator type below.
	 */
	class Entry {

	public:
            Entry():next(0),prev(0) {}
            // use default copy constructor
            Data d;
            Key k;
            Entry * next;
            Entry * prev;

	private:
            // Entry();
            Entry &operator=(const Entry &);
	};

	//	template <typename X1, typename Y1, typename Z1>
	//		friend std::ostream& operator<<(std::ostream& , const typename ListContainingMap<X1, Y1, Z1>::Entry &);

    public:

	ListContainingMap() :
            sentinel_(&sentinel_elem_), head_(sentinel_), tail_(sentinel_) {
            sentinel_->next = sentinel_;
            sentinel_->prev = sentinel_;
	}

	/*
	 * Iterator class for walking over the contained list.
	 */
	class Iterator {
            friend class ListContainingMap;

	private:
            Iterator();
            Iterator(ListContainingMap *mymap, Entry *entry) :
                mymap_(mymap), entry_(entry) {
                assert(entry != 0);
            }
	public:
            Data * operator*() const {
                if (entry_ == mymap_->sentinel_) {
                    return 0;
                }
                return &(entry_->d);
            }

            bool operator==(const Iterator &it2) const {
                return it2.entry_ == entry_;
            }

            bool operator!=(const Iterator &it2) const {
                return it2.entry_ != entry_;
            }

            /**
             * returns the key of the corresponsing map location
             * of the current list entry the iterator points to.
             */
            Key key() const
            {
                if (entry_ == mymap_->sentinel_) {
                    throw std::out_of_range("ListContainingMap operator Key out of range");
                }
                return entry_->k;

            }


            Iterator &operator++() {
                if (entry_ == mymap_->sentinel_) {
                    throw std::out_of_range(
                                            "ListContainingMap operator++ out of range");
                }
                entry_ = entry_->next;
                return *this;
            }

            Iterator &operator--() {
                if (entry_ == mymap_->sentinel_) {
                    entry_ = mymap_->tail_;

                }
                else {
                    entry_ = entry_->prev;
                }

                return *this;
            }

	private:
            ListContainingMap *mymap_;
            Entry *entry_;

            //    	template <typename X2, typename Y2, typename Z2>
            //    	    		friend std::ostream& operator<<(std::ostream& , const typename ListContainingMap<X2, Y2, Z2>::Iterator &);
	};

	/**
	 * get entry for a key; if not existing, returns end()
	 * The pointer returned points directly to memory
	 * contained in the std::map, do NOT delete it!
	 */
	Iterator get(Key k) {
            typename map_t_::iterator i = map_.find(k);
            if (i == map_.end()) {
                return Iterator(this, sentinel_);
            }
            Entry &entry = i->second;
            return Iterator(this, &entry);
	}
        const Iterator get(Key k) const{
            typename map_t_::const_iterator i = map_.find(k);
            if (i == map_.end()) {
                return Iterator(this, sentinel_);
            }
            Entry &entry = i->second;
            return Iterator(this, &entry);
        }

        Iterator begin(){
            return Iterator(this, head_);
        }

        Iterator end() {
            return Iterator(this,sentinel_);
        }

	const Iterator end() const {
            return Iterator(this, sentinel_);
	}

        const Iterator begin() const {
            return Iterator(this, head_);
	}

        /**
	 * get entry for a key; if not existing, returns end()
	 * The pointer returned points directly to memory
	 * contained in the std::map, do NOT delete it!
     * Moreover it puts the entry at the end of the dll.
     * This is 0(log n).
	 */

        Iterator getAndPutAtTail(Key k){
            typename map_t_::iterator i = map_.find(k);
            if (i == map_.end()) {
                return Iterator(this, sentinel_);
            }

            Entry &entry = i->second;
            if(&entry == tail_){
                return Iterator(this, &entry);
            }

            if(&entry == head_){
                entry.next->prev = entry.prev;
                head_= entry.next;
                tail_->next = &entry;
                entry.prev = tail_;
                entry.next = sentinel_;
                tail_ = &entry;
                return Iterator(this,&entry);

            }
            else {
                entry.next->prev = entry.prev;
                entry.prev->next = entry.next;
                tail_->next = &entry;
                entry.prev = tail_;
                entry.next = sentinel_;
                tail_ = &entry;
                return Iterator(this,&entry);
            }
        }

	/**
	 * add data entry for a certain key, this creates
	 * an entry for the key and sets the data in it
	 */
	void pushBack(Key k, const Data & d) {
            Entry &entry = map_[k];
            entry.d = d;
            entry.k = k;
            entry.next = sentinel_;
            // due to sentinel no need to check if tail == 0
            tail_->next = &entry;
            entry.prev = tail_;
            tail_ = &entry;
            if (head_ == sentinel_) {
                head_ = &entry;
            }
	}

	/**
	 * remove an entry; this is O(log n) due to the entry
	 * being part of the linkedlist data structure
	 */
	void remove(Iterator &i) {
            Entry *entry = i.entry_;
            assert(entry != 0);

            if (entry == sentinel_) {
                throw std::out_of_range(
					"ListContainingMap remove out of range");
            }
            if (entry == tail_) {
                tail_ = entry->prev;
            }
            if (entry == head_) {
                head_ = entry->next;
            }
            // due to sentinel no need to check for nullpointers here:
            entry->prev->next = entry->next;
            entry->next->prev = entry->prev;
            map_.erase(entry->k);
	}

	int32_t size() const {
            return map_.size();
	}

	bool empty() const {
            return size() == 0;
	}

    private:

	Entry sentinel_elem_;
	Entry * sentinel_;
	Entry *head_;
	Entry *tail_;
	typedef std::map<Key, Entry, Compare> map_t_;
	map_t_ map_;

    };

}

#if 0
This doesn't work currently. This implies that you can't directly compare iterators with EXPECT_EQ in tests...
    use EXPECT_TRUE instead.
    template <typename X1, typename Y1, typename Z1> inline
    std::ostream& operator<<(std::ostream& outp, const typename fungi::ListContainingMap<X1, Y1, Z1>::Entry & /*e*/) {
    //outp << "Entry(" << e.k << ":" << e.d  << ")";
    outp << "Entry()";
    return outp;
}

template <typename X2, typename Y2, typename Z2> inline
    std::ostream& operator<<(std::ostream& outp, const typename fungi::ListContainingMap<X2, Y2, Z2>::Iterator & /*it*/) {
    //outp << "Iterator(" << *(it.entry_) << ")";
    outp << "Iterator()";
    return outp;
}
#endif

#endif
