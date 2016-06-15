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

#include "../Restore.h"
#include "../VolManager.h"

#include <iostream>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <youtils/BuildInfoString.h>
#include <youtils/HeartBeatLockService.h>
#include <youtils/Logger.h>
#include <youtils/Main.h>
#include <youtils/WithGlobalLock.h>

#include <backend/LockStore.h>

namespace
{

namespace be = backend;
namespace bpt = boost::property_tree;
namespace po = boost::program_options;
namespace vd = volumedriver;
namespace vd_bu = volumedriver_backup;
namespace yt = youtils;

class RestoreMain
    : public yt::MainHelper
{
public:
    RestoreMain(int argc,
                char** argv)
        : MainHelper(argc,
                     argv)
        , restore_options_("Restore Options")
    {
        restore_options_.add_options()
        ("configuration-file",
         po::value<std::string>(&configuration_file_)->required(),
         "File that holds the configuration for this job")
        ("barf-on-busted-backup",
         po::value<bool>(&barf_on_busted_backup_)->default_value(false),
         "Will exit a restore when it recognizes it's being asked to restore from a failed backup");
    }

    virtual void
    setup_logging()
    {
        MainHelper::setup_logging("restore");
    }

    virtual void
    log_extra_help(std::ostream& strm)
    {
        strm << restore_options_;
    }


    virtual void
    parse_command_line_arguments()
    {
        parse_unparsed_options(restore_options_,
                               yt::AllowUnregisteredOptions::T,
                               vm_);
    }

    virtual int
    run()
        try
        {
            bpt::ptree config_ptree;
            bpt::json_parser::read_json(configuration_file_,
                                                         config_ptree);

            boost::optional<bpt::ptree&> source_ptree_opt =
                config_ptree.get_child_optional("source_configuration");
            if(source_ptree_opt)
            {
                auto bcm(be::BackendConnectionManager::create(*source_ptree_opt));

                const uint64_t update_interval = config_ptree.get<uint64_t>("global_lock_update_interval_in_seconds", 30);
                const uint64_t grace_period_in_seconds = config_ptree.get<uint64_t>("grace_period_in_seconds", 30);
                LOG_INFO("global_lock_update_interval_in_seconds is " << update_interval);

                vd_bu::Restore restore(configuration_file_,
                                       barf_on_busted_backup_,
                                       yt::GracePeriod(boost::posix_time::seconds(grace_period_in_seconds)));

                using LockedRestore =
                    yt::HeartBeatLockService::WithGlobalLock<yt::ExceptionPolicy::ThrowExceptions,
                                                             vd_bu::Restore,
                                                             &vd_bu::Restore::info>;

                const std::string locking_namespace =
                    source_ptree_opt->get<std::string>("namespace");

                yt::GlobalLockStorePtr
                    lock_store(new be::LockStore(bcm->newBackendInterface(be::Namespace(locking_namespace))));

                LockedRestore locked_restore(boost::ref(restore),
                                             yt::NumberOfRetries(1),
                                             LockedRestore::connection_retry_timeout_default(),
                                             lock_store,
                                             yt::UpdateInterval(boost::posix_time::seconds(update_interval)));

                try
                {
                    locked_restore();
                }
                catch(yt::WithGlobalLockExceptions::CouldNotInterruptCallable& e)
                {
                    LOG_FATAL("Aborting because the worker thread could not be interrupted and we *don't* have the lock anymore");
                    exit(1);
                }

                catch(yt::WithGlobalLockExceptions::CallableException& e)
                {
                    std::rethrow_exception(e.exception_);
                }
            }
            else
            {
                vd_bu::Restore restore(configuration_file_,
                                       barf_on_busted_backup_,
                                       yt::GracePeriod(boost::posix_time::seconds(30)));
                restore();
            }
            return 0;
        }
        catch(std::exception& e)
        {
            std::cerr << "Exception exiting main: " << e.what() << std::endl;
            throw;

        }
        catch(...)
        {
            std::cerr << "Unknown exception exiting main: " << std::endl;
            throw;
        }


    std::string configuration_file_;
    bool barf_on_busted_backup_;
    po::options_description restore_options_;

};
}

MAIN(RestoreMain)

// Local Variables: **
// mode: c++ **
// End: **
