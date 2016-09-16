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

#ifndef ARAKOON_NODE_CONFIG_H_
#define ARAKOON_NODE_CONFIG_H_

#include "InitializedParam.h"

#include "Serialization.h"
#include "StrongTypedString.h"

#include <sstream>

STRONG_TYPED_STRING(arakoon, ClusterID);
STRONG_TYPED_STRING(arakoon, NodeID);

namespace arakoon
{

class ArakoonNodeConfig
{
public:
    ArakoonNodeConfig(const NodeID& node_id,
                      const std::string& hostname,
                      uint16_t port)
        : node_id_(node_id)
        , hostname_(hostname)
        , port_(port)
    {}

    ArakoonNodeConfig()
        : port_(0)
    {}

    ArakoonNodeConfig(const ArakoonNodeConfig& other)
        : node_id_(other.node_id_)
        , hostname_(other.hostname_)
        , port_(other.port_)
    {}

    ArakoonNodeConfig&
    operator=(const ArakoonNodeConfig& other)
    {
        if (this != &other)
        {
            const_cast<NodeID&>(node_id_) = other.node_id_;
            const_cast<std::string&>(hostname_) = other.hostname_;
            const_cast<uint16_t&>(port_) = other.port_;
        }
        return *this;
    }

    bool
    operator==(const ArakoonNodeConfig& other) const
    {
        return
            node_id_ == other.node_id_ and
            hostname_ == other.hostname_ and
            port_ == other.port_;
    }

    bool
    operator!=(const ArakoonNodeConfig& other) const
    {
        return not (*this == other);
    }

    bool
    operator<(const ArakoonNodeConfig& other) const
    {
        if (node_id_ < other.node_id_)
        {
            return true;
        }
        else if (node_id_ == other.node_id_)
        {
            if (hostname_ < other.hostname_)
            {
                return true;
            }
            else if (hostname_ == other.hostname_)
            {
                return port_ < other.port_;
            }
        }

        return false;
    }

    // for python consumption
    std::string
    str() const
    {
        std::stringstream ss;
        ss << "ArakoonNodeConfig { "
            "node_id: \"" << node_id_ << "\", " <<
            "host: \"" << hostname_ << "\", " <<
            "port: " << port_ << " }";
        return ss.str();
    }

    const NodeID node_id_;
    const std::string hostname_;
    const uint16_t port_;

private:
    friend class boost::serialization::access;

    BOOST_SERIALIZATION_SPLIT_MEMBER();

    template<class Archive>
    void
    load(Archive& ar,
         const unsigned version)
    {
        if(version == 1)
        {
            ar &  boost::serialization::make_nvp("node_id",
                                                 const_cast<NodeID&>(node_id_));
            ar & boost::serialization::make_nvp("hostname",
                                                const_cast<std::string&>(hostname_));
            ar & boost::serialization::make_nvp("port",
                                                const_cast<uint16_t&>(port_));
        }
        else
        {
            THROW_SERIALIZATION_ERROR(version, 1, 1);
        }
    }

    template<class Archive>
    void
    save(Archive& ar,
         const unsigned int version) const
    {
        if(version != 1)
        {
            THROW_SERIALIZATION_ERROR(version, 1, 1);
        }
        ar & boost::serialization::make_nvp("node_id",
                                            node_id_);
        ar & boost::serialization::make_nvp("hostname",
                                            hostname_);
        ar & boost::serialization::make_nvp("port",
                                            port_);
    }
};

std::ostream&
operator<<(std::ostream& os,
           const ArakoonNodeConfig& cfg);

using ArakoonNodeConfigs = std::vector<ArakoonNodeConfig>;

std::ostream&
operator<<(std::ostream& os,
           const ArakoonNodeConfigs& cfg);

}

BOOST_CLASS_VERSION(arakoon::ArakoonNodeConfig, 1);

namespace initialized_params
{

template<>
struct PropertyTreeVectorAccessor<arakoon::ArakoonNodeConfig>
{
    static const std::string node_id_key;
    static const std::string host_key;
    static const std::string port_key;

    using Type = arakoon::ArakoonNodeConfig;

    static Type
    get(const boost::property_tree::ptree& pt)
    {
        return Type(pt.get<arakoon::NodeID>(node_id_key),
                    pt.get<std::string>(host_key),
                    pt.get<uint16_t>(port_key));
    }

    static void
    put(boost::property_tree::ptree& pt,
        const Type& cfg)
    {
        pt.put(node_id_key, cfg.node_id_);
        pt.put(host_key, cfg.hostname_);
        pt.put(port_key, cfg.port_);
    }
};

}

#endif // ARAKOON_NODE_CONFIG_H_

// Local Variables: **
// mode: c++ **
// End: **
