/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2022, Google Inc.
 *
 * media_device.h - Media device handler
 */

#pragma once

#include "libcamera/internal/media_device_base.h"

namespace libcamera {

class MediaDeviceVirtual : public MediaDeviceBase
{
public:
	MediaDeviceVirtual(const std::string &deviceNode);
	~MediaDeviceVirtual();
};

} /* namespace libcamera */
