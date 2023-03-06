/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Sophie Friedrich
 *
 * device_match.h - API to match devices
 */

#pragma once

#include <string>
#include <vector>

#include <libcamera/base/class.h>


namespace libcamera {

class MediaDeviceBase;

class DeviceMatchEntityInterface
{
public:
	virtual const std::string &match_key() const = 0;
	virtual const std::string &match_value() const = 0;
	virtual ~DeviceMatchEntityInterface() = default;

	static const std::string MATCH_ALL_KEY;
};

class DeviceMatch
{
public:
	DeviceMatch(const std::string &driver);

	void add(const std::string &entity);
	void add(const std::string &key, const std::string &value);

	bool match(const MediaDeviceBase *device) const;

private:
	std::string driver_;
	std::vector<std::pair<std::string, std::string>> map_;
};


}
