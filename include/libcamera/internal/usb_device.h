/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Ideas On Board Oy
 *
 * usb_device.h - Description of a USB device
 */

#pragma once

#include <fcntl.h>

#include <libcamera/base/log.h>
#include <libcamera/base/unique_fd.h>

#include "libcamera/internal/camera_device.h"

struct libusb_device_handle;
struct libusb_device;

namespace libcamera {

class USBDevice : protected Loggable, public CameraDevice
{
public:
	USBDevice(uint16_t *vid, uint16_t *pid)
		: CameraDevice(), vid_(vid), pid_(pid)
	{
	}


	~USBDevice();

	bool acquire() override;
	void release() override;
	bool lock() override;
	void unlock() override;

	std::shared_ptr<libusb_device_handle> getUSBHandle();
	libusb_device *getUSBDevice();

	uint16_t pid() const { return pid_; }
	uint16_t vid() const { return vid_; }
	const std::string &deviceNode() const { return deviceNode_; }

	std::string simpleName() const;

protected:
	std::string logPrefix() const override;

private:
	int open(int flag = O_RDWR);
	void close();

	uint16_t vid_, pid_;

	std::string deviceNode_;
	UniqueFD fd_;
	std::shared_ptr<libusb_device_handle> usb_handle_;
};

} /* namespace libcamera */
