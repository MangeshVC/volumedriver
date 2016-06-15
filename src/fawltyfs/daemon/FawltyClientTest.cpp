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

#include <youtils/TestBase.h>
#include <pstreams/pstream.h>
#include <youtils/Logging.h>
#include <youtils/FileUtils.h>
#include "Fawlty.h"

#include <transport/TSocket.h>
#include <transport/TBufferTransports.h>
#include <protocol/TBinaryProtocol.h>
#include "FawltyClient.h"
#include <youtils/Assert.h>
#include <boost/filesystem.hpp>
// For the waitpid WFEXITSTATUS macros.
#include <sys/types.h>
#include <sys/wait.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace fawlty_daemon;
namespace fs = boost::filesystem;



class FawltyClientTest : public youtilstest::TestBase
{
    typedef redi::ipstream stream_t;

public:
    FawltyClientTest()
        : pstream_(new stream_t())
        , directory_(youtils::FileUtils::temp_path() / "FawltyClientTest")
    {}
    DECLARE_LOGGER("FawltyClientTest");

    void SetUp()
    {
        youtils::FileUtils::ensure_directory(directory_);

        // port negotiation here!
        port_ = port();
        ASSERT_TRUE(port_ > 0);
        do
        {
            std::stringstream command;
            command << "./FawltyServer --port=" << port_;

            pstream_->open(command.str(), redi::pstreambuf::pstdout | redi::pstreambuf::pstderr);
            while(not pstream_->is_open())
            {
                sleep(1);
            }
            socket_.reset(new TSocket("localhost", port_));

            transport_.reset(new TBufferedTransport(socket_));

            protocol_.reset(new TBinaryProtocol(transport_));

            client_.reset(new FawltyClient(protocol_));

            try
            {
                transport_->open();
            }
            catch(...)
            {}

            if(pstream_->rdbuf()->exited())
            {
                int status = pstream_->rdbuf()->status();
                if( WEXITSTATUS(status) == 3)
                {
                    ++port_;
                    continue;
                }
                else
                {
                    abort();
                }
            }
            break;
        } while(port_ < (port() + port_range()));

        VERIFY(not pstream_->rdbuf()->exited());
        LOG_INFO("Started on port " << port_);

    }

    void TearDown()
    {

        if(client_)
        {
            LOG_INFO("Stopping The client");
            client_->stop();
        }

        if(transport_)
        {
            LOG_INFO("Closing the transport");

            transport_->close();
        }
        if(pstream_)
        {
            LOG_INFO("Closing the pstream");

            pstream_->close();
        }
        youtils::FileUtils::removeAllNoThrow(directory_);

    };

    std::list<std::string>
    get_standard_args()
    {
        std::list<std::string> res = { "-f", "-s" };
        return res;
    }

    std::tuple<fs::path,
               fs::path>
    get_dir_pair()
    {
        fs::path frontend = youtils::FileUtils::create_temp_dir(directory_,
                                                                "frontend");
        fs::path backend = youtils::FileUtils::create_temp_dir(directory_,
                                                               "backend");
        return std::make_pair(frontend, backend);
    }

private:
    boost::shared_ptr<TSocket> socket_;
    boost::shared_ptr<TTransport> transport_;
    boost::shared_ptr<TProtocol> protocol_;
    std::shared_ptr<stream_t> pstream_;
    const fs::path directory_;

protected:
    std::unique_ptr<FawltyClient> client_;
public:
    uint16_t port_;
    uint16_t port_range_;

};

TEST_F(FawltyClientTest, ping)
{
    int retval = client_->ping();
    EXPECT_EQ(retval, 0);
}

TEST_F(FawltyClientTest, make_file_system)
{

    fs::path front;
    fs::path back;
    std::tie(front, back) = get_dir_pair();

    std::list<std::string> fs_args = get_standard_args();

    try
    {
        filesystem_handle_t handle = 0;

        handle = client_->make_file_system(back.string(),
                                           front.string(),
                                           std::vector<std::string>(fs_args.begin(), fs_args.end()),
                                           "fawlty_client_test");
        ASSERT_TRUE(handle);
    }
    catch(FawltyException& e)
    {
        LOG_FATAL("caught exception " << e.problem);

    }


}

TEST_F(FawltyClientTest, mount)
{
    //    client_->mount();
}
