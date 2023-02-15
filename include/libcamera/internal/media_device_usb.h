/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Sophie Friedrich
 *
 * media_device_usb.h - Media device instance for libusb devices
*/

#pragma once

#include "libcamera/internal/media_device_base.h"

namespace libcamera {

class MediaDeviceUSB : public MediaDeviceBase
{
public:
	MediaDeviceUSB(const std::string &deviceNode);
	~MediaDeviceUSB();
};

} // namespace libcamera
