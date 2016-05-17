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
#ifndef __SHM_CONTROL_CHANNEL_CLIENT_H
#define __SHM_CONTROL_CHANNEL_CLIENT_H

#include "../ShmControlChannelProtocol.h"

#include <memory>
#include <boost/asio.hpp>
#include <youtils/SpinLock.h>

class ShmControlChannelClient
{
public:
    ShmControlChannelClient()
    : socket_(io_service_)
    , ep_(ShmConnectionDetails::Endpoint())
    {}

    ~ShmControlChannelClient()
    {
        socket_.shutdown(boost::asio::local::stream_protocol::socket::shutdown_both);
        socket_.close();
    };

    ShmControlChannelClient(const ShmControlChannelClient&) = delete;
    ShmControlChannelClient& operator=(const ShmControlChannelClient&) = delete;

    bool
    connect_and_register(const std::string& volume_name,
                         const std::string& key)
    {
        boost::system::error_code ec;
        ShmControlChannelMsg msg(ShmMsgOpcode::Register);
        msg.volume_name(volume_name);
        msg.key(key);

        socket_.connect(ep_,
                        ec);
        if (ec)
        {
            return false;
        }

        if (not _ctl_sendmsg(msg))
        {
            return false;
        }
        if (not _ctl_recvmsg(msg))
        {
            return false;
        }
        if (msg.is_success())
        {
            return true;
        }
        return false;
    }

    bool
    deregister()
    {
        ShmControlChannelMsg msg(ShmMsgOpcode::Deregister);

        fungi::ScopedSpinLock lock_(ctl_channel_lock_);
        if (not _ctl_sendmsg(msg))
        {
            return false;
        }
        if (not _ctl_recvmsg(msg))
        {
            return false;
        }
        if (msg.is_success())
        {
            return true;
        }
        return false;
    }

    bool
    allocate(boost::interprocess::managed_shared_memory::handle_t& hdl,
             size_t size)
    {
        ShmControlChannelMsg msg(ShmMsgOpcode::Allocate);
        msg.size(size);
        {
            fungi::ScopedSpinLock lock_(ctl_channel_lock_);
            if (not _ctl_sendmsg(msg))
            {
                return false;
            }
            if (not _ctl_recvmsg(msg))
            {
                return false;
            }
        }
        if (msg.is_success())
        {
            hdl = msg.handle();
            return true;
        }
        return false;
    }

    bool
    deallocate(const boost::interprocess::managed_shared_memory::handle_t& hdl)
    {
        ShmControlChannelMsg msg(ShmMsgOpcode::Deallocate);
        msg.handle(hdl);
        {
            fungi::ScopedSpinLock lock_(ctl_channel_lock_);
            if (not _ctl_sendmsg(msg))
            {
                return false;
            }
            if (not _ctl_recvmsg(msg))
            {
                return false;
            }
        }
        if (msg.is_success())
        {
            return true;
        }
        return false;
    }

    bool
    is_open() const
    {
        return socket_.is_open();
    }

    bool
    is_connected()
    {
        char buf;
        ssize_t ret = recv(socket_.native_handle(),
                           &buf,
                           1,
                           MSG_DONTWAIT | MSG_PEEK);
        if (ret == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

private:
    boost::asio::io_service io_service_;
    boost::asio::local::stream_protocol::socket socket_;
    boost::asio::local::stream_protocol::endpoint ep_;
    fungi::SpinLock ctl_channel_lock_;

    bool
    _ctl_sendmsg(const ShmControlChannelMsg& msg)
    {
        const std::string msg_str(msg.pack_msg());
        uint64_t msg_payload = msg_str.length();

        try
        {
            boost::asio::write(socket_,
                               boost::asio::buffer(&msg_payload,
                                                   shm_msg_header_size));
            boost::asio::write(socket_,
                               boost::asio::buffer(msg_str,
                                                   msg_payload));
        }
        catch(boost::system::system_error&)
        {
            return false;
        }
        return true;
    }

    bool
    _ctl_recvmsg(ShmControlChannelMsg& msg)
    {
        uint64_t payload_size;
        try
        {
            std::vector<char> reply;
            std::size_t len = boost::asio::read(socket_,
                                                boost::asio::buffer(&payload_size,
                                                                    shm_msg_header_size));
            assert(len == shm_msg_header_size);
            reply.resize(payload_size);
            len = boost::asio::read(socket_,
                                    boost::asio::buffer(reply,
                                                        payload_size));
            assert(len == payload_size);
            msg.unpack_msg(reply.data(),
                           len);
        }
        catch(boost::system::system_error&)
        {
            return false;
        }
        return true;
    }
};

typedef std::shared_ptr<ShmControlChannelClient> ShmControlChannelClientPtr;

#endif //__SHM_CONTROL_CHANNEL_CLIENT_H
