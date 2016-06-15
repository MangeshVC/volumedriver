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

#ifndef YT_ALTERNATIVE_OPTION_AGAIN_H_
#define YT_ALTERNATIVE_OPTION_AGAIN_H_
#include <string>
#include <vector>
#include <loki/Typelist.h>

#include <memory>
#include <ostream>

namespace boost
{
namespace program_options
{
class options_description;
}
}

namespace youtils
{
template<typename T>
struct AlternativeOptionTraits;

template<typename T>
struct AltOptionInterface
{
    typedef T option_type;

    virtual ~AltOptionInterface() = default;

    virtual void
    actions() = 0;

    virtual const boost::program_options::options_description&
    options_description() const = 0;

};



template<typename T,
         T value>
struct AlternativeOptionAgain : public AltOptionInterface<T>
{
    explicit AlternativeOptionAgain()
    {
        static_assert(std::is_enum<T>::value,"Only works with enums");
    }

    AlternativeOptionAgain(const AlternativeOptionAgain&) = delete;

    AlternativeOptionAgain&
    operator=(const AlternativeOptionAgain&) = delete;

    static bool
    is_ok(const std::string& in)
    {
        return AlternativeOptionTraits<T>::str_.at(value) == in;
    }

    static const char*
    id_()
    {
        return AlternativeOptionTraits<T>::str_.at(value);
    }

};

template<typename T>
struct make_alt_option_interface;


template<typename Options, Options option>
struct make_alt_option_interface<AlternativeOptionAgain<Options, option> >
{
    typedef AlternativeOptionAgain<Options, option> option_type;
};


template <class TList>
struct print_doc;

template<typename Head, typename Tail>
struct print_doc< ::Loki::Typelist<Head, Tail> >
{
    static void
    go(std::ostream& strm)
    {
        strm << Head().options_description() << std::endl;
        print_doc<Tail>::go(strm);
    }
};

template<>
struct print_doc< ::Loki::NullType >
{
    static void
    go(std::ostream& /*strm*/)
    {
    }
};



template <class TList>
struct GetOption;

template<typename Head, typename Tail>
struct GetOption< ::Loki::Typelist<Head, Tail> >
{
    static void*
    get(const std::string& arg)
    {
        if (Head::is_ok(arg))
        {
            return new Head();
        }
        else
        {
            return GetOption<Tail>::get(arg);
        }
    }
};


template<>
struct GetOption<  ::Loki::NullType >
{
    static void*
    get(const std::string& /*arg*/)
    {
        return nullptr;
    }
};

template<typename T,
         typename List>
std::unique_ptr<AltOptionInterface<T> >
get_option(const std::string& str)
{
    std::unique_ptr<AltOptionInterface<T > > val
        (reinterpret_cast<AltOptionInterface<T> *>(GetOption<List>::get(str)));
    if(not val)
    {
        throw std::out_of_range("No such option" + str);
    }
    else
    {
        return val;
    }
}

}


#endif // YT_ALTERNATIVE_OPTION_AGAIN_H_
