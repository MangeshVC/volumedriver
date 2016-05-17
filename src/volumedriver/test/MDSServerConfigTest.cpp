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

#include "../metadata-server/RocksConfig.h"
#include "../metadata-server/ServerConfig.h"
#include "../metadata-server/Parameters.h"

#include <boost/filesystem.hpp>

#include <youtils/InitializedParam.h>
#include <youtils/TestBase.h>

namespace volumedrivertest
{

namespace bpt = boost::property_tree;
namespace fs = boost::filesystem;
namespace ip = initialized_params;
namespace mds = metadata_server;

using namespace volumedriver;

class MDSServerConfigTest
    : public youtilstest::TestBase
{};

TEST_F(MDSServerConfigTest, roundtrip)
{
    std::vector<mds::RocksConfig> rocks_configs;
    std::vector<MDSNodeConfig> node_configs;
    std::vector<mds::ServerConfig> server_configs;
    mds::RocksConfig rocks_config;

    const std::string host("localhost");

    const size_t count = 3;
    const fs::path db_pfx = "/tmp/db";
    const fs::path scratch_pfx = "/tmp/scratch";

    for (size_t i = 1; i <= count; ++i)
    {
        mds::RocksConfig rocks_config;
        rocks_config.db_threads = mds::DbThreads(i);
        rocks_config.write_cache_size = mds::WriteCacheSize(i << 20);
        rocks_config.read_cache_size = mds::ReadCacheSize(i << 20);
        rocks_config.enable_wal = mds::EnableWal::T;
        rocks_config.data_sync = mds::DataSync::T;

        rocks_configs.emplace_back(std::move(rocks_config));

        node_configs.emplace_back(MDSNodeConfig(host,
                                                i));

        const std::string sfx(boost::lexical_cast<std::string>(i));

        server_configs.emplace_back(mds::ServerConfig(node_configs.back(),
                                                      db_pfx / sfx,
                                                      scratch_pfx / sfx,
                                                      rocks_configs.back()));

    }

    bpt::ptree pt;
    ip::PARAMETER_TYPE(mds_nodes)(server_configs).persist(pt);

    ip::PARAMETER_TYPE(mds_nodes) server_configs_in(pt);

    ASSERT_EQ(count, server_configs_in.value().size());

    for (size_t i = 0; i < count; ++i)
    {
        EXPECT_EQ(server_configs[i],
                  server_configs_in.value()[i]);
    }
}

}
