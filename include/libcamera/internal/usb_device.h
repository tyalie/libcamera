/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Ideas On Board Oy
 *
 * usb_device.h - Description of a USB device
 */

#pragma once

#include "libcamera/internal/camera_device.h"

namespace libcamera {

class USBDevice : public CameraDevice
{
public:
	USBDevice(uint16_t *vid, uint16_t *pid)
		: CameraDevice(), vid_(vid), pid_(pid)
	{
	}

	/* \todo Implement acquire/release and lock/unlock */
	bool acquire() override
	{
		/* This only works within the same process!! */
		if (acquired_)
			return false;

		acquired_ = true;
		return true;
	}
	void release() override
	{
	}

	bool lock() override
	{
		return true;
	}
	void unlock() override
	{
	}
	~USBDevice() {}

	uint16_t pid() const { return pid_; }
	uint16_t vid() const { return vid_; }

private:
	uint16_t vid_, pid_;
};

} /* namespace libcamera */
