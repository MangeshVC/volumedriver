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

#ifndef PERFORMANCE_COUNTERS_H
#define PERFORMANCE_COUNTERS_H

#include <atomic>
#include <mutex>

#include <boost/serialization/nvp.hpp>

#include <youtils/Serialization.h>
#include <youtils/SpinLock.h>

namespace volumedriver
{

template<typename T>
class PerformanceCounter
{
public:
    using Type = T;

    PerformanceCounter()
        : events_(0)
        , sum_(0)
        , sqsum_(0)
        , min_(std::numeric_limits<T>::max())
        , max_(std::numeric_limits<T>::min())
    {}

    PerformanceCounter(const PerformanceCounter& other)
        : events_(other.events())
        , sum_(other.sum())
        , sqsum_(other.sum_of_squares())
        , min_(other.min())
        , max_(other.max())
    {}

    PerformanceCounter&
    operator=(const PerformanceCounter& other)
    {
        if (this != &other)
        {
            std::lock_guard<decltype(lock_)> g(lock_);
            std::lock_guard<decltype(lock_)> h(other.lock_);

            events_ = other.events_;
            sum_ = other.sum_;
            sqsum_ = other.sqsum_;
            min_ = other.min_;
            max_ = other.max_;
        }

        return *this;
    }

    PerformanceCounter&
    operator+=(const PerformanceCounter& other)
    {
        std::lock_guard<decltype(lock_)> g(lock_);
        if (this == &other)
        {
            events_ *= 2;
            sum_ *= 2;
            sqsum_ *= 2;
            return *this;
        }
        else
        {
            std::lock_guard<decltype(lock_)> h(other.lock_);
            events_ += other.events_;
            sum_ += other.sum_;
            sqsum_ += other.sqsum_;
            min_ = std::min<T>(min_,
                               other.min_);
            max_ = std::max<T>(max_,
                               other.max_);

            return *this;
        }
    }

    friend PerformanceCounter
    operator+(PerformanceCounter lhs,
              const PerformanceCounter& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    void
    count(const T t)
    {
        std::lock_guard<decltype(lock_)> g(lock_);

        events_++;
        sum_ += t;
        sqsum_ += t * t;
        if (min_ > t)
        {
            min_ = t;
        }
        if (max_ < t)
        {
            max_ = t;
        }
    }

    void
    reset()
    {
        std::lock_guard<decltype(lock_)> g(lock_);

        events_ = 0;
        sum_ = 0;
        sqsum_ = 0;
        min_ = std::numeric_limits<T>::max();
        max_ = std::numeric_limits<T>::min();
    }

    uint64_t
    events() const
    {
        std::lock_guard<decltype(lock_)> g(lock_);
        return events_;
    }

    T
    sum() const
    {
        std::lock_guard<decltype(lock_)> g(lock_);
        return sum_;
    }

    T
    sum_of_squares() const
    {
        std::lock_guard<decltype(lock_)> g(lock_);
        return sqsum_;
    }

    T
    min() const
    {
        std::lock_guard<decltype(lock_)> g(lock_);
        return min_;
    }

    T
    max() const
    {
        std::lock_guard<decltype(lock_)> g(lock_);
        return max_;
    }

    bool
    operator==(const PerformanceCounter<T>& other) const
    {
        return
            events() == other.events() and
            sum() == other.sum() and
            sum_of_squares() == other.sum_of_squares() and
            min() == other.min() and
            max() == other.max();
    }

private:
    mutable fungi::SpinLock lock_;
    uint64_t events_;
    T sum_;
    T sqsum_;
    T min_;
    T max_;

    friend class boost::serialization::access;
    BOOST_SERIALIZATION_SPLIT_MEMBER();

    template<typename Archive>
    void
    load(Archive& ar,
         const unsigned version)
    {
        std::lock_guard<decltype(lock_)> g(lock_);

        ar & boost::serialization::make_nvp("events",
                                            events_);
        ar & boost::serialization::make_nvp("sum",
                                            sum_);
        ar & boost::serialization::make_nvp("sqsum",
                                            sqsum_);
        if (version > 0)
        {
            ar & boost::serialization::make_nvp("min",
                                                min_);
            ar & boost::serialization::make_nvp("max",
                                                max_);
        }
    }

    template<typename Archive>
    void
    save(Archive& ar,
         const unsigned /* version */) const
    {
        const uint64_t evts = events();
        ar & boost::serialization::make_nvp("events",
                                            evts);
        const T s = sum();
        ar & boost::serialization::make_nvp("sum",
                                            s);

        const T sq = sum_of_squares();
        ar & boost::serialization::make_nvp("sqsum",
                                            sq);

        const T mn = min();
        ar & boost::serialization::make_nvp("min",
                                            mn);
        const T mx = max();
        ar & boost::serialization::make_nvp("max",
                                            mx);
    }
};

struct PerformanceCounters
{
    PerformanceCounter<uint64_t> write_request_size;
    PerformanceCounter<uint64_t> write_request_usecs;

