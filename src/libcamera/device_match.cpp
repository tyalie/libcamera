/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Sophie Friedrich
 *
 * device_match.cpp - API to match devices
 */

#include "libcamera/internal/device_match.h"

#include <string.h>

#include <libcamera/base/log.h>

#include "libcamera/internal/media_device.h"

/**
 * \file device_match.h
 * \brief Matching of media devices
 *
 * Pipelines find compatible devices by matching against known strings or
 * key/value pairs during device enumeration. MediaDevice are responsible for
 * finding these entities during their population phase.
 */

namespace libcamera {

LOG_DEFINE_CATEGORY(DeviceMatch)

/**
 * \interface DeviceMatchInterface
 * \brief Interface which enables matching using DeviceMatch
 *
 *
 *
 */
const std::string DeviceMatchEntityInterface::MATCH_ALL_KEY = "*";

/**
 * \class DeviceMatch
 * \brief Description of a media device search pattern
 *
 * The DeviceMatch class describes a media device using properties from the
 * Media Controller struct media_device_info, entity names in the media graph
 * or other properties that can be used to identify a media device.
 *
 * The description is meant to be filled by pipeline managers and passed to a
 * device enumerator to find matching media devices.
 *
 * A DeviceMatch is created with a specific Linux device driver in mind,
 * therefore the name of the driver is a required property. One or more Entity
 * names can be added as match criteria.
 *
 * Pipeline handlers are recommended to add entities to DeviceMatch as
 * appropriare to ensure that the media device they need can be uniquely
 * identified. This is useful when the corresponding kernel driver can produce
 * different graphs, for instance as a result of different driver versions or
 * hardware configurations, and not all those graphs are suitable for a pipeline
 * handler.
 */

/**
 * \brief Construct a media device search pattern
 * \param[in] driver The Linux device driver name that created the media device
 */
DeviceMatch::DeviceMatch(const std::string &driver)
	: driver_(driver)
{
}

/**
 * \brief Add a media entity name to the search pattern
 * \param[in] entity The name of the entity in the media graph
 */
void DeviceMatch::add(const std::string &entity)
{
	add(DeviceMatchEntityInterface::MATCH_ALL_KEY, entity);
}

/**
 * \brief Add key / value pair to search pattern
 * \param[in] key The key the value is associated to
 * \param[in] value The value that should be searched for
 *
 * There is no uniqueness enforced to key/value. Mentioning a certain
 * key multiple times (in best case with different values) allows matching
 * multiple properties on the same key.
 */
void DeviceMatch::add(const std::string &key, const std::string &value)
{
	map_.push_back(std::pair(key, value));
}

/**
 * \brief Compare a search pattern with a media device
 * \param[in] device The media device
 *
 * Matching is performed on the Linux device driver name and entity names from
 * the media graph. A match is found if both the driver name matches and the
 * media device contains all the entities listed in the search pattern.
 *
 * \return true if the media device matches the search pattern, false otherwise
 */
bool DeviceMatch::match(const MediaDevice *device) const
{
	if (driver_ != device->driver())
		return false;

	for (const std::pair<std::string, std::string> &item : map_) {
		bool found = false;

		for (const DeviceMatchEntityInterface *entity : device->entities()) {
			if (item.first == entity->match_key() &&
				item.second == entity->match_value()) {
				found = true;
				break;
			}
		}

		if (!found)
			return false;
	}

	return true;
}

}