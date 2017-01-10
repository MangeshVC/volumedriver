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

#include "AioCompletion.h"

void
AioCompletion::schedule(ovs_completion_t *completion)
{
    io_service_.post(boost::bind(completion->complete_cb,
                                 completion,
                                 completion->cb_arg));
}

AioCompletion&
AioCompletion::get_aio_context()
{
    static AioCompletion aio_completion_instance_;
    return aio_completion_instance_;
}
