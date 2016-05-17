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

#ifndef VD_BACKEND_TASKS_H_
#define VD_BACKEND_TASKS_H_

#include "DataStoreCallBack.h"
#include "SCO.h"
#include "VolumeInterface.h"
#include "VolumeThreadPool.h"

#include <youtils/CheckSum.h>
#include <youtils/FileUtils.h>
#include <youtils/ThreadPool.h>
#include <youtils/UUID.h>

#include <backend/BackendInterface.h>

namespace volumedriver
{

using youtils::CheckSum;

class Volume;

namespace backend_task
{

typedef volumedriver::VolPoolTask TaskType;
typedef TaskType::Producer_t ProducerType;

//template<uint64_t i>
class TaskBase
    : public TaskType
{
public:
    TaskBase(VolumeInterface* vol,
             const youtils::BarrierTask barrier)
        : Task(barrier),
          volume_(vol)
    {};

    virtual const ProducerType&
    getProducerID() const final
    {
        return volume_;
    }
protected:
    VolumeInterface* volume_;
};

class Barrier final
    : public TaskBase
{
public:
    Barrier(VolumeInterface* vol)
        : TaskBase(vol,
                   youtils::BarrierTask::T)
    {}

    virtual const std::string&
    getName() const override
    {
        static const std::string str("barrier");
        return str;
    }

    virtual void
    run(int) override
    {}

};

class WriteSCO final
    : public TaskBase
{
public:
    WriteSCO(VolumeInterface *,
             DataStoreCallBack* cb,
             SCO sco,
             const CheckSum& cs,
             const OverwriteObject overwrite);

    virtual const
    std::string & getName() const override;

    virtual void
    run(int threadid) override;

    const fs::path
    getSource() const;

private:
    DECLARE_LOGGER("WriteSCOTask");

    SCO sco_;
    DataStoreCallBack* cb_;
    const CheckSum cs_;
    const OverwriteObject overwrite_;
};

class WriteTLog final
    : public TaskBase
{
public:
    WriteTLog(VolumeInterface*,
              const fs::path & source,
              const TLogId& tlogid,
              const SCO sconame,
              const CheckSum& checksum);

    virtual const std::string&
    getName() const override;

    virtual void
    run(int threadid) override;

private:
    DECLARE_LOGGER("WriteTLogTask");

    const fs::path tlogpath_; // just an alias for path_
    const TLogId tlogid_;
    const SCO sconame_;
    const CheckSum checksum_;
};

class DeleteTLog final
    : public TaskBase
{
public:
    // Does not use TLogId but std::string as
    // it's also used to remove relocation logs
    // ... to be revisited!
    DeleteTLog(VolumeInterface*,
               const std::string& tlog);

    virtual const std::string&
    getName() const override;

    virtual void
    run(int threadId) override;

private:
    DECLARE_LOGGER("DeleteTLogTask");

    const std::string tlog_;
};


class BlockDeleteTLogs final
    : public TaskBase
{
    typedef const std::vector<std::string> VectorType;

public:
    BlockDeleteTLogs(VolumeInterface*,
                     VectorType& source);

    virtual const std::string&
    getName() const override;

    virtual void
    run(int threadId) override;

    DECLARE_LOGGER("BlockDeleteTLogsTask");

private:
    VectorType sources_;
};

class WriteSnapshot final
    : public TaskBase
{
public:
    WriteSnapshot(VolumeInterface*);

    virtual void
    run(int threadId) override;

    virtual const std::string&
    getName() const override;

private:
    DECLARE_LOGGER("WriteSnapshotTask")
};

class BlockDeleteSCOS final
    : public TaskBase
{
    typedef const std::vector<SCO> VectorType;
public:
    BlockDeleteSCOS(VolumeInterface*,
                    VectorType& source,
                    const youtils::BarrierTask = youtils::BarrierTask::F);


    virtual const std::string&
    getName() const override;

    virtual void
    run(int threadId) override;

private:
    DECLARE_LOGGER("BlockDeleteSCOSTask");

    VectorType sources_;
};

class DeleteSCO final
    : public TaskBase
{
public:
    DeleteSCO(VolumeInterface*,
              const SCO sco,
              const youtils::BarrierTask = youtils::BarrierTask::F);

    virtual const std::string&
    getName() const override;

    virtual void
    run(int threadId) override;

    DECLARE_LOGGER("DeleteSCOTask");

private:
    const SCO sco_;
};

class FunTask final
    : public TaskBase
{
public:
    using Fun = std::function<void()>;

    FunTask(VolumeInterface& vol,
            Fun&& fun,
            const youtils::BarrierTask barrier = youtils::BarrierTask::F)
        : TaskBase(&vol,
                   barrier)
        , fun_(fun)
    {}

    virtual const std::string&
    getName() const override
    {
        static const std::string n("volumedriver::backend_task::FunTask");
        return n;
    }

    virtual void
    run(int /* thread_id */) override
    {
        fun_();
    }

private:
    DECLARE_LOGGER("FunTask");

    Fun fun_;
};

}

}

#endif /* VD_BACKEND_TASKS_H_ */

// Local Variables: **
// mode: c++ **
// End: **
