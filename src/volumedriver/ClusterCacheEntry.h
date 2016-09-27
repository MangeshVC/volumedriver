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

#ifndef CLUSTER_CACHE_ENTRY_H_
#define CLUSTER_CACHE_ENTRY_H_

#include "ClusterCacheKey.h"
#include "ClusterCacheMode.h"

#include <type_traits>

#include <boost/intrusive/link_mode.hpp>

#include <youtils/Assert.h>
#include <youtils/Md5.h>

namespace volumedriver
{

class ClusterCacheEntry
{
public:
    using key_t = ClusterCacheKey;

    explicit ClusterCacheEntry(const youtils::Weed& w)
        : key(w)
        , dprevious_(0)
        , dnext_(nullptr)
        , snext_(nullptr)
    {
        set_mode(ClusterCacheMode::ContentBased);
    };

    ClusterCacheEntry(ClusterCacheHandle tag,
                      ClusterAddress ca)
        : key(tag,
              ca)
        , dprevious_(0)
        , dnext_(nullptr)
        , snext_(nullptr)
    {
        set_mode(ClusterCacheMode::LocationBased);
    }

    ClusterCacheEntry(const ClusterCacheKey& k,
                      ClusterCacheMode m)
        : ClusterCacheEntry(reinterpret_cast<const youtils::Weed&>(k))
    {
        set_mode(m);
    }

    ClusterCacheEntry()
        : ClusterCacheEntry(youtils::Weed())
    {}

    ClusterCacheEntry&
    operator=(const ClusterCacheEntry&)
    {
        ASSERT(0=="Programming error");
        return *reinterpret_cast<ClusterCacheEntry*>(0);
    }

    ClusterCacheEntry*
    snext() const
    {
        return snext_;
    }

    void
    snext(ClusterCacheEntry* next)
    {
        snext_ = next;
    }

    ClusterCacheEntry*
    dprevious() const
    {
        return reinterpret_cast<ClusterCacheEntry*>(dprevious_ bitand ptr_mask);
    }

    void
    dprevious(ClusterCacheEntry* previous)
    {
        ASSERT((reinterpret_cast<uintptr_t>(previous) bitand priv_mask) == 0);
        dprevious_ = (reinterpret_cast<uint64_t>(previous) bitand ptr_mask) bitor
                     (dprevious_ bitand priv_mask);
    }

    ClusterCacheEntry*
    dnext() const
    {
        return dnext_;
    }

    void
    dnext(ClusterCacheEntry* next)
    {
        dnext_ = next;
    }

    ClusterCacheMode
    mode() const
    {
        return ClusterCacheMode(dprevious_ bitand priv_mask);
    }

    friend bool
    operator>(const ClusterCacheEntry& first,
              const ClusterCacheEntry& second)
    {
        return first.key > second.key;
    }

    static const uint64_t align_bits = 3;
    static const uint64_t priv_mask = (1ULL << align_bits) - 1;
    static const uint64_t ptr_mask = ~priv_mask;

    const ClusterCacheKey key;

private:
    DECLARE_LOGGER("ClusterCacheEntry");

    uint64_t dprevious_;
    ClusterCacheEntry* dnext_;
    ClusterCacheEntry* snext_;

    void
    set_mode(const ClusterCacheMode& mode)
    {
        switch (mode)
        {
        case ClusterCacheMode::ContentBased:
            dprevious_ = (dprevious_ bitand ~(1 << 1)) bitor 1;
            break;
        case ClusterCacheMode::LocationBased:
            dprevious_ = (dprevious_ bitand ~1) bitor (1 << 1);
            break;
        }
    }
};

static_assert(sizeof(ClusterCacheEntry) ==
              sizeof(youtils::Weed) + 3 * sizeof(uintptr_t),
              "ClusterCacheEntry size assumption violated");

static_assert(std::alignment_of<ClusterCacheEntry>::value ==
              (1 << ClusterCacheEntry::align_bits),
              "ClusterCacheEntry alignment assumption violated");

}

#endif // !CLUSTER_CACHE_ENTRY_H_

// Local Variables: **
// End: **
