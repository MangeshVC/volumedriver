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

#include "Namespace.h"
#include <boost/algorithm/string.hpp>
#include <youtils/UUID.h>
namespace backend
{

namespace
{
bool
number_or_lowercase(const char c)
{
    return islower(c) or
        isdigit(c);
}

bool
number_lowercase_or_dash(const char c)
{
    return c == '-' or
        number_or_lowercase(c);
}
}

const std::string
Namespace::nvp_name("namespace");

Namespace::Namespace()
    : str_(youtils::UUID().str())
{}

Namespace::Namespace(std::string&& str)
    : str_(std::move(str))
{
    // http://docs.aws.amazon.com/AmazonS3/latest/dev/BucketRestrictions.html
    const uint64_t maxlen = 63;
    size_t len = str_.size();

    if (len < 3)
    {
        LOG_ERROR(str_ << ": bucket name too short (min: 3)");
        throw TooShortException("BucketNameTooShort");
    }
    if(len > maxlen)
    {
        LOG_ERROR(str_ << ": bucket name too long (max: " << maxlen << ")");
        throw TooLongException("BucketNameTooLong");
    }

    std::vector<std::string> split_strings;
    boost::split(split_strings, str_, boost::is_any_of("."));

    for(const std::string& label : split_strings)
    {
        if(label.empty())
        {
            LOG_ERROR(str_ << ": bucket label should not be empty");
            throw LabelTooShortException("BucketLabelTooShort");
        }
        if(not number_or_lowercase(label.front()))
        {
            LOG_ERROR(str_ << ": cannot have a " << label.front() <<
                      " character at the start of a label");
            throw LabelStartException("InvalidBucketLabelStart");
        }
        if(not number_or_lowercase(label.back()))
        {
            LOG_ERROR(str_ << ": cannot have a " << label.back() <<
                      " character at the end of a label");
            throw LabelEndException("InvalidBucketLabelEnd");
        }
        if(not boost::all(label, number_lowercase_or_dash))
        {
            LOG_ERROR(str_ << ": invalid characters in label " << label);
            throw LabelNameException("InvalidLabelName");
        }
    }
}

Namespace::Namespace(const std::string& str)
    : Namespace(std::string(str))
{}

// We don't need to go through a new validation normally only for the NamespaceSerializationWrapper.
Namespace::Namespace(const Namespace& ns)
    : Namespace(ns.str_)
{}

Namespace::Namespace(Namespace&& ns)
    : Namespace(std::move(ns.str_))
{}

Namespace&
Namespace::operator=(const Namespace& i_ns)
{
    if(this != &i_ns)
    {
        str_ = i_ns.str_;
    }
    return *this;

}

Namespace&
Namespace::operator=(Namespace&& i_ns)

{
    if(this != &i_ns)
    {
        str_ = std::move(i_ns.str_);
    }
    return *this;
}

const char*
Namespace::c_str() const
{
    return str_.c_str();
}
}
