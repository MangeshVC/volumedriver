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

#include "RandomDeviceReader.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <iostream>
#include "VTException.h"
#include <errno.h>
#include <youtils/Logging.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>

RandomDeviceReader::RandomDeviceReader(const std::string& device,
                                       const std::string& reference,
                                       const uint64_t num_reads)
 : device_name_(device)
 , reference_name_(reference)
 , size_(0)
 , num_reads_(num_reads)
 , device_fd_(-1)
 , reference_fd_(-1)
{
  device_fd_ = open(device.c_str(),
                    O_RDONLY|O_FSYNC);
  if(device_fd_<0)
  {
    LOG_ERROR("Could not open " << device_name_);
    throw VTException();
  }

  if(not reference.empty())
  {

      reference_fd_= open(reference.c_str(),
                          O_RDONLY|O_EXCL|O_FSYNC);

      if(reference_fd_<0)
      {
          LOG_ERROR("Could not open file " << reference_name_);
          throw VTException();
      }
  }

  ioctl(device_fd_,BLKGETSIZE64, & size_);

}

uint64_t
RandomDeviceReader::nextReadPoint()
{
    uint64_t ret = drand48() * (double)size_;
    return ret;

}

bool
RandomDeviceReader::operator()()
{
    char device[blocksize_];
    char reference[blocksize_];
    for(uint64_t i = 0; i < num_reads_; ++i)
    {
        uint64_t place_to_read = nextReadPoint();

        if(lseek(device_fd_,place_to_read, SEEK_SET)==-1)
        {
            LOG_ERROR("Could not seek to " << place_to_read <<"  on " << device_name_);
            return false;
        }
        ssize_t device_read_size = read(device_fd_,&device, blocksize_);

        if (device_read_size < 0)
        {
            LOG_ERROR("Read error on device");
            throw VTException("Read error on device");
        }

        if(reference_fd_ >= 0)
        {
            if(lseek(reference_fd_,place_to_read, SEEK_SET)==-1)
            {
                LOG_ERROR("Could not seek to " << place_to_read <<"  on " << reference_name_);
                return false;
            }

            ssize_t reference_read_size= read(reference_fd_,&reference,blocksize_);

            if (reference_read_size < 0)
            {
                LOG_ERROR("Read error on reference");
                throw VTException("Read error on reference");
            }


            if(device_read_size != reference_read_size)
            {
                LOG_ERROR("Read different amounts of data from the device and reference");
                return false;
            }

            if(memcmp(device, reference,device_read_size) != 0)
            {
                LOG_ERROR("Difference in blocks at offset " << place_to_read );
                return false;
            }
        }
    }
    return true;
}

// Local Variables: **
// compile-command: "scons -D --kernel_version=system --ignore-buildinfo -j 5" **
// mode: c++ **
// End: **
