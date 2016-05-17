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

#ifndef _VOLUMEDRIVERFS_CLONEFILE_FLAGS_H
#define _VOLUMEDRIVERFS_CLONEFILE_FLAGS_H

namespace volumedriverfs
{

enum class CloneFileFlags
{
    Guarded = (1 << 0),
    Lazy = (1 << 1),
    Valid = (1 << 2),
    SkipZeroes = (1 << 4)
};

inline bool
operator& (CloneFileFlags a, CloneFileFlags b)
{
    return static_cast<bool>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

inline CloneFileFlags
operator| (CloneFileFlags a, CloneFileFlags b)
{
    return static_cast<CloneFileFlags>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

} //namespace volumedriverfs


#endif //_VOLUMEDRIVERFS_CLONEFILE_FLAGS_H
