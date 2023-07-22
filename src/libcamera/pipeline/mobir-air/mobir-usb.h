/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023 Sophie 'Tyalie' Friedrich
 *
 * mobir-usb.h -
 */

#pragma once

#include <queue>
#include <vector>

#include <libcamera/base/log.h>
#include <libcamera/base/mutex.h>
#include <libcamera/base/thread.h>

#include "libcamera/internal/media_device_usb.h"

#include <libusb-1.0/libusb.h>

namespace libcamera {

class MobirAirUSBWrapper : public Thread
{
public:
	enum RequestStatus {
		USB_REQUEST_INFLIGHT,
		USB_REQUEST_COMPLETE,
		USB_REQUEST_ERROR
	};

	struct Request {
		friend MobirAirUSBWrapper;

		std::vector<unsigned char> input;
		std::vector<unsigned char> output;
		size_t expected_length;

		enum RequestStatus status;

	private:
		ConditionVariable lock_;
		Mutex mutex_;
	};

	MobirAirUSBWrapper(MediaDeviceUSB *dev)
		: media_device(dev)
	{
	}

	~MobirAirUSBWrapper();

	int open();
	void exit(int code = 0);

	int doRequest(Request *request) LIBCAMERA_TSA_EXCLUDES(mutex_);

	mutable Mutex mutex_;

protected:
	void run() override;

private:
	int claimIf(int ifNum, int altNum);

	void sendRequest(Request *r);

	libusb_device_handle *handle() { return media_device->getUSBHandle(); }
	libusb_device *device() { return libusb_get_device(handle()); }

	MediaDeviceUSB *media_device;

	std::queue<Request *> requests LIBCAMERA_TSA_GUARDED_BY(mutex_);

	std::atomic<bool> should_exit_ = false;
};

} // namespace libcamera
