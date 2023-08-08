/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2018, Google Inc.
 *
 * device_enumerator.h - API to enumerate and find media devices
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <libcamera/base/signal.h>

#include "libcamera/internal/device_match.h"

namespace libcamera {

class MediaDevice;
class USBDevice;

class DeviceEnumerator
{
public:
	static std::unique_ptr<DeviceEnumerator> create();

	virtual ~DeviceEnumerator();

	virtual int init() = 0;
	virtual int enumerate() = 0;

	std::shared_ptr<MediaDevice> search(const MediaDeviceMatch &dm);

	Signal<> devicesAdded;

protected:
	std::unique_ptr<MediaDevice> createMediaDevice(const std::string &deviceNode);
	void addMediaDevice(std::unique_ptr<MediaDevice> media);
	void removeMediaDevice(const std::string &deviceNode);

	void addUSBDevice(std::unique_ptr<USBDevice> usb);
	/* \todo implement remove() */

private:
	std::vector<std::shared_ptr<MediaDevice>> mediaDevices_;
	std::vector<std::shared_ptr<USBDevice>> usbDevices_;
};

} /* namespace libcamera */
