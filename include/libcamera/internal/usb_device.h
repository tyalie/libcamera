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
	USBDevice(const char *vid, const char *pid)
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

	const std::string &vid() const { return vid_; };
	const std::string &pid() const { return pid_; };

private:
	std::string vid_;
	std::string pid_;
};

} /* namespace libcamera */
