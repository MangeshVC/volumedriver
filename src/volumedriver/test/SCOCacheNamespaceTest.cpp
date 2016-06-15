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

#include <youtils/Logging.h>

#include "ExGTest.h"
#include "../SCOCacheNamespace.h"

namespace volumedriver
{

TEST(SCOCacheNamespaceConstructorTest, constructor)
{
    const backend::Namespace nspace;
    uint64_t min = 1 << 20;
    uint64_t max = 1 << 30;

    {
        std::auto_ptr<SCOCacheNamespace> ns(new SCOCacheNamespace(nspace,
                                                                  max,
                                                                  min));

        EXPECT_EQ(nspace, ns->getName());
        EXPECT_EQ(max, ns->getMinSize());
        EXPECT_EQ(min, ns->getMaxNonDisposableSize());
    }
    {
        std::unique_ptr<SCOCacheNamespace> ns(new SCOCacheNamespace(nspace,
                                                                  min,
                                                                  max));

        EXPECT_EQ(nspace, ns->getName());
        EXPECT_EQ(min, ns->getMinSize());
        EXPECT_EQ(max, ns->getMaxNonDisposableSize());
    }
}

class SCOCacheNamespaceTest : public ExGTest
{
protected:
    virtual void
    SetUp()
    {
        min_ = 1 << 20;
        max_ = 1 << 30;
        ns_.reset(new SCOCacheNamespace(backend::Namespace(),
                                        min_,
                                        max_));
    }

    uint64_t min_;
    uint64_t max_;
    std::unique_ptr<SCOCacheNamespace> ns_;

private:
    DECLARE_LOGGER("SCOCacheNamespaceTest");
};

TEST_F(SCOCacheNamespaceTest, lookup)
{
    EXPECT_TRUE(ns_->empty());

    SCOCacheNamespaceEntry* e = ns_->findEntry(SCO(1));
    EXPECT_EQ(0, e);

    EXPECT_THROW(e = ns_->findEntry_throw(SCO(2)),
                 fungi::IOException);


    CachedSCOPtr sco(0);
    SCOCacheNamespaceEntry e2(sco, true);
    std::pair<SCOCacheNamespace::iterator, bool> res =
        ns_->insert(std::make_pair(SCO(3), e2));

    EXPECT_TRUE(res.first != ns_->end());
    EXPECT_TRUE(res.second);

    e = ns_->findEntry(SCO(3));
    EXPECT_TRUE(e->isBlocked());
    EXPECT_EQ(sco, e->getSCO());
    e->setBlocked(false);

    e = 0;
    e = ns_->findEntry_throw(SCO(3));
    EXPECT_FALSE(e->isBlocked());
    EXPECT_EQ(sco, e->getSCO());
}

TEST_F(SCOCacheNamespaceTest, updateLimits)
{
    uint64_t min = ns_->getMinSize();
    uint64_t max = ns_->getMaxNonDisposableSize();

    EXPECT_NO_THROW(ns_->setLimits(max, min));

    min = 0;
    max = std::numeric_limits<uint64_t>::max();

    EXPECT_LT(min, min_);
    EXPECT_GT(max, max_);

    ns_->setLimits(min, max);

    EXPECT_EQ(min, ns_->getMinSize());
    EXPECT_EQ(max, ns_->getMaxNonDisposableSize());
}
}
// Local Variables: **
// mode: c++ **
// End: **
