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

#ifndef LRU_CACHE_TOO_H_
#define LRU_CACHE_TOO_H_

#include "Assert.h"
#include "Logging.h"

#include <boost/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/optional.hpp>

namespace youtils
{

// This LRU cache contains objects whereas the other one (LRUCache.h) references
// to objects (e.g. files on disk) which require special steps on eviction
// (remove the file).
// Better naming suggestions / ideas for unification welcome.
template<typename K,
         typename V,
         template<typename...> class Set = boost::bimaps::unordered_set_of>
class LRUCacheToo
{
public:
    LRUCacheToo(const std::string& name,
                size_t capacity)
        : capacity_(capacity)
        , name_(name)
    {
        if (capacity_ == 0)
        {
            throw std::logic_error("LRUCache capacity should be > 0");
        }
    }

    ~LRUCacheToo() = default;

    LRUCacheToo(const LRUCacheToo&) = delete;

    LRUCacheToo&
    operator=(const LRUCacheToo&) = delete;

    boost::optional<V>
    find(const K& k)
    {
        auto it = bimap_.left.find(k);
        if (it != bimap_.left.end())
        {
            bimap_.right.relocate(bimap_.right.end(),
                                  bimap_.project_right(it));
            return it->second;
        }
        else
        {
            return boost::none;
        }
    }

    void
    insert(const K& k,
           const V& v)
    {
        if (bimap_.size() >= capacity_)
        {
            bimap_.right.erase(bimap_.right.begin());
        }

        auto r(bimap_.insert(typename Bimap::value_type(k, v)));
        if (not r.second)
        {
            bimap_.left.erase(k);
            r = bimap_.insert(typename Bimap::value_type(k, v));
            VERIFY(r.second);
        }
    }

    bool
    erase(const K& k)
    {
        return bimap_.left.erase(k);
    }

    std::vector<K>
    keys() const
    {
        std::vector<K> keys;
        keys.reserve(bimap_.size());

        for (const auto& v : bimap_.right)
        {
            keys.push_back(v.second);
        }

        return keys;
    }

    void
    clear()
    {
        bimap_.clear();
    }

    bool
    empty() const
    {
        return bimap_.empty();
    }

    size_t
    size() const
    {
        return bimap_.size();
    }

    size_t
    capacity() const
    {
        return capacity_;
    }

private:
    DECLARE_LOGGER("LRUCacheToo");

    using Bimap = boost::bimaps::bimap<Set<K>,
                                       boost::bimaps::list_of<V>>;
    Bimap bimap_;
    const size_t capacity_;
    const std::string name_;
};

}

#endif // !LRU_CACHE_TOO_
