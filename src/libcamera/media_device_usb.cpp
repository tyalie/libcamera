/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Sophie Friedrich
 *
 * media_device_usb.cpp - USB media device handler
 */

#include "libcamera/internal/media_device_usb.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>

#include <libcamera/base/log.h>

#include <libusb-1.0/libusb.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(MediaDeviceUSB)

MediaDeviceUSB::MediaDeviceUSB(const std::string &deviceNode)
	: MediaDeviceBase(deviceNode)
{
}

MediaDeviceUSB::~MediaDeviceUSB()
{
	fd_.reset();
}

int MediaDeviceUSB::populate()
{
	int ret;

	driver_ = "libusb";

	ret = open();
	if (ret)
		return ret;

	close();

	return 0;
}

int MediaDeviceUSB::open()
{
	int ret;

	if (fd_.isValid()) {
		LOG(MediaDeviceUSB, Error) << "MediaDeviceUSB already open";
		return -EBUSY;
	}

	fd_ = UniqueFD(::open(deviceNode_.c_str(), O_RDONLY | O_CLOEXEC));
	if (!fd_.isValid()) {
		ret = -errno;
		LOG(MediaDeviceUSB, Error)
			<< "Failed to open usb device at "
			<< deviceNode_ << ": " << strerror(-ret);
		return ret;
	}

	ret = libusb_wrap_sys_device(NULL, (intptr_t)fd_.get(), &usb_handle);

	if (ret < 0) {
		LOG(MediaDeviceUSB, Error)
			<< "Failed to get libusb device from node";
		return ret;
	}

	return 0;
}

void MediaDeviceUSB::close()
{
	libusb_close(usb_handle);
	usb_handle = NULL;
	fd_.reset();
}

void MediaDeviceUSB::clear()
{
}

} // namespace libcamera
