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

#include "../MainHelper.h"
#include "../Main.h"
#include "../OrbHelper.h"
#include "corba_youtils.h"

namespace youtils_test
{
using namespace youtils;
namespace po = boost::program_options;

class CorbaYoutilsServant
{
public:
    CorbaYoutilsServant(OrbHelper& orb_helper)
        : orb_helper_(orb_helper)
    {}

    ~CorbaYoutilsServant()
    {}

    void
    stop()
    {
        orb_helper_.stop();
    }
private:
    OrbHelper& orb_helper_;
};


class YoutilsCorbaServer : public MainHelper
{
public:
    YoutilsCorbaServer(int argc,
                       char** argv)
        : MainHelper(argc,
                     argv)
        , opts_("Tester Specific Options")
    {}

    void
    parse_command_line_arguments() override final
    {
        parse_corba_server_args();

        parse_unparsed_options(opts_,
                               youtils::AllowUnregisteredOptions::F,
                               vm_);
    }

    void
    log_extra_help(std::ostream& strm) override final
    {
        strm << opts_;
        log_corba_server_help(strm);
    }

    void
    setup_logging() override final
    {
        MainHelper::setup_logging("corba_youtils_server");
    }

    int
    run() override final
    {
        OrbHelper orb(executable_name_);

        CORBA::Object_var obj = orb.orb()->resolve_initial_references("RootPOA");

        PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

        POA_youtils_test::corba_test_tie<CorbaYoutilsServant>
            corba_youtils_servant(new CorbaYoutilsServant(orb));

        PortableServer::ObjectId_var corba_youtils_servant_id =
            poa->activate_object(&corba_youtils_servant);

        CORBA::Object_var obj1;

        obj1  = corba_youtils_servant._this();
        orb.bindObjectToName(obj1,
                             "context_name",
                             "context_kind",
                             "object_name",
                             "object_kind");


        PortableServer::POAManager_var pman = poa->the_POAManager();
        pman->activate();


        omni_thread::sleep(1);
        orb.orb()->run();

        poa->deactivate_object(corba_youtils_servant_id);

        return 0;
    }

private:
    po::options_description opts_;
};


}

MAIN(youtils_test::YoutilsCorbaServer)
