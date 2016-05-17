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

#include "ArakoonLockStoreBuilder.h"

namespace volumedriver
{

namespace ara = arakoon;
namespace bpt = boost::property_tree;
namespace yt = youtils;

ArakoonLockStoreBuilder::ArakoonLockStoreBuilder(const boost::property_tree::ptree& pt)
    : LockStoreBuilder(pt,
                       RegisterComponent::F)
    , dls_arakoon_cluster_id(pt)
    , dls_arakoon_cluster_nodes(pt)
    , dls_arakoon_timeout_ms(pt)
{
    THROW_WHEN(dls_arakoon_cluster_id.value().empty());
    THROW_WHEN(dls_arakoon_cluster_nodes.value().empty());

    larakoon_ =
        std::make_shared<yt::LockedArakoon>(ara::ClusterID(dls_arakoon_cluster_id.value()),
                                            dls_arakoon_cluster_nodes.value(),
                                            ara::Cluster::MilliSeconds(dls_arakoon_timeout_ms.value()));
}

yt::GlobalLockStorePtr
ArakoonLockStoreBuilder::build_one(const backend::Namespace& nspace)
{
    return yt::GlobalLockStorePtr(new yt::ArakoonLockStore(larakoon_,
                                                           nspace.str()));
}

void
ArakoonLockStoreBuilder::update(const bpt::ptree& pt,
                                youtils::UpdateReport& rep)
{
#define U(val)                                  \
    val.update(pt, rep)

    U(dls_arakoon_cluster_id);
    U(dls_arakoon_timeout_ms);

    bool updated = false;

    const decltype(dls_arakoon_timeout_ms) timeout(pt);
    const auto tms(ara::Cluster::MilliSeconds(timeout.value()));
    try
    {
        if (timeout.value() != dls_arakoon_timeout_ms.value())
        {
            larakoon_->timeout(tms);
        }

        updated = true;
    }
    CATCH_STD_ALL_LOG_IGNORE("Failed to update arakoon timeout");

    if (updated)
    {
        U(dls_arakoon_timeout_ms);
    }
    else
    {
        rep.no_update(dls_arakoon_timeout_ms.name());
    }

    updated = false;

    try
    {
        const decltype(dls_arakoon_cluster_nodes) nodes(pt);

        if (nodes.value() != dls_arakoon_cluster_nodes.value())
        {
            larakoon_->reconnect(nodes.value(),
                                 tms);
        }

        updated = true;

    }
    CATCH_STD_ALL_LOG_IGNORE("Failed to update arakoon config");

    if (updated)
    {
        U(dls_arakoon_cluster_nodes);
    }
    else
    {
        rep.no_update(dls_arakoon_cluster_nodes.name());
    }

#undef U
}

void
ArakoonLockStoreBuilder::persist(bpt::ptree& pt,
                                 const ReportDefault report_default) const
{
#define P(var)                                  \
    (var).persist(pt, report_default);

    P(dls_arakoon_cluster_id);
    P(dls_arakoon_cluster_nodes);
    P(dls_arakoon_timeout_ms);

#undef P
}

bool
ArakoonLockStoreBuilder::checkConfig(const bpt::ptree& pt,
                                     youtils::ConfigurationReport& crep) const
{
    const decltype(dls_arakoon_cluster_nodes) new_nodes(pt);
    if (new_nodes.value().empty())
    {
        crep.push_back(yt::ConfigurationProblem(new_nodes.name(),
                                                new_nodes.section_name(),
                                                "empty arakoon node configs not permitted"));
        return false;
    }
    else
    {
        return true;
    }
}

}
