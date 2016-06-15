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

#ifndef CLUSTERLOCATION_AND_HASH_
#define CLUSTERLOCATION_AND_HASH_

#include "ClusterLocation.h"
#include "VolumeConfig.h"

#include <youtils/Weed.h>

namespace volumedrivertesting
{
class CachedTCBTTest;
}

namespace volumedriver
{

struct ClusterLocationAndHash;
}

std::ostream&
operator<<(std::ostream& ostr,
           const volumedriver::ClusterLocationAndHash& loc);

namespace volumedriver
{

struct ClusterLocationAndHash
{
    ClusterLocationAndHash(const ClusterLocation& cloc,
                           const uint8_t* data
#ifndef ENABLE_MD5_HASH
                           __attribute__((unused))
#endif
                           ,
                           const size_t size
#ifndef ENABLE_MD5_HASH
                           __attribute__((unused))
#endif
                           )
        : clusterLocation(cloc)
#ifdef ENABLE_MD5_HASH
        , weed_(data,
                size)
#endif
    {}

    ClusterLocationAndHash(const ClusterLocation& cloc,
                           const youtils::Weed& wd
#ifndef ENABLE_MD5_HASH
                           __attribute__((unused))
#endif
                           )
        : clusterLocation(cloc)
#ifdef ENABLE_MD5_HASH
        , weed_(wd)
#endif
    {}

#ifndef ENABLE_MD5_HASH
    ClusterLocationAndHash(const ClusterLocation& cloc)
        : clusterLocation(cloc)
    {}
#endif

    operator const ClusterLocation& () const
    {
        return clusterLocation;
    }

    operator ClusterLocation& ()
    {
        return clusterLocation;
    }

    ClusterLocationAndHash() = default;

    bool
    operator==(const ClusterLocationAndHash& other) const
    {
#ifdef ENABLE_MD5_HASH
        return clusterLocation == other.clusterLocation
            and weed_ == other.weed_;
#else
        return clusterLocation == other.clusterLocation;
#endif
    }

    bool
    operator!=(const ClusterLocationAndHash& other) const
    {
        return not (*this == other);
    }

    void
    cloneID(SCOCloneID scid)
    {
        clusterLocation.cloneID(scid);
    }

    static bool
    use_hash()
    {
#ifdef ENABLE_MD5_HASH
        return true;
#else
        return false;
#endif
    }

    static const ClusterLocationAndHash&
    discarded_location_and_hash()
    {
        static const std::vector<byte> z(VolumeConfig::default_cluster_size(), 0);
        static const ClusterLocationAndHash zloc(ClusterLocation(0),
                                                 youtils::Weed(z));
        return zloc;
    }

    const youtils::Weed&
    weed() const
    {
#ifdef ENABLE_MD5_HASH
        return weed_;
#else
       static const youtils::Weed w(nullptr, 0);
       return w;
#endif
    }

    // TODO: constify
    ClusterLocation clusterLocation;
#ifdef ENABLE_MD5_HASH
    youtils::Weed weed_;
#endif

private:
    friend class CachedTCBTTest;
};

#ifdef ENABLE_MD5_HASH
static_assert(sizeof(ClusterLocationAndHash) == sizeof(ClusterLocation) + sizeof(youtils::Weed),
              "ClusterLocation size assumption does not hold");
#else
static_assert(sizeof(ClusterLocationAndHash) == sizeof(ClusterLocation),
              "ClusterLocation size assumption does not hold");
#endif

}

#endif // CLUSTERLOCATION_AND_HASH_

// Local Variables: **
// mode: c++ **
// End: **
