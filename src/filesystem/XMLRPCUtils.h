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

#ifndef VFS_XMLRPC_UTILS_H_
#define VFS_XMLRPC_UTILS_H_

#include <xmlrpc++0.7/src/XmlRpc.h>

#include <youtils/IOException.h>

namespace volumedriverfs
{

struct XMLRPCUtils
{
    DECLARE_LOGGER("XMLRPCUtils");

    static void
    ensure_arg(::XmlRpc::XmlRpcValue& param,
               const std::string& name,
               bool asString = true);

    // the python client (used to) stringify everything.
    template<typename BooleanEnum>
    static BooleanEnum
    get_boolean_enum(::XmlRpc::XmlRpcValue& params,
                     const std::string& name)
    {
        ensure_arg(params, name);
        XmlRpc::XmlRpcValue param = params[name];

        std::string bl(param);
        if (bl == "true" || bl == "True")
        {
            return BooleanEnum::T;
        }
        else if(bl == "false" || bl == "False")
        {
            return BooleanEnum::F;
        }
        else
        {
            throw fungi::IOException(("Argument " + bl +
                                      " should be [Tt]rue or [Ff]alse").c_str());
        }
    }

    static void
    put(::XmlRpc::XmlRpcValue& params,
        const std::string& key,
        const bool b)
    {
        params[key] = b ? "true" : "false";
    };

    template<typename BE>
    static void
    put(::XmlRpc::XmlRpcValue& params,
        const std::string& key,
        const BE b)
    {
        // a bit of a hack.
        static_assert(sizeof(b) == sizeof(bool),
                      "this is only intended for boolean enums!");
        return put_bool(b == BE::T);
    };

};

}

#endif // !VFS_XMLRPC_UTILS_H_