    PerformanceCounter<uint64_t> unaligned_write_request_size;

    PerformanceCounter<uint64_t> backend_write_request_usecs;
    PerformanceCounter<uint64_t> backend_write_request_size;

    PerformanceCounter<uint64_t> read_request_size;
    PerformanceCounter<uint64_t> read_request_usecs;

    PerformanceCounter<uint64_t> unaligned_read_request_size;
    PerformanceCounter<uint64_t> backend_read_request_size;
    PerformanceCounter<uint64_t> backend_read_request_usecs;

    PerformanceCounter<uint64_t> sync_request_usecs;

    PerformanceCounters() = default;

    ~PerformanceCounters() = default;

    PerformanceCounters(const PerformanceCounters&) = default;

    PerformanceCounters&
    operator=(const PerformanceCounters&) = default;

    bool
    operator==(const PerformanceCounters& other) const
    {
#define EQ(x)                                   \
        x == other.x

        return
            EQ(write_request_size) and
            EQ(write_request_usecs) and
            EQ(unaligned_write_request_size) and
            EQ(backend_write_request_usecs) and
            EQ(backend_write_request_size) and
            EQ(read_request_size) and
            EQ(read_request_usecs) and
            EQ(unaligned_read_request_size) and
            EQ(backend_read_request_size) and
            EQ(backend_read_request_usecs) and
            EQ(sync_request_usecs);

#undef EQ
    }

    PerformanceCounters&
    operator+=(const PerformanceCounters& other)
    {
#define ADD(x)                                  \
        x += other.x

        ADD(write_request_size);
        ADD(write_request_usecs);
        ADD(unaligned_write_request_size);
        ADD(backend_write_request_usecs);
        ADD(backend_write_request_size);
        ADD(read_request_size);
        ADD(read_request_usecs);
        ADD(unaligned_read_request_size);
        ADD(backend_read_request_size);
        ADD(backend_read_request_usecs);
        ADD(sync_request_usecs);

        return *this;
#undef ADD
    }

    friend PerformanceCounters
    operator+(PerformanceCounters lhs,
              const PerformanceCounters& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    void
    reset_all_counters()
    {
        write_request_size.reset();
        write_request_usecs.reset();

        unaligned_write_request_size.reset();

        backend_write_request_usecs.reset();
        backend_write_request_size.reset();

        read_request_size.reset();
        read_request_usecs.reset();

        unaligned_read_request_size.reset();
        backend_read_request_size.reset();
        backend_read_request_usecs.reset();

        sync_request_usecs.reset();
    }

    template<typename Archive>
    void
    serialize(Archive& ar,
              const unsigned /* version */)
    {
#define S(x)                                   \
        ar & BOOST_SERIALIZATION_NVP(x)

        S(write_request_size);
        S(write_request_usecs);
        S(unaligned_write_request_size);
        S(backend_write_request_usecs);
        S(backend_write_request_size);
        S(read_request_size);
        S(read_request_usecs);
        S(unaligned_read_request_size);
        S(backend_read_request_size);
        S(backend_read_request_usecs);
        S(sync_request_usecs);

#undef S
    }
};

}

BOOST_CLASS_VERSION(volumedriver::PerformanceCounter<uint64_t>, 1);

BOOST_CLASS_VERSION(volumedriver::PerformanceCounters, 0);

#endif // PERFORMANCE_COUNTERS_H
