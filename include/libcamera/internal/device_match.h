/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Ideas On Board Oy
 *
 * device_match.h - Match and identify devices to create cameras with
 */

#pragma once

#include <string>
#include <vector>

namespace libcamera {

class CameraDevice;

class DeviceMatch
{
public:
	virtual bool match(const CameraDevice *device) const = 0;
};

class MediaDeviceMatch : public DeviceMatch
{
public:
	void add(const std::string &entity);
	MediaDeviceMatch(const std::string &driver);

	bool match(const CameraDevice *device) const override;

private:
	std::string driver_;
	std::vector<std::string> entities_;
};

class USBDeviceMatch : public DeviceMatch
{
public:
	USBDeviceMatch(uint16_t vid, uint16_t pid)
		: vid_(vid), pid_(pid)
	{
	}

	bool match(const CameraDevice *device) const override;

private:
	uint16_t vid_;
	uint16_t pid_;
};

}; /* namespace libcamera */
