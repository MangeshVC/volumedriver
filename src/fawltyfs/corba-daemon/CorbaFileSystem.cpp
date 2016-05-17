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

#include "CorbaFileSystem.h"
#include "../ErrorRules.h"
#include <youtils/Assert.h>
#include "Literals.h"
void
CorbaFileSystem::mount()
{
    fawltyfs::FileSystem::mount();
}

void
CorbaFileSystem::umount()
{
    fawltyfs::FileSystem::umount();
}




void
CorbaFileSystem::addDelayRule(const int rule_id,
                              const char* path_regex,
                              const Fawlty::FileSystemCalls& calls,
                              const int ramp_up,
                              const int duration,
                              const int idelay_usecs)
{


    fawltyfs::DelayRulePtr dr(new fawltyfs::DelayRule(rule_id,
                                                      path_regex,
                                                      fromCorba(calls),
                                                       ramp_up,
                                                      duration,
                                                      idelay_usecs));
    fawltyfs::FileSystem::addDelayRule(dr);


}

void
CorbaFileSystem::removeDelayRule(const int rule_id)
{
    fawltyfs::FileSystem::removeDelayRule(rule_id);

}

Fawlty::DelayRules*
CorbaFileSystem::listDelayRules()
{
    fawltyfs::delay_rules_list_t d_r;
    fawltyfs::FileSystem::listDelayRules(d_r);

    size_t len = d_r.size();

    Fawlty::DelayRules* delay_rules = new Fawlty::DelayRules(len);
    size_t i = 0;
    delay_rules->length(len);

    for(const fawltyfs::DelayRulePtr& rule : d_r)
    {
        Fawlty::DelayRule d;
        d.id = rule->id;
        d.path_regex = rule->path_regex.str().c_str();
        d.calls = toCorba(rule->calls_);
        d.ramp_up = rule->ramp_up;
        d.duration = rule->duration;
        d.delay_usecs = rule->delay_usecs;
        VERIFY(i < delay_rules->length());

        (*delay_rules)[i++] = d;
    }
    VERIFY(i == len);

    return delay_rules;
}


void
CorbaFileSystem::addFailureRule(const int rule_id,
                                const char* path_regex,
                                const Fawlty::FileSystemCalls& calls,
                                const int ramp_up,
                                const int duration,
                                const int errcode)
{
    fawltyfs::FailureRulePtr fr(new fawltyfs::FailureRule(rule_id,
                                                          path_regex,
                                                          fromCorba(calls),
                                                          ramp_up,
                                                          duration,
                                                          errcode));
    fawltyfs::FileSystem::addFailureRule(fr);

}

void
CorbaFileSystem::removeFailureRule(const int rule_id)
{
    fawltyfs::FileSystem::removeFailureRule(rule_id);
}

Fawlty::FailureRules*
CorbaFileSystem::listFailureRules()
{
    fawltyfs::failure_rules_list_t f_r;
    fawltyfs::FileSystem::listFailureRules(f_r);

    size_t len = f_r.size();

    Fawlty::FailureRules* failure_rules = new Fawlty::FailureRules(len);
    failure_rules->length(len);


    size_t i = 0;
    for(const fawltyfs::FailureRulePtr& rule : f_r)
    {
        Fawlty::FailureRule d;
        d.id = rule->id;
        d.path_regex = rule->path_regex.str().c_str();
        d.calls = toCorba(rule->calls_);
        d.ramp_up = rule->ramp_up;
        d.duration = rule->duration;
        d.errcode = rule->errcode;
        VERIFY(i < failure_rules->length());

        (*failure_rules)[i++] = d;
    }
    VERIFY(i == len);

    return failure_rules;

}

void
CorbaFileSystem::addDirectIORule(const int rule_id,
                                 const char* path_regex,
                                 const bool direct_io)
{
    fawltyfs::DirectIORulePtr dio(new fawltyfs::DirectIORule(rule_id,
                                                             path_regex,
                                                             direct_io));
    fawltyfs::FileSystem::addDirectIORule(dio);
}


void
CorbaFileSystem::removeDirectIORule(const int rule_id)
{
    fawltyfs::FileSystem::removeDirectIORule(rule_id);
}


Fawlty::DirectIORules*
CorbaFileSystem::listDirectIORules()
{
    fawltyfs::direct_io_rules_list_t dios;
    fawltyfs::FileSystem::listDirectIORules(dios);

    size_t len = dios.size();
    Fawlty::DirectIORules* directio_rules = new Fawlty::DirectIORules(len);
    directio_rules->length(len);

    size_t i = 0;
    for(const fawltyfs::DirectIORulePtr& rule : dios)
    {
        Fawlty::DirectIORule d;
        d.id = rule->id;
        d.path_regex = rule->path_regex.str().c_str();
        d.direct_io = rule->direct_io;
        (*directio_rules)[i++] = d;
    }
    VERIFY(i == len);
    return directio_rules;
}
