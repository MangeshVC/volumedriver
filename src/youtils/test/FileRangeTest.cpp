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

#include <gtest/gtest.h>

#include <boost/iostreams/stream.hpp>

#include "../FileRange.h"
#include "../FileDescriptor.h"
#include "../FileUtils.h"

namespace youtilstest
{
using namespace youtils;

class FileRangeTest
    : public testing::Test
{
public:
    FileRangeTest()
        : dir_(FileUtils::temp_path("FileRangeTest"))
    {}

    virtual void
    SetUp()
    {
        fs::remove_all(dir_);
        fs::create_directories(dir_);
        const fs::path p(dir_ / "testfile");
        test_file_.reset(new file_type(p, FDMode::ReadWrite, CreateIfNecessary::T));
        test_file_->fallocate(1ULL << 20);
    }

    virtual void
    TearDown()
    {
        fs::remove_all(dir_);
    }

    using file_type = FileDescriptor;

protected:

    const fs::path dir_;
    std::shared_ptr<file_type> test_file_;
};

typedef FileRange<FileRangeTest::file_type> file_range_type;

TEST_F(FileRangeTest, construction)
{
    const uint64_t size = test_file_->size();
    EXPECT_LT(0UL, size);

    EXPECT_THROW(file_range_type(test_file_,
                                 0,
                                 size + 1),
                 FileRangeViolation);

    EXPECT_THROW(file_range_type(test_file_,
                                 size,
                                 1),
                 FileRangeViolation);

    EXPECT_LT(0U, size / 3);
    file_range_type r(test_file_,
                      size / 3,
                      size / 3);
}

TEST_F(FileRangeTest, basics)
{
    const size_t size = test_file_->size();
    EXPECT_LT(0U, size);

    const uint8_t segments = 17;
    const size_t seg_size = size / segments;
    EXPECT_LT(0U, seg_size);

    for (uint8_t i = 0; i < segments; ++ i)
    {
        const std::vector<uint8_t> b(seg_size, i);
        file_range_type r(test_file_,
                          i * seg_size,
                          seg_size);
        EXPECT_EQ(static_cast<ssize_t>(seg_size),
                  r.pwrite(&b[0],
                           b.size(),
                           0));
    }

    for (uint8_t i = 0; i < segments; ++i)
    {
        const std::vector<uint8_t> ref(seg_size, i);
        std::vector<uint8_t> b(seg_size);

        file_range_type r(test_file_,
                          i * seg_size,
                          seg_size);

        EXPECT_EQ(static_cast<ssize_t>(seg_size),
                  r.pread(&b[0],
                          b.size(),
                          0));
        EXPECT_EQ(0, memcmp(&ref[0], &b[0], ref.size()));

        EXPECT_EQ(seg_size, test_file_->pread(&b[0],
                                              b.size(),
                                              seg_size * i));
        EXPECT_EQ(0, memcmp(&ref[0], &b[0], ref.size()));
    }
}

TEST_F(FileRangeTest, boundaries)
{
    const size_t size = test_file_->size();
    EXPECT_LT(0U, size);

    const uint8_t segments = 7;
    const size_t seg_size = size / segments;
    EXPECT_LT(0U, seg_size);

    const std::vector<uint8_t> ref(seg_size, segments);

    for (uint8_t i = 0; i < segments; ++i)
    {
        EXPECT_EQ(ref.size(), test_file_->pwrite(&ref[0],
                                                 ref.size(),
                                                 i * ref.size()));

        file_range_type r(test_file_,
                          seg_size * i,
                          seg_size);

        const std::vector<uint8_t> wbuf(seg_size + 1, i);
        // a bit (byte) too big
        EXPECT_THROW(r.pwrite(&wbuf[0], wbuf.size(), 0),
                     FileRangeViolation);

        // a bit (byte) off
        EXPECT_THROW(r.pwrite(&wbuf[0], seg_size, 1),
                     FileRangeViolation);

        // way off
        EXPECT_THROW(r.pwrite(&wbuf[0], seg_size, seg_size),
                     FileRangeViolation);

        EXPECT_EQ(0, r.pwrite(&wbuf[0], 0, 0));
        EXPECT_EQ(0, r.pwrite(&wbuf[0], 0, seg_size));

        std::vector<uint8_t> rbuf(seg_size);

        EXPECT_EQ(static_cast<ssize_t>(seg_size),
                  r.pread(&rbuf[0], rbuf.size(), 0));
        EXPECT_EQ(0, memcmp(&ref[0], &rbuf[0], ref.size()));

        EXPECT_EQ(seg_size,
                  test_file_->pread(&rbuf[0], rbuf.size(), seg_size * i));
        EXPECT_EQ(0, memcmp(&ref[0], &rbuf[0], ref.size()));
    }
}

TEST_F(FileRangeTest, streaming)
{
    const ssize_t size = test_file_->size();
    EXPECT_LT(0, size);

    const uint8_t segments = 2;
    const ssize_t seg_size = size / segments;

    EXPECT_LT(0, seg_size);

    const ssize_t iosize = 4096;
    EXPECT_EQ(0, seg_size % iosize);
    EXPECT_LT(iosize, seg_size);

    for (uint8_t i = 0; i < segments; ++i)
    {
        file_range_type r(test_file_,
                          i * seg_size,
                          iosize);

        bio::stream<file_range_type> io(file_range_type(test_file_,
                                                        i * seg_size,
                                                        seg_size),
                                        4096);
        for (ssize_t j = 0; j < seg_size / iosize; ++j)
        {
            const std::vector<uint8_t> wbuf(4096, i + j);
            io.write(reinterpret_cast<const char*>(&wbuf[0]), wbuf.size());
            EXPECT_TRUE(io.good());
        }
    }

    for (uint8_t i = 0; i < segments; ++i)
    {
        file_range_type r(test_file_,
                          i * seg_size,
                          iosize);

        bio::stream<file_range_type> io(file_range_type(test_file_,
                                                        i * seg_size,
                                                        seg_size),
                                        4096);
        for (ssize_t j = 0; j < seg_size / iosize; ++j)
        {
            const std::vector<uint8_t> ref(iosize, i + j);
            std::vector<uint8_t> rbuf(iosize);

            io.read(reinterpret_cast<char*>(&rbuf[0]), rbuf.size());
            EXPECT_TRUE(io.good());
            EXPECT_EQ(0, memcmp(&ref[0], &rbuf[0], ref.size()));
        }
    }
}

}

// Local Variables: **
// mode: c++ **
// End: **
