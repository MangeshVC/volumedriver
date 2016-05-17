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

#include "../Assert.h"
#include "../LoggerPrivate.h"
#include "../Logging.h"
#include "../SourceOfUncertainty.h"
#include "../System.h"
#include "../TestBase.h"

#include <stdlib.h>
#include <iostream>
#include <locale.h>
#include <string.h>
#include <string>

#include <boost/log/attributes/constant.hpp>
#include <boost/log/trivial.hpp>

#include <boost/random/discrete_distribution.hpp>
#include <boost/thread.hpp>
#include <boost/chrono/chrono.hpp>

namespace youtilstest
{

using namespace youtils;
using namespace std::literals::string_literals;

namespace
{

const std::vector<std::string> names {
    "AAAAAAA",
        "BBBBBBB",
        "CCCCCCC",
        "DDDDDDD",
        "EEEEEEE",
        "FFFFFFF",
        "GGGGGGG",
        "HHHHHHH",
        "IIIIIIII",
        "JJJJJJJJ",
        "KKKKKKKK",
        "LLLLLLLL"
        };

const std::vector<Severity> severities {
    Severity::trace,
        Severity::debug,
        Severity::info,
        Severity::warning,
        Severity::info,
        Severity::fatal,
        Severity::notification
        };

}

class TestLogging
    : public TestBase
{
protected:
    TestLogging()
    {}

    virtual void SetUp(void)
    {}

    virtual void TearDown(void)
    {}

public:
    DECLARE_LOGGER("TestLogging");
};

TEST_F(TestLogging, test1)
{

    LOG_ERROR("Got My Mojo Working " << 3 << " " << std::boolalpha <<  true);
    LOG_ERROR("Got My Mojo Not Working " << std::noboolalpha << false << " " << 3.14);
}

struct LoggerThread
{
    LoggerThread(const Severity sev)
        : sev_(sev)
        , stop_(false)
    {}

    void operator()()
    {
        while(not stop_)
        {
            static unsigned i = 0;
            LOG_TRACE("Message " << sev_  << " " << i);
            usleep(100);
        }
    }

    Logger::logger_type&
    getLogger__()
    {
        return TestLogging::getLogger__();
    }

    const Severity sev_;
    bool stop_;
};

TEST_F(TestLogging, test_setup_teardown)
{
    const unsigned num_loggers = 32;
    std::vector<LoggerThread> loggers;

    for(unsigned i = 0; i < num_loggers; ++i)
    {
        loggers.emplace_back(Severity::fatal);
    }

    std::vector<boost::thread*> threads;

    for(LoggerThread& logger : loggers)
    {
        threads.push_back(new boost::thread(boost::ref(logger)));
    }

    for(unsigned i =0 ; i < 1024; ++i)
    {
        Logger::teardownLogging();
        usleep(2000);
        const std::vector<std::string> sinks = { Logger::console_sink_name() };
        Logger::setupLogging("LoggingSetupTearDownTest",
                             sinks,
                             Severity::trace,
                             LogRotation::F);
        usleep(2000);
    }

    for(LoggerThread& logger : loggers)
    {
        logger.stop_ = true;
    }

    for(boost::thread* thread : threads)
    {
        thread->join();
        delete thread;
    }
}

TEST_F(TestLogging, test_flushing)
{
    unsigned num_tests=10;

    for(unsigned i = 0; i < num_tests; ++i)
    {
        LOG_INFO("INFO Message " << i);
        sleep(1);
    }
}

TEST_F(TestLogging, DISABLED_lazylogging)
{
    for(int i = 0; i < 10; ++i)
    {
        LOG_NOTIFY("Another one bites the dust");
        sleep(5);
    }
}

TEST_F(TestLogging, performance)
{
    const unsigned num_tests = 1024 * System::get_env_with_default("TEST_LOGGING_PERFORMANCE_TIMES_K", 0);

    for(unsigned i = 0; i < num_tests; ++i)
    {
        LOG_TRACE("TRACE Message " << i );
        LOG_DEBUG("DEBUG Message " << i );
        LOG_INFO("INFO Message " << i );
        LOG_WARN("WARN Message " << i);
        LOG_ERROR("ERROR Message " << i);
        LOG_FATAL("FATAL Message " << i);
    }
}

TEST_F(TestLogging, trace_performance)
{
    const unsigned num_tests = 1024 * System::get_env_with_default("TEST_LOGGING_PERFORMANCE_TIMES_K", 0);

    for(unsigned i = 0; i < num_tests; ++i)
    {
        LOG_TRACE("TRACE Message " << i);
    }
}

namespace
{

struct PlayWithLogger
{
    PlayWithLogger()
        : stop_(false)
    {}
    typedef std::pair<std::string, Severity> filter_t;

