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

#include "../../Assert.h"
#include "../../FileUtils.h"
#include "../../Main.h"
#include <pstreams/pstream.h>
#include "../../wall_timer.h"
#include "../../TestBase.h"
#include "../../TestMainHelper.h"

#include "../prudence.h"

#include <sys/wait.h>

#include <boost/filesystem/fstream.hpp>
#include <boost/thread.hpp>

namespace youtils_test
{

using namespace youtils;

namespace fs = boost::filesystem;
namespace po = boost::program_options;

class TestPrudence
    : public youtilstest::TestBase
{

protected:
    CleanedUpFile
    write_script(const std::vector<std::string>& lines,
                 const std::string& name)
    {
        CleanedUpFile p(new fs::path(FileUtils::create_temp_file_in_temp_dir(name)));

        fs::ofstream os(*p);
        os << "#! /bin/bash" << std::endl;
        os << "# This script was written as part of the prudence tester" << std::endl;
        for(const std::string& line : lines)
        {
            os << line;

       }
        int res = chmod(p->string().c_str(),0777);
        EXPECT_EQ(res, 0);


        return p;

    }

    DECLARE_LOGGER("TestPrudence");

    void
    read_stream_till_exit(redi::ipstream& ipstream)
    {
        std::string line;

        while(std::getline(ipstream, line))
        {
            std::cout << line << std::endl;
        }

        // Occasionally the tests failed b/c
        // ipstream.rdbuf()->exited is not immediately
        // true after the loop (race condition?).
        // Let's give it some time.
        const size_t count = 100;
        const size_t msecs = 10;
        wall_timer t;

        for (size_t i = 0; i < count; ++i)
        {
            if (ipstream.rdbuf()->exited())
            {
                return;
            }
            else
            {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(msecs));
            }
        }

        FAIL() << "child has not exited after " << t.elapsed() << " seconds";
    }
};

TEST_F(TestPrudence, test1)
{
    std::vector<std::string> argv;
    argv.push_back("prudence");
    argv.push_back("--loglevel=debug");

    redi::ipstream test1(argv);
    read_stream_till_exit(test1);

    auto exit_status = test1.rdbuf()->status();
    EXPECT_TRUE(WIFEXITED(exit_status));
    EXPECT_FALSE(WIFSIGNALED(exit_status));
    // int WTEint WTERMSIG(int status) --> can get the signal
    EXPECT_EQ(WEXITSTATUS(exit_status), StartScriptNotOk);
}

TEST_F(TestPrudence, test2)
{
    using namespace std::string_literals;

    fs::path file = FileUtils::create_temp_file_in_temp_dir("to_be_deleted");
    ALWAYS_CLEANUP_FILE(file);

    std::vector<std::string> start_script_lines;
    start_script_lines.emplace_back("rm "s + file.string());

    CleanedUpFile p = write_script(start_script_lines,
                                   "test2_start_script");

    std::vector<std::string> argv;
    argv.push_back("prudence");
    argv.push_back("--loglevel=trace");
    argv.push_back("--start-script=" + p->string());
    redi::ipstream test2(argv);
    read_stream_till_exit(test2);

    auto exit_status = test2.rdbuf()->status();
    EXPECT_TRUE(WIFEXITED(exit_status));
    EXPECT_FALSE(WIFSIGNALED(exit_status));
    // int WTEint WTERMSIG(int status) --> can get the signal
    EXPECT_EQ(WEXITSTATUS(exit_status), Success);
    EXPECT_FALSE(fs::exists(file));
}

TEST_F(TestPrudence, test3)
{
    using namespace std::string_literals;

    std::vector<std::string> start_script_lines;
    start_script_lines.emplace_back("while true; do echo 'help' >> /tmp/out; sleep 1; done\n");

    CleanedUpFile p = write_script(start_script_lines,
                                   "test3_start_script");

    std::vector<std::string> argv;
    argv.push_back("prudence");
    argv.push_back("--loglevel=trace");
    argv.push_back("--start-script="s + p->string());
    redi::ipstream test1(argv);
    boost::thread t([&test1, this]()
                    {
                        read_stream_till_exit(test1);
                    });
    sleep(3);
    std::cout << "Killing the parent" << std::endl;

    test1.rdbuf()->kill(parent_dies_signal);
    std::cout << "Joining" << std::endl;
    t.join();
    std::cout << "Joined" << std::endl;
    auto exit_status = test1.rdbuf()->status();
    EXPECT_TRUE(WIFEXITED(exit_status));
    EXPECT_FALSE(WIFSIGNALED(exit_status));
    // int WTEint WTERMSIG(int status) --> can get the signal
    EXPECT_EQ(WEXITSTATUS(exit_status), Success);
    //    EXPECT_FALSE(fs::exists(file));
}

