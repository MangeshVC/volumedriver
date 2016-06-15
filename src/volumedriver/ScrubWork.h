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

#ifndef _SCRUBWORK_H_
#define _SCRUBWORK_H_

#include "SnapshotName.h"
#include "Types.h"

#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <youtils/Serialization.h>

#include <backend/BackendConfig.h>


namespace scrubbing
{
struct ScrubWork
{
    ScrubWork(std::unique_ptr<backend::BackendConfig> backend_config,
              const volumedriver::Namespace& ns,
              const volumedriver::VolumeId& id,
              const volumedriver::ClusterExponent cluster_exponent,
              const uint32_t sco_size,
              const volumedriver::SnapshotName& snapshot_name)
        : backend_config_(std::move(backend_config))
        , ns_(ns)
        , id_(id)
        , cluster_exponent_(cluster_exponent)
        , sco_size_(sco_size)
        , snapshot_name_(snapshot_name)
    {}

    ScrubWork()
        : ns_()
    {}

    explicit ScrubWork(const std::string& in)
        : ns_()
    {
        std::stringstream iss(in);
        ScrubWork::iarchive_type ia(iss);
        ia & boost::serialization::make_nvp("scrubwork",
                                            *this);
    }

    std::string
    str() const
    {
        std::stringstream oss;
        ScrubWork::oarchive_type oa(oss);
        oa & boost::serialization::make_nvp("scrubwork",
                                            *this);
        return oss.str();
    }

    DECLARE_LOGGER("ScrubWork");

    typedef boost::archive::xml_iarchive iarchive_type;
    typedef boost::archive::xml_oarchive oarchive_type;

    std::unique_ptr<backend::BackendConfig> backend_config_;
    volumedriver::Namespace ns_;
    volumedriver::VolumeId id_;
    volumedriver::ClusterExponent cluster_exponent_;
    uint32_t sco_size_;
    volumedriver::SnapshotName snapshot_name_;

    BOOST_SERIALIZATION_SPLIT_MEMBER();

    template<class Archive>
    inline void
    save(Archive & ar,
         const unsigned int version) const
    {
        VERIFY(backend_config_.get());
        if(version == 2)
        {
            boost::property_tree::ptree pt;
            backend_config_->persist_internal(pt,
                                              ReportDefault::T);
            std::stringstream ss;
            write_json(ss,
                       pt);

            std::string backend_config = ss.str();

            ar & BOOST_SERIALIZATION_NVP(backend_config);
            ar & BOOST_SERIALIZATION_NVP(ns_);
            ar & BOOST_SERIALIZATION_NVP(id_);
            ar & BOOST_SERIALIZATION_NVP(cluster_exponent_);
            ar & BOOST_SERIALIZATION_NVP(sco_size_);
            ar & BOOST_SERIALIZATION_NVP(snapshot_name_);
        }
        else
        {
            throw youtils::SerializationVersionException("ScrubWork",
                                                         version,
                                                         2,
                                                         2);
        }
    }

    template<class Archive>
    void
    load(Archive& ar,
         const unsigned int version)
    {
        std::string backend_config;
        ar & BOOST_SERIALIZATION_NVP(backend_config);
        backend_config_ = backend::BackendConfig::makeBackendConfig(backend_config);

        ar & BOOST_SERIALIZATION_NVP(ns_);
        ar & BOOST_SERIALIZATION_NVP(id_);
        ar & BOOST_SERIALIZATION_NVP(cluster_exponent_);
        ar & BOOST_SERIALIZATION_NVP(sco_size_);

        if(version < 2)
        {
            std::string snap;
            ar & boost::serialization::make_nvp("snapshot_name_",
                                                snap);
            snapshot_name_ = volumedriver::SnapshotName(snap);
        }
        else
        {
            ar & BOOST_SERIALIZATION_NVP(snapshot_name_);
        }
    }
};

}

BOOST_CLASS_VERSION(scrubbing::ScrubWork, 2);

#endif // _SCRUBWORK_H_