    void operator()()
    {
        const boost::chrono::duration<float>  sleep_period(0.005);

        while(not stop_)
        {
            double probabilities[] = { 0.47, 0.47, 0.3, 0.3 };
            boost::random::discrete_distribution<> dist(probabilities);
            int i = dist(gen);
            switch(i)
            {
            case 0:
                Logger::add_filter(randomString(),
                                   randomSeverity());
                break;

            case 1:
                Logger::remove_filter(randomString());
                break;

            case 2:
                print_filters();
                Logger::remove_all_filters();
                print_filters();
                break;
            case 3:
                std::cout << "General Logging " << Logger::generalLogging() << std::endl;
                Logger::generalLogging(randomSeverity());
                std::cout << "General Logging " << Logger::generalLogging() << std::endl;
                break;
            }
            boost::this_thread::sleep_for(sleep_period);

        }
    }

    void
    print_filters()
    {
        std::vector<filter_t> filters;
        Logger::all_filters(filters);
        std::cout << filters.size() << " filters: " << std::endl;
        for(const filter_t& filter : filters)
        {
            std::cout << "\t" << filter.first << " : " << filter.second << std::endl;
        }
    }

    std::string
    randomString()
    {
        static boost::random::uniform_int_distribution<unsigned>dist(0, names.size()-1);
        unsigned index = dist(gen);
        ASSERT(index < names.size());
        return names[index];
    }

    Severity
    randomSeverity()
    {
        static boost::random::uniform_int_distribution<unsigned>dist(0, severities.size()-1);
        unsigned index = dist(gen);
        ASSERT(index < severities.size());
        return severities[index];
    }

    bool stop_;
    boost::random::mt19937 gen;
};

}

TEST_F(TestLogging, DISABLED_torture)
{
    const unsigned num_loggers = 32;

    PlayWithLogger pwl;



    std::vector<LoggerThread> loggers;

    for(unsigned i = 0; i < num_loggers; ++i)
    {
        loggers.emplace_back(Severity::fatal);
    }

    std::vector<boost::thread*> threads;

    for(LoggerThread& logger : loggers)
    {
        threads.push_back(new boost::thread(boost::ref(logger)));
    }

    boost::thread t(boost::ref(pwl));

    sleep(60);

    for(LoggerThread& logger : loggers)
    {
        logger.stop_ = true;
    }

    for(boost::thread* thread : threads)
    {
        thread->join();
        delete thread;
    }


    pwl.stop_ = true;
    t.join();
}

TEST_F(TestLogging, lexical_cast)
{
#define T(sev, as_str)                                                  \
    {                                                                   \
        const Severity s = Severity::sev;                               \
        const std::string str(boost::lexical_cast<std::string>(s));     \
        EXPECT_EQ(as_str, str);                                         \
        const Severity t(boost::lexical_cast<Severity>(str));           \
        EXPECT_EQ(s, t);                                                \
    }

    T(trace, "trace");
    T(debug, "debug");
    T(info, "info");
    T(warning, "warning");
    T(fatal, "fatal");
    T(notification, "notice");

#undef T

    EXPECT_THROW(boost::lexical_cast<Severity>("utterly important, really"),
                 std::exception);
}

namespace blt = boost::log::trivial;

TEST_F(TestLogging, DISABLED_integration_with_trivial_boost_logging)
{
    blt::logger::get().add_attribute(LOGGER_ATTRIBUTE_ID,
                                     boost::log::attributes::constant<LOGGER_ATTRIBUTE_ID_TYPE>("trivial"));
    blt::logger::get().add_attribute("Severity",
                                     boost::log::attributes::constant<Severity>(Severity::info));

    BOOST_LOG_TRIVIAL(trace) << "A trace severity message";
    BOOST_LOG_TRIVIAL(debug) << "A debug severity message";
    BOOST_LOG_TRIVIAL(info) << "An informational severity message";
    BOOST_LOG_TRIVIAL(warning) << "A warning severity message";
    BOOST_LOG_TRIVIAL(error) << "An error severity message";
    BOOST_LOG_TRIVIAL(fatal) << "A fatal severity message";
}

// Semi-manual test for OVS-2680:
// make sure to set the rotation interval to < 3 secs in Logger::setup_file_logging and
// to run with logging enabled, of course.
TEST_F(TestLogging, DISABLED_exceptional_rotation)
{
    const auto logfile(System::get_env_with_default("LOG_FILE",
                                                    "voldrv.log"s));

    fs::remove_all(logfile);

    std::this_thread::sleep_for(std::chrono::seconds(3));
    LOG_INFO(logfile << ": should be gone by now - let's see what happens if we log and trigger a rotation that way");
    LOG_INFO(logfile << ": the prior message is probably gone, but this one should be visible");
}

}

// Local Variables: **
// mode: c++ **
// End: **
