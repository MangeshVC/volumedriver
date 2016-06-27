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

#ifndef FAILOVERCACHESTREAMERS_H
#define FAILOVERCACHESTREAMERS_H

#include "distributed-transaction-log/fungilib/IOBaseStream.h"
#include <youtils/IOException.h>
#include "Types.h"
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_array.hpp>
#include "ClusterLocation.h"

namespace volumedriver
{

enum FailOverCacheCommand
{
    RemoveUpTo     =  0x1,
    GetSCO         =  0x2,
    Register       =  0x3,
    AddEntries     =  0x4,
    GetEntries     =  0x5,
    Flush          =  0x6,
    Ok             =  0x7,
    NotOk          =  0x8,
    Unregister     =  0x9,
    Clear          =  0xA,
    GetSCORange    =  0xB
    //    Bye            =  0xff
};

#define OUT_ENUM(stream, val)                   \
    {                                           \
        const uint32_t com = val;               \
        stream << com;                          \
    }                                           \


template<FailOverCacheCommand val>
struct CommandData;

template<>
struct CommandData<Register>
{
    CommandData(const std::string& ns = "",
                volumedriver::ClusterSize clustersize = ClusterSize(0))
        : ns_(ns)
        , clustersize_(clustersize)
    {}

    std::string ns_;
    ClusterSize clustersize_;
};

fungi::IOBaseStream&
checkStreamOK(fungi::IOBaseStream& stream,
              const std::string& ex);


fungi::IOBaseStream&
operator<<(fungi::IOBaseStream& stream, const CommandData<Register>& data);


fungi::IOBaseStream&
operator>>(fungi::IOBaseStream& stream, CommandData<Register>& data);

struct FailOverCacheEntry
{

    FailOverCacheEntry(ClusterLocation cli,
                       uint64_t lba,
                       const uint8_t* buffer,
                       uint32_t size);

    ClusterLocation cli_;
    uint64_t lba_;
    uint32_t size_;
    const uint8_t* buffer_;
};


template<>
struct CommandData<AddEntries>

{
    using EntryVector = std::vector<FailOverCacheEntry>;

    // Only used when streaming in
    CommandData(EntryVector entries)
        : entries_(std::move(entries))
    {}

    // Only used when streaming in
    std::unique_ptr<uint8_t[]> buf_;

    // Only used when streaming out
    CommandData() = default;

    EntryVector entries_;
};

fungi::IOBaseStream&
operator<<(fungi::IOBaseStream& stream, const CommandData<AddEntries>& data);


fungi::IOBaseStream&
operator>>(fungi::IOBaseStream& stream, CommandData<AddEntries>& data);


template<>
struct CommandData<Flush>
{};

fungi::IOBaseStream&
operator<<(fungi::IOBaseStream& stream, const CommandData<Flush>& /*data*/);

fungi::IOBaseStream&
operator>>(fungi::IOBaseStream& stream, CommandData<Flush>& /*data*/);

template<>
struct CommandData<Clear>
{};

fungi::IOBaseStream&
operator<<(fungi::IOBaseStream& stream, const CommandData<Clear>& /*data*/);


}

#endif // FAILOVERCACHESTREAMERS_H

// Local Variables: **
// mode: c++ **
// End: **
