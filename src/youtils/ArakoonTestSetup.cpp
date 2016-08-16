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

#include "ArakoonTestSetup.h"
#include "Assert.h"

#include <unistd.h>
#include <fstream>

#include <boost/lexical_cast.hpp>
#include <boost/scope_exit.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <gtest/gtest.h>

namespace ba = boost::asio;
namespace fs = boost::filesystem;

namespace arakoon
{

uint16_t
ArakoonTestSetup::port_base_ = 61000;

fs::path
ArakoonTestSetup::binary_path_ = "";

std::string
ArakoonTestSetup::server_cfg_ = "arakoonserver.cfg";

std::vector<boost::filesystem::path>
ArakoonTestSetup::plugins_;

fs::path
ArakoonTestSetup::node_home_dir_(unsigned i) const
{
    return dir_ / boost::lexical_cast<std::string>(i);
}

ArakoonTestSetup::ArakoonTestSetup(const fs::path& basedir,
                                   const uint16_t num_nodes,
                                   const std::string& node_name,
                                   const std::string& cluster_name,
                                   const std::string& log_level)
    : host_name_("127.0.0.1")
    , cluster_name_(cluster_name)
    , node_name_(node_name)
    , pid_(-1)
    , dir_(basedir)
    , log_level_(log_level)
    , num_nodes_(num_nodes)
{
    VERIFY(log_level_ == "fatal" or
           log_level_ == "error" or
           log_level_ == "warning" or
           log_level_ == "notice" or
           log_level_ == "info" or
           log_level_ == "debug");
}

std::list<ArakoonNodeConfig>
ArakoonTestSetup::node_configs() const
{
    std::list<ArakoonNodeConfig> nodes;

    if(not binary_path_.empty())
    {
        for(unsigned i = 0; i < num_nodes_; ++i)
        {
            nodes.emplace_back(nodeID(i),
                               host_name_,
                               client_port(i));
        }
    }

    return nodes;
}

NodeID
ArakoonTestSetup::nodeID(const uint16_t node_index) const
{
    std::stringstream ss;
    ss << node_name_ << "_" << node_index;
    return NodeID(ss.str());
}

ClusterID
ArakoonTestSetup::clusterID() const
{
    if(not binary_path_.empty())
    {
        return ClusterID(cluster_name_);
    }
    else
    {
        return ClusterID("");
    }
}

uint16_t
ArakoonTestSetup::num_nodes() const
{
    return num_nodes_;
}

void
ArakoonTestSetup::symlink_plugins_() const
{
    for (const auto& p : plugins_)
    {
        for (unsigned i = 0; i < num_nodes_; ++i)
        {
            VERIFY(not p.empty());
            const fs::path dst(node_home_dir_(i) / p.filename());

            LOG_INFO("Creating symlink " << dst << " -> " << p);
            fs::create_symlink(p, dst);
        }
    }
}

void
ArakoonTestSetup::setUpArakoon()
{
    if (useArakoon())
    {
        LOG_DEBUG("Checking arakoons not running");
        for(uint16_t i = 0; i < num_nodes_; ++i)
        {
            checkArakoonNotRunning(i);
        }

        fs::remove_all(dir_);
        fs::create_directories(dir_);

        for (uint16_t i = 0; i < num_nodes_; ++i)
        {
            fs::create_directories(node_home_dir_(i));
        }

        {
            LOG_DEBUG("Creating config file");
            fs::ofstream ofs(server_config_file());
            write_config(ofs);
        }

        symlink_plugins_();

        start_nodes();
    }
}

void
ArakoonTestSetup::start_nodes()
{
    ASSERT_TRUE(useArakoon());

    LOG_DEBUG("Starting arakoon nodes");
    for (uint16_t i = 0; i < num_nodes_; ++i)
    {
        LOG_DEBUG("Starting arakoon node " << i);
        redi::pstreams::argv_type arguments;
        arguments.push_back("arakoon-from-vdtest");
        arguments.push_back("-config");
        arguments.push_back(server_config_file().string());
        arguments.push_back("--node");
        arguments.push_back(nodeID(i));
        redi::ipstream* stream = new redi::ipstream(binary_path_.string(), arguments);
        VERIFY(stream);
        nodes[nodeID(i)] = stream;
        LOG_INFO("Started arakoon " << stream->command());
        // Check that the stream has not exited which is possible if arakoon was
        // still running
        VERIFY(not stream->rdbuf()->exited());
    }

    waitForArakoon_();
}

void
ArakoonTestSetup::shootDownNode(const std::string& node,
                                int signal)
{
    node_map_iterator it = nodes.find(node);
    if(it == nodes.end())
    {
        LOG_ERROR("No such node " << node);
        return;
    }

    redi::ipstream* stream = it->second;
    VERIFY(stream);
    VERIFY(stream->is_open());
    stream->rdbuf()->kill(signal);
    boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    unsigned i = 1;

    while(not stream->rdbuf()->exited())
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(200));
        if(i%5 == 0)
        {
            LOG_FATAL("Could not cleanly stop the arakoon node after " << (i/5) << " seconds");
        }
    }
    delete stream;
    nodes.erase(it);
}

