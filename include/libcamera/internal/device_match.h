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

class MediaDevice;

class DeviceMatch
{
public:
	DeviceMatch(const std::string &driver);

	void add(const std::string &entity);

	bool match(const MediaDevice *device) const;

private:
	std::string driver_;
	std::vector<std::string> entities_;
};

}; /* namespace libcamera */