TEST_F(TestPrudence, test4)
{
    using namespace std::string_literals;
    CleanedUpFile start;

    {
        std::vector<std::string> start_script_lines;
        start_script_lines.emplace_back("while true; do echo 'help' >> /tmp/out; sleep 1; done\n");

        start = write_script(start_script_lines,
                             "test3_start_script");
    }

    CleanedUpFile stop;
    {

        std::vector<std::string> stop_script_lines;
        stop_script_lines.emplace_back("rm "s + start->string());

        stop = write_script(stop_script_lines,
                            "test3_stop_script");
    }

    std::vector<std::string> argv;
    argv.push_back("prudence");
    argv.push_back("--loglevel=trace");
    argv.push_back("--start-script="s + start->string());
    argv.push_back("--cleanup-script="s + stop->string());
    redi::ipstream test1(argv);
    boost::thread t([&test1, this]()
                    {
                        read_stream_till_exit(test1);
                    });
    sleep(3);
    std::cout << "Killing the parent" << std::endl;

    test1.rdbuf()->kill(parent_dies_signal);
    std::cout << "Joining" << std::endl;
    t.join();
    std::cout << "Joined" << std::endl;
    auto exit_status = test1.rdbuf()->status();
    EXPECT_TRUE(WIFEXITED(exit_status));
    EXPECT_FALSE(WIFSIGNALED(exit_status));
    // int WTEint WTERMSIG(int status) --> can get the signal
    EXPECT_EQ(WEXITSTATUS(exit_status), Success);
    EXPECT_FALSE(fs::exists(*start));
    EXPECT_TRUE(fs::exists(*stop));
}

TEST_F(TestPrudence, test5)
{
    using namespace std::string_literals;
    CleanedUpFile start;
    fs::path start_file = FileUtils::create_temp_file_in_temp_dir("starter");
    ALWAYS_CLEANUP_FILE(start_file);


    {
        std::vector<std::string> start_script_lines;
        start_script_lines.emplace_back("trap \"rm "s + start_file.string() + "; exit\" SIGTERM\n"s);

        start_script_lines.emplace_back("while true; do echo 'help' >> /tmp/out; sleep 1; done\n"s);
        start = write_script(start_script_lines,
                             "test3_start_script");
    }

    CleanedUpFile stop;
    fs::path stop_file = FileUtils::create_temp_file_in_temp_dir("stopper");
    ALWAYS_CLEANUP_FILE(stop_file);

    {
        std::vector<std::string> stop_script_lines;
        stop_script_lines.emplace_back("trap \"rm "s + stop_file.string() + "; exit\" SIGTERM\n"s);

        stop_script_lines.emplace_back("while true; do echo 'help' >> /tmp/out; sleep 1; done\n");
        stop = write_script(stop_script_lines,
                            "test3_stop_script");
    }

    std::vector<std::string> argv;
    argv.push_back("prudence");
    argv.push_back("--loglevel=trace");
    argv.push_back("--start-script="s + start->string());
    argv.push_back("--cleanup-script="s + stop->string());
    redi::ipstream test1(argv);
    boost::thread t([&test1, this]()
                    {
                        read_stream_till_exit(test1);
                    });
    sleep(3);
    std::cout << "Killing the parent" << std::endl;

    test1.rdbuf()->kill(parent_dies_signal);
    std::cout << "Joining" << std::endl;
    t.join();
    std::cout << "Joined" << std::endl;
    auto exit_status = test1.rdbuf()->status();
    EXPECT_TRUE(WIFEXITED(exit_status));
    EXPECT_FALSE(WIFSIGNALED(exit_status));
    // int WTEint WTERMSIG(int status) --> can get the signal
    EXPECT_EQ(WEXITSTATUS(exit_status), Success);
    EXPECT_FALSE(fs::exists(start_file));
    EXPECT_TRUE(fs::exists(stop_file));
}

class TestPrudenceMain
    : public TestMainHelper
{
public:
    TestPrudenceMain(int argc,
                     char** argv)
        : TestMainHelper(argc,
                         argv)
        , opts_("Tester Specific Options")
    {}

    void
    parse_command_line_arguments() override final
    {
        init_google_test();
        parse_unparsed_options(opts_,
                               youtils::AllowUnregisteredOptions::F,
                               vm_);
    }

    void
    log_extra_help(std::ostream& strm) override final
    {
        strm << opts_;
        log_google_test_help(strm);
    }

    void
    setup_logging() override final
    {
        MainHelper::setup_logging("prudence_test");
    }

private:
    po::options_description opts_;
};

}

MAIN(youtils_test::TestPrudenceMain)