void
ArakoonTestSetup::stop_nodes()
{
    for (auto val : nodes)
    {
        redi::ipstream* stream = val.second;
        VERIFY(stream);
        VERIFY(stream->is_open());
        stream->rdbuf()->kill(SIGTERM);
        boost::this_thread::sleep(boost::posix_time::milliseconds(200));

        if(not stream->rdbuf()->exited())
        {
            LOG_FATAL("Could not cleanly stop the arakoon node");
        }

        delete stream;
    }

    nodes.clear();
}

void
ArakoonTestSetup::tearDownArakoon(bool cleanup)
{
    stop_nodes();

    if(cleanup)
    {
        fs::remove_all(dir_);
    }
}

namespace
{

std::string
plugin_name_from_path(const fs::path& p)
{
    return p.filename().stem().string();
}

}

void
ArakoonTestSetup::write_config(std::ostream& os)
{
    os << "[global]" << std::endl;
    os << "cluster_id = " << cluster_name_ << std::endl;
    // here we have to add all the nodes
    os << "cluster =";

    for(int i = 0; i < num_nodes_; ++i)
    {
        os << (i ?  ", " : " ") << nodeID(i);
    }

    os << std::endl;

    if (not plugins_.empty())
    {
        os << "plugins =";

        for (size_t i = 0; i < plugins_.size(); ++i)
        {
            os << (i ? ", " : " ") << plugin_name_from_path(plugins_[i]);
        }

        os << std::endl;
    }

    os << "master = " << nodeID(0) << std::endl;
    os << "preferred_master = true" << std::endl;

    for(uint16_t i = 0; i < num_nodes_; ++i)
    {
        os << "[" << nodeID(i) << "]" << std::endl;
        os << "ip = " << host_name() << std::endl;
        os << "client_port = " << client_port(i) << std::endl;
        os << "log_level = " << log_level_ << std::endl;
        os << "messaging_port = " << message_port(i) << std::endl;
        os << "log_dir = " << node_home_dir_(i).string() << std::endl;
        os << "home = " << node_home_dir_(i).string() << std::endl;
        os << "disable_tlog_compression = true" << std::endl;
    }
}

void
ArakoonTestSetup::checkArakoonNotRunning(const uint16_t i) const
{
    LOG_DEBUG("checking arakoon not running for node " << i);
    ba::io_service io_service;
    ba::ip::tcp::socket socket(io_service);
    ba::ip::tcp::endpoint endpoint(ba::ip::address::from_string("127.0.0.1"),
                                   client_port(i));
    boost::system::error_code ec;
    socket.connect(endpoint, ec);
    ASSERT_EQ (ec.value(), ECONNREFUSED) << "Arakoon already running on port " << client_port(i) << "?";
}

void
ArakoonTestSetup::waitForArakoon_() const
{
    const Cluster::MilliSeconds timeout(60000);
    ASSERT_NO_THROW(make_client(timeout)) << "failed to connect to arakoon";
}

fs::path
ArakoonTestSetup::server_config_file() const
{
    return fs::path(dir_ / server_cfg_);
}

bool
ArakoonTestSetup::useArakoon()
{
    return binary_path_ != "";
}

void
ArakoonTestSetup::setArakoonBinaryPath(const fs::path& p)
{
    binary_path_ = p;
}

void
ArakoonTestSetup::setArakoonBasePort(const uint16_t base_port)
{
    port_base_ = base_port;
}

void
ArakoonTestSetup::setArakoonPlugins(const std::vector<fs::path>& p)
{
    plugins_ = p;
}

std::unique_ptr<Cluster>
ArakoonTestSetup::make_client(const arakoon::Cluster::MaybeMilliSeconds& mms) const
{
    return std::make_unique<Cluster>(clusterID(),
                                     node_configs(),
                                     mms);
}

}

// Local Variables: **
// mode: c++ **
// End: **
