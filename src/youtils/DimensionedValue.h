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

#ifndef _PREFIX_H_
#define _PREFIX_H_

#include "Logging.h"
#include <loki/Typelist.h>

#include <exception>
#include <iosfwd>
#include <string>
#include <utility>

namespace youtils
{

enum class Prefix
{
  B,
  KB,
  MB,
  GB,
  TB,
  PB,
  KiB,
  MiB,
  GiB,
  TiB,
  PiB,
  NotValid,
  Unlimited
};

template<Prefix p>
struct PT;

#define DPT(val,size)                                    \
  template<>                                             \
  struct PT<val>                                         \
  {                                                      \
    const static Prefix prefix = val;                    \
    static const char* name;                             \
    static const unsigned long long multiplier = size;   \
  };

DPT(Prefix::B,1)
DPT(Prefix::KB,1000ULL)
DPT(Prefix::MB,1000ULL*1000ULL)
DPT(Prefix::GB,1000ULL*1000ULL*1000ULL)
DPT(Prefix::TB,1000ULL*1000ULL*1000ULL*1000ULL)
DPT(Prefix::PB,1000ULL*1000ULL*1000ULL*1000ULL*1000ULL)
DPT(Prefix::KiB,1024ULL)
DPT(Prefix::MiB,1024ULL*1024ULL)
DPT(Prefix::GiB,1024ULL*1024ULL*1024ULL)
DPT(Prefix::TiB,1024ULL*1024ULL*1024ULL*1024ULL)
DPT(Prefix::PiB,1024ULL*1024ULL*1024ULL*1024ULL*1024ULL)

#undef DPT

typedef LOKI_TYPELIST_11(PT<Prefix::B>,
                         PT<Prefix::KB>,
                         PT<Prefix::MB>,
                         PT<Prefix::GB>,
                         PT<Prefix::TB>,
                         PT<Prefix::PB>,
                         PT<Prefix::KiB>,
                         PT<Prefix::MiB>,
                         PT<Prefix::GiB>,
                         PT<Prefix::TiB>,
                         PT<Prefix::PiB>) types;

template<typename T>
struct FindPrefix;

template<>
struct FindPrefix<Loki::NullType>
{
    static bool
    name(const std::string& /*name*/,
         Prefix& /*o_pref*/,
         unsigned long long & /*o_mult*/)
    {
        return false;
    }
};

template<typename T1, typename T2>
struct FindPrefix<Loki::Typelist<T1, T2> >
{

    static bool
    name(const std::string& nm,
         Prefix& o_pref,
         unsigned long long & o_mult)
    {
      if (nm == T1::name)
      {
        o_pref = T1::prefix;
        o_mult = T1::multiplier;
        return true;

      }
      else
      {
        return FindPrefix<T2>::name(nm,
                                    o_pref,
                                    o_mult);
      }
    }
};


template<typename T>
struct FindName;

template<typename T1, typename T2>
struct FindName<Loki::Typelist<T1, T2> >
{
    static const char*
    getString(Prefix prefix)
    {
        if(prefix == T1::prefix)
        {
            return T1::name;
        }
        else
        {
            return FindName<T2>::getString(prefix);
        }

    }
};

template<>
struct FindName<Loki::NullType>
{
    static const char*
    getString(Prefix /*prefix*/)
    {
        throw "No Such Prefix";
    }
};

template<typename T1>
struct FindBestPrefix;

template<typename T1, typename T2>
struct FindBestPrefix<Loki::Typelist<T1, T2> >
{
    static std::pair<Prefix, uint64_t>
    fbp(uint64_t val, Prefix pref, uint64_t multiplier)
    {
        if((val % T1::multiplier) == 0 and
           (val / T1::multiplier) < (val / multiplier))
        {
            return FindBestPrefix<T2>::fbp(val, T1::prefix, T1::multiplier);
        }
        else
        {
            return FindBestPrefix<T2>::fbp(val, pref, multiplier);
        }
    }

    static std::pair<Prefix, uint64_t>
    fbp(uint64_t val)
    {
        return fbp(val, Prefix::B, 1);
    }
};


template<>
struct FindBestPrefix<Loki::NullType>
{
    static std::pair<Prefix,uint64_t>
    fbp(uint64_t, Prefix pref, uint64_t multiplier)
    {
       return std::make_pair(pref, multiplier);
    }
};

class DimensionedValue
{
public:
    explicit DimensionedValue(const std::string& i);

    explicit DimensionedValue(unsigned long long bytes);

    DimensionedValue();

    ~DimensionedValue() = default;

    DimensionedValue(const DimensionedValue&) = default;

    DimensionedValue&
    operator=(const DimensionedValue&) = default;

    bool
    operator==(const DimensionedValue& other) const;

    bool
    operator!=(const DimensionedValue& other) const
    {
        return not operator==(other);
    }

    unsigned long long
    getBytes() const;

    const char*
    getString(Prefix p) const
    {
        return FindName<types>::getString(p);
    }

    std::string
    toString() const;

private:
    DECLARE_LOGGER("DimensionedValue");

    Prefix prefix_;
    unsigned long long multiplier_;
    uint64_t value_;
};

inline std::ostream&
operator<<(std::ostream& os, const DimensionedValue& v)
{
    return os << v.toString();
}

std::istream&
operator>>(std::istream& is,
           DimensionedValue& v);

class NoParse
    : public std::exception
{
  public:
    NoParse(const std::string& str) throw()
    : str_(str)
    {}

    ~NoParse() throw()
    {}

    // Y42: This is *really* bad (and by my hand)
    virtual const char* what() const throw()
    {
      return (std::string("Could not parse: ") + str_).c_str();
    }
  private:
    std::string str_;
};
}

#endif // _PREFIX_H_

// Local Variables: **
// mode: c++ **
// End: **
