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

#ifndef __NETWORK_HA_CONTEXT_H
#define __NETWORK_HA_CONTEXT_H

#include "volumedriver.h"
#include "common.h"
#include "context.h"
#include "IOThread.h"

#include <youtils/SpinLock.h>

#include <libxio.h>

#include <atomic>
#include <memory>
#include <queue>
#include <map>
#include <unordered_map>

namespace volumedriverfs
{

class NetworkHAContext : public ovs_context_t
{
public:
    NetworkHAContext(const std::string& uri,
                     uint64_t net_client_qdepth,
                     bool ha_enabled);

    ~NetworkHAContext();

    int
    open_volume(const char *volume_name,
                int oflag);

    void
    close_volume();

    int
    create_volume(const char *volume_name,
                  uint64_t size);

    int
    remove_volume(const char *volume_name);

    int
    truncate_volume(const char *volume_name,
                    uint64_t size);

    int
    truncate(uint64_t size);

    int
    snapshot_create(const char *volume_name,
                    const char *snapshot_name,
                    const uint64_t timeout);

    int
    snapshot_rollback(const char *volume_name,
                      const char *snapshot_name);

    int
    snapshot_remove(const char *volume_name,
                    const char *snapshot_name);

    void
    list_snapshots(std::vector<std::string>& snaps,
                   const char *volume_name,
                   uint64_t *size,
                   int *saved_errno);

    int
    is_snapshot_synced(const char *volume_name,
                       const char *snapshot_name);

    int
    list_volumes(std::vector<std::string>& volumes);

    int
    list_cluster_node_uri(std::vector<std::string>& uris);

    int
    send_read_request(struct ovs_aiocb *ovs_aiocbp,
                      ovs_aio_request *request);

    int
    send_write_request(struct ovs_aiocb *ovs_aiocbp,
                       ovs_aio_request *request);

    int
    send_flush_request(ovs_aio_request *request);

    int
    stat_volume(struct stat *st);

    ovs_buffer_t*
    allocate(size_t size);

    int
    deallocate(ovs_buffer_t *ptr);

    uint64_t
    assign_request_id(ovs_aio_request *request);

    void
    insert_inflight_request(uint64_t id,
                            ovs_aio_request *request);

    void
    remove_inflight_request(uint64_t id);

    bool
    is_ha_enabled() const
    {
        return ha_enabled_;
    }

    void
    set_connection_error()
    {
        if (is_ha_enabled())
        {
            connection_error_ = true;
        }
    }
private:
    std::atomic<ovs_context_t*> ctx_;
    std::string volume_name_;
    int oflag_;
    std::string uri_;
    uint64_t qd_;
    bool ha_enabled_;
    std::shared_ptr<xio_mempool> mpool;

    fungi::SpinLock inflight_reqs_lock_;
    std::unordered_map<uint64_t, ovs_aio_request*> inflight_ha_reqs_;

    std::atomic<uint64_t> request_id_;

    std::vector<std::string> cluster_nw_uris_;

    IOThread ha_ctx_thread_;

    bool opened_;
    bool openning_;
    bool connection_error_;

    ovs_context_t*
    atomic_get_ctx()
    {
        return std::atomic_load(&ctx_);
    }

    void
    atomic_xchg_ctx(ovs_context_t *ctx)
    {
        auto old_ctx = atomic_get_ctx();
        std::atomic_store(&ctx_, reinterpret_cast<ovs_context_t*>(ctx));
        delete old_ctx;
    }

    bool
    is_connection_error() const
    {
        return connection_error_;
    }

    void
    update_cluster_node_uri();

    void
    ha_ctx_handler(void *arg);

    void
    fail_inflight_requests(int errval);
};

} //namespace volumedriverfs

#endif //__NETWORK_HA_CONTEXT_H
