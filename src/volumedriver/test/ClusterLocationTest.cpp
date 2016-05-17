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

#include "ExGTest.h"
#include "../ClusterLocation.h"

namespace volumedrivertest
{
using namespace volumedriver;

class ClusterLocationTest
    : public ExGTest
{
public:
    ClusterLocation
    randomClusterLocation()
    {
        SCONumber num = ((std::numeric_limits<SCONumber>::max() - 1) * drand48()) + 1;
        SCOOffset offset = std::numeric_limits<SCOOffset>::max() * drand48();
        SCOVersion version = SCOVersion(std::numeric_limits<SCOVersion>::max() * drand48());
        SCOCloneID cloneid = SCOCloneID(std::numeric_limits<SCOCloneID>::max() * drand48());
        return ClusterLocation(num,
                               offset,
                               cloneid,
                               version);
    }
    SCO
    randomSCO()
    {
        SCONumber num = ((std::numeric_limits<SCONumber>::max() - 1) * drand48()) + 1;
        SCOVersion version = SCOVersion(std::numeric_limits<SCOVersion>::max() * drand48());
        SCOCloneID cloneid = SCOCloneID(std::numeric_limits<SCOCloneID>::max() * drand48());
        return SCO(num,
                   cloneid,
                   version);
    }
};

TEST_F(ClusterLocationTest, SCONameTest1)
{
    ASSERT_EQ(sizeof(ClusterLocation), sizeof(SCO) + sizeof(SCOOffset));
    ASSERT_EQ(6U, sizeof(SCO));
    ASSERT_EQ(8U, sizeof(ClusterLocation));

    ClusterLocation loc;
    // Check for unavailability of these operators
    // ASSERT_FALSE(    loc < loc);

    SCO sco;
    ASSERT_EQ(loc.cloneID(), sco.cloneID());
    ASSERT_EQ(loc.number(), sco.number());
    ASSERT_EQ(loc.version(), sco.version());
    ASSERT_FALSE(sco.asBool());
    ASSERT_TRUE(loc.isNull());

    for(unsigned i = 0; i < 100000; ++i)
    {
        ClusterLocation loc = randomClusterLocation();
        SCO sco = loc.sco();
        ASSERT_EQ(loc.cloneID(), sco.cloneID());
        ASSERT_EQ(loc.number(), sco.number());
        ASSERT_EQ(loc.version(), sco.version());
        ASSERT_TRUE(sco.asBool() xor loc.isNull());

        SCOOffset offset = loc.offset();
        ClusterLocation loc2(sco,
                            offset);
        ASSERT_EQ(loc2, loc);

        SCO sco2(loc.number(),
                 loc.cloneID(),
                 loc.version());

        ASSERT_EQ(sco, sco2);
        ASSERT_TRUE(not (sco.asBool() xor sco2.asBool()));

        std::string tmp = sco.str();
        ASSERT_TRUE(SCO::isSCOString(tmp));
        ASSERT_EQ(SCO(tmp), sco);

        {
            const SCONumber i = drand48() * std::numeric_limits<SCONumber>::max();

            loc.incrementNumber(i);
            ASSERT_EQ(loc.cloneID(), sco.cloneID());
            if(i)
                ASSERT_NE(loc.number(), sco.number());
            else
                ASSERT_EQ(loc.number(), sco.number());

            ASSERT_EQ(loc.version(), sco.version());
            ASSERT_EQ(loc.offset(), offset);

            loc.decrementNumber(i);
            ASSERT_EQ(loc.cloneID(), sco.cloneID());
            ASSERT_EQ(loc.number(), sco.number());
            ASSERT_EQ(loc.version(), sco.version());
            ASSERT_EQ(loc.offset(), offset);

            sco.incrementNumber(i);
            ASSERT_EQ(loc.cloneID(), sco.cloneID());
            if(i)
                ASSERT_NE(loc.number(), sco.number());
            else
                ASSERT_EQ(loc.number(), sco.number());

            ASSERT_EQ(loc.version(), sco.version());
            ASSERT_EQ(loc.offset(), offset);

            sco.decrementNumber(i);
            ASSERT_EQ(loc.cloneID(), sco.cloneID());
            ASSERT_EQ(loc.number(), sco.number());
            ASSERT_EQ(loc.version(), sco.version());
            ASSERT_EQ(loc.offset(), offset);
        }
        {
            const SCOVersion i(drand48() * std::numeric_limits<SCOVersion>::max());
            loc.incrementVersion(i);
            ASSERT_EQ(loc.cloneID(), sco.cloneID());
            ASSERT_EQ(loc.number(), sco.number());
            if(i)
                ASSERT_NE(loc.version(), sco.version());
            else
                ASSERT_EQ(loc.version(), sco.version());


            ASSERT_EQ(loc.offset(), offset);

            loc.decrementVersion(i);
            ASSERT_EQ(loc.cloneID(), sco.cloneID());
            ASSERT_EQ(loc.number(), sco.number());
            ASSERT_EQ(loc.version(), sco.version());
            ASSERT_EQ(loc.offset(), offset);

            sco.incrementVersion(i);
            ASSERT_EQ(loc.cloneID(), sco.cloneID());
            ASSERT_EQ(loc.number(), sco.number());
            if(i)
                ASSERT_NE(loc.version(), sco.version());
            else
                ASSERT_EQ(loc.version(), sco.version());

            ASSERT_EQ(loc.offset(), offset);

            sco.decrementVersion(i);
            ASSERT_EQ(loc.cloneID(), sco.cloneID());
            ASSERT_EQ(loc.number(), sco.number());
            ASSERT_EQ(loc.version(), sco.version());
            ASSERT_EQ(loc.offset(), offset);
        }
        {
            const SCOCloneID i(drand48() * std::numeric_limits<SCOCloneID>::max());
            loc.incrementCloneID(i);
            if(i)
                ASSERT_NE(loc.cloneID(), sco.cloneID());
            else
                ASSERT_EQ(loc.cloneID(), sco.cloneID());

            ASSERT_EQ(loc.number(), sco.number());
            ASSERT_EQ(loc.version(), sco.version());
            ASSERT_EQ(loc.offset(), offset);

            loc.decrementCloneID(i);
            ASSERT_EQ(loc.cloneID(), sco.cloneID());
            ASSERT_EQ(loc.number(), sco.number());
            ASSERT_EQ(loc.version(), sco.version());
            ASSERT_EQ(loc.offset(), offset);

            sco.incrementCloneID(i);
            if(i)
                ASSERT_NE(loc.cloneID(), sco.cloneID());
            else
                ASSERT_EQ(loc.cloneID(), sco.cloneID());

            ASSERT_EQ(loc.number(), sco.number());
            ASSERT_EQ(loc.version(), sco.version());
            ASSERT_EQ(loc.offset(), offset);

            sco.decrementCloneID(i);
            ASSERT_EQ(loc.cloneID(), sco.cloneID());
            ASSERT_EQ(loc.number(), sco.number());
            ASSERT_EQ(loc.version(), sco.version());
            ASSERT_EQ(loc.offset(), offset);
        }
    }
}

TEST_F(ClusterLocationTest, stream_out)
{
    const SCO sco(SCONumber(1),
                  SCOCloneID(2),
                  SCOVersion(3));

    const SCOOffset off(4);

    const ClusterLocation loc(sco, off);

    std::stringstream ss;
    ss << std::oct << std::setfill('X');
    ss << loc;

    EXPECT_EQ('X', ss.fill());
    EXPECT_TRUE(ss.flags() bitand std::ios::oct);
    EXPECT_TRUE(ClusterLocation::isClusterLocationString(ss.str(), true));

    const ClusterLocation loc2(ss.str());
    EXPECT_EQ(loc, loc2);
}

TEST_F(ClusterLocationTest, stringified)
{
    const SCO sco(SCONumber(1),
                  SCOCloneID(2),
                  SCOVersion(3));

    const SCOOffset off(4);

    const ClusterLocation loc(sco, off);

    const std::string locstr(boost::lexical_cast<std::string>(loc));

    EXPECT_TRUE(ClusterLocation::isClusterLocationString(locstr, true));
    EXPECT_FALSE(ClusterLocation::isClusterLocationString(locstr.substr(1), true));
    EXPECT_FALSE(ClusterLocation::isClusterLocationString(locstr.substr(0,
                                                                        locstr.length() - 1),
                                                          true));
}

TEST_F(ClusterLocationTest, literals)
{
    SCOCloneID t(250);
    ASSERT_TRUE(250_scocloneid == t);
}

}

// Local Variables: **
// mode: c++ **
// End: **
