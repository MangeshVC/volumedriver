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

#ifndef SETUP_HELPER_H_
#define SETUP_HELPER_H_
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>
#include <youtils/BooleanEnum.h>
#include <youtils/IOException.h>
#include <youtils/Logging.h>

VD_BOOLEAN_ENUM(UseClusterCache);
VD_BOOLEAN_ENUM(CleanupLocalBackend);

namespace volumedriver
{

class SetupHelper
{
public:
    SetupHelper(const boost::filesystem::path& json,
                const boost::filesystem::path& default_dir = boost::filesystem::path(),
                const UseClusterCache = UseClusterCache::T,
                const CleanupLocalBackend = CleanupLocalBackend::T);

    void
    setup();

    void
    start_volmanager();

    void
    dump_json(const boost::filesystem::path&);

    MAKE_EXCEPTION(SetupException, fungi::IOException);
    MAKE_EXCEPTION(SetupExceptionCouldNotCreateDirectory, SetupException);
    MAKE_EXCEPTION(SetupExceptionNotADirectory, SetupException);
    MAKE_EXCEPTION(SetupHelperExceptionConfigPtreeEmpty, SetupException);

    DECLARE_LOGGER("SetupHelper")
private:
    const boost::filesystem::path json_config_file_;
    const boost::filesystem::path default_directory_;
    boost::property_tree::ptree config_ptree_;


    const unsigned open_scos_per_volume_ = 32;
    const unsigned dstore_throttle_usecs_ = 4000;
    const unsigned foc_throttle_usecs_ = 10000;
    const uint32_t num_scos_in_tlog = 20;
    const uint32_t num_threads = 4;
    const UseClusterCache use_read_cache_;
    const CleanupLocalBackend cleanup_local_backend_;

    void
    setup_scocache_mountpoints(const boost::property_tree::ptree& ptree);


    void
    setup_backend(const boost::property_tree::ptree& ptree);

    void
    setup_readcache_serialization(const boost::property_tree::ptree& ptree);

    void
    setup_volmanager_parameters(const boost::property_tree::ptree& ptree);

    void
    setup_failovercache(const boost::property_tree::ptree& ptree);

    void
    setup_threadpool(const boost::property_tree::ptree& ptree);

};


}

#endif // SETUP_HELPER_H_
