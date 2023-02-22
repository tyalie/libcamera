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

class MediaDeviceBase;

class DeviceEnumerator
{
public:
	static std::unique_ptr<DeviceEnumerator> create();

	virtual ~DeviceEnumerator();

	virtual int init() = 0;
	virtual int enumerate() = 0;

	void addDevice(std::unique_ptr<MediaDeviceBase> media);
	std::shared_ptr<MediaDeviceBase> search(const DeviceMatch &dm);

	Signal<> devicesAdded;

protected:
	template<class T>
	std::unique_ptr<MediaDeviceBase> createDevice(const std::string &deviceNode);
	void removeDevice(const std::string &deviceNode);

private:
	std::vector<std::shared_ptr<MediaDeviceBase>> devices_;
};

} /* namespace libcamera */
