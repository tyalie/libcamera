/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2022, Google Inc.
 *
 * media_device.h - Media device handler
 */

#include "libcamera/internal/media_device_virtual.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(MediaDeviceVirtual)

MediaDeviceVirtual::MediaDeviceVirtual(const std::string &deviceNode)
	: MediaDeviceBase(deviceNode)
{
	driver_ = "virtual";
}

MediaDeviceVirtual::~MediaDeviceVirtual() = default;

} /* namespace libcamera */
