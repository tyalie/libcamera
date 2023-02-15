/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Sophie Friedrich
 *
 * media_device_usb.cpp - USB media device handler
 */

#include "libcamera/internal/media_device_usb.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(MediaDeviceUSB)

MediaDeviceUSB::MediaDeviceUSB(const std::string &deviceNode)
	: MediaDeviceBase(deviceNode)
{
}

MediaDeviceUSB::~MediaDeviceUSB() = default;

} // namespace libcamera
