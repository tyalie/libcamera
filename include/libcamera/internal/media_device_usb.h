/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Sophie Friedrich
 *
 * media_device_usb.h - Media device instance for libusb devices
*/

#pragma once

#include "libcamera/internal/media_device_base.h"

#include <libusb-1.0/libusb.h>

namespace libcamera {

class MediaDeviceUSB : public MediaDeviceBase
{
public:
	MediaDeviceUSB(const std::string &deviceNode);
	~MediaDeviceUSB();

	int populate() override;

	libusb_device_handle *getUSBHandle()
	{
		return usb_handle;
	}

private:
	int open(int flag);

	int open() override;
	void close() override;

	void clear() override;

	UniqueFD fd_;
	libusb_device_handle *usb_handle = NULL;
};

} // namespace libcamera
