/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Ideas On Board Oy
 *
 * camera_device.h - Base class for camera devices
 */

#pragma once

namespace libcamera {

class CameraDevice
{
public:
	CameraDevice()
		: acquired_(false)
	{
	}

	virtual bool acquire() = 0;
	virtual void release() = 0;
	bool busy() const { return acquired_; }

	virtual bool lock() = 0;
	virtual void unlock() = 0;

protected:
	bool acquired_;
};

} /* namespace libcamera */
