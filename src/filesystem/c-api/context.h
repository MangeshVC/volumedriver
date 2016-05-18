// This file is dual licensed GPLv2 and Apache 2.0.
// Active license depends on how it is used.
//
// Copyright 2016 iNuron NV
//
// // GPL //
// This file is part of OpenvStorage.
//
// OpenvStorage is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenvStorage. If not, see <http://www.gnu.org/licenses/>.
//
// // Apache 2.0 //
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __CONTEXT_H
#define __CONTEXT_H

#include <boost/asio.hpp>

#include "../ShmControlChannelProtocol.h"
#include "../ShmClient.h"
#include "../NetworkXioClient.h"

struct ovs_context_t
{
    TransportType transport;
    std::string host;
    int port;
    std::string uri;
    std::string volume_name;
    int oflag;
    ovs_shm_context *shm_ctx_;
    volumedriverfs::NetworkXioClientPtr net_client_;
    uint64_t net_client_qdepth;
};

static bool
_is_volume_name_valid(const char *volume_name)
{
    if (volume_name == NULL || strlen(volume_name) == 0 ||
        strlen(volume_name) >= NAME_MAX)
    {
        return false;
    }
    else
    {
        return true;
    }
}

static inline int
_hostname_to_ip(const char *hostname, std::string& ip)
{

    try
    {
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::ip::tcp::resolver::query query(std::string(hostname), "");
        for (boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query);
                i != boost::asio::ip::tcp::resolver::iterator(); i++)
        {
            boost::asio::ip::tcp::endpoint ep = *i;
            ip = ep.address().to_string();
            return 0;
        }
        errno = EINVAL;
    }
    catch (const std::bad_alloc&)
    {
        errno = ENOMEM;
    }
    catch (...)
    {
        errno = EIO;
    }
    return -1;
}

static inline
ovs_aio_request* create_new_request(RequestOp op,
                                    struct ovs_aiocb *aio,
                                    ovs_completion_t *completion)
{
    try
    {
        ovs_aio_request *request = new ovs_aio_request;
        request->_op = op;
        request->ovs_aiocbp = aio;
        request->completion = completion;
        /*cnanakos TODO: err handling */
        pthread_cond_init(&request->_cond, NULL);
        pthread_mutex_init(&request->_mutex, NULL);
        request->_on_suspend = false;
        request->_canceled = false;
        request->_completed = false;
        request->_signaled = false;
        request->_rv = 0;
        if (aio and op != RequestOp::Noop)
        {
            aio->request_ = request;
        }
        return request;
    }
    catch (const std::bad_alloc&)
    {
        return NULL;
    }
}

static int
ovs_xio_open_volume(ovs_ctx_t *ctx, const char *volume_name)
{
    ssize_t r;
    struct ovs_aiocb aio;

    ovs_aio_request *request = create_new_request(RequestOp::Open,
                                                  &aio,
                                                  NULL);
    if (request == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    try
    {
        ctx->net_client_->xio_send_open_request(volume_name,
                                                reinterpret_cast<void*>(request));
    }
    catch (const volumedriverfs::XioClientQueueIsBusyException&)
    {
        errno = EBUSY;  r = -1;
    }
    catch (const std::bad_alloc&)
    {
        errno = ENOMEM;
        return -1;
    }
    catch (...)
    {
        errno = EIO;
        return -1;
    }

    if ((r = ovs_aio_suspend(ctx, &aio, NULL)) < 0)
    {
        return r;
    }
    r = ovs_aio_return(ctx, &aio);
    if (ovs_aio_finish(ctx, &aio) < 0)
    {
        r = -1;
    }
    return r;
}
#endif // __CONTEXT_H
