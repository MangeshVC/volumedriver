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

#include <exception>
#include <iostream>
#include <cstdlib>

#include <boost/foreach.hpp>
#include <boost/scope_exit.hpp>
#include <boost/program_options.hpp>

#include <fawltyfs/FileSystem.h>
#include <fawltyfs/Logger.h>
#include <fawltyfs/Logging.h>
#include <fawltyfs/scmrevision.h>

#define EXEC_NAME "FawltyFS"

namespace po = boost::program_options;

namespace
{

void
fuse_print_helper(const std::string& argv0,
                  const std::string& opt)
{
    std::vector<char*> argv(2);
    BOOST_SCOPE_EXIT((&argv))
    {
        BOOST_FOREACH(char* str, argv)
        {
            free(str);
        }
    }
    BOOST_SCOPE_EXIT_END;

    argv[0] = strdup(argv0.c_str());
    argv[1] = strdup(opt.c_str());
    fuse_operations dummy;
    bzero(&dummy, sizeof(dummy));
    fuse_main(argv.size(), &argv[0], &dummy, NULL);
}

void
print_fuse_help(const std::string& argv0)
{
    fuse_print_helper(argv0, "-ho");
}

void
print_fuse_version(const std::string& argv0)
{
    fuse_print_helper(argv0, "-V");
}
}

int
main(int argc,
     char** argv)
try
{
    fawltyfs::Logger::readLocalConfig(EXEC_NAME "Logging",
                                      "INFO");

    umask(0);

    std::string sourcedir;
    std::string mountpoint;

    po::options_description help("Help options");
    help.add_options()
        ("help,h", "produce help message")
        ("version,v", "show version info");

    po::options_description options("Options");

    options.add_options()
        ("sourcedir",
         po::value<std::string>(&sourcedir)->required(),
         "path to source dir")
        ("mountpoint",
         po::value<std::string>(&mountpoint)->required(),
         "path to mountpoint");

    po::variables_map vm;

    po::command_line_parser p = po::command_line_parser(argc, argv);
    p.options(help);
    p.allow_unregistered();
    po::store(p.run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << help << std::endl;
        std::cout << options << std::endl;
        std::cout << "FUSE specific options: " << std::endl << std::endl;
        print_fuse_help(argv[0]);
    }
    if (vm.count("version"))
    {
        std::cout << EXEC_NAME << " version: " << SCM_BRANCH << ": " <<
            SCM_REVISION << std::endl;
        print_fuse_version(argv[0]);
    }
    if (vm.count("version") or vm.count("help"))
    {
        return 0;
    }

    po::command_line_parser clp(argc, argv);
    po::parsed_options parsed_opts = clp.options(options).allow_unregistered().run();
    po::store(parsed_opts, vm);
    po::notify(vm);

    std::vector<std::string>
        unrecognized(po::collect_unrecognized(parsed_opts.options,
                                              po::include_positional));


    fawltyfs::FileSystem fs(sourcedir, mountpoint, unrecognized);
    fs.mount();
}
catch (std::exception& e)
{
    LOG_FFATAL(EXEC_NAME, "Caught exception: " << e.what() << " - exiting");
    exit(EXIT_FAILURE);
}
catch (...)
{
    LOG_FFATAL(EXEC_NAME, "Caught unknown exception - exiting");
    exit(EXIT_FAILURE);
}

// Local Variables: **
// compile-command: "scons -D -j 4" **
// mode: c++ **
// End: **
