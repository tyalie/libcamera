/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Ideas On Board Oy
 *
 * device_match.cpp - Match and identify devices to create cameras with
 */

#include "libcamera/internal/device_match.h"

#include "libcamera/internal/media_device.h"

/**
 * \file device_match.h
 * \brief Define types and functions to identify devices used to create cameras
 */

namespace libcamera {

/**
 * \class DeviceMatch
 * \brief Pure virtual base class for device serch pattern
 *
 * The DeviceMatch class defines the interface to implement device search
 * patterns to allow searching and matching different device typologies, such as
 * media devices for V4L2/MC cameras, USB for cameras controlled through the USB
 * protocol which do not implement the UVC specification and for virtual
 * cameras.
 *
 * Pipeline handlers are expected to instantiate the correct derived class
 * depending on the device type they support and populate it with their desired
 * matching criteria. Derived classes of DeviceMatch override the pure virtual
 * match() function to implement custom matching criteria based on the device
 * type they represent.
 */

/**
 * \class MediaDeviceMatch
 * \brief Description of a media device search pattern
 *
 * The MediaDeviceMatch class describes a media device using properties from the
 * Media Controller struct media_device_info, entity names in the media graph
 * or other properties that can be used to identify a media device.
 *
 * The description is meant to be filled by pipeline managers and passed to a
 * device enumerator to find matching media devices.
 *
 * A MediaDeviceMatch is created with a specific Linux device driver in mind,
 * therefore the name of the driver is a required property. One or more Entity
 * names can be added as match criteria.
 *
 * Pipeline handlers are recommended to add entities to MediaDeviceMatch as
 * appropriate to ensure that the media device they need can be uniquely
 * identified. This is useful when the corresponding kernel driver can produce
 * different graphs, for instance as a result of different driver versions or
 * hardware configurations, and not all those graphs are suitable for a pipeline
 * handler.
 */

/**
 * \brief Construct a media device search pattern
 * \param[in] driver The Linux device driver name that created the media device
 */
MediaDeviceMatch::MediaDeviceMatch(const std::string &driver)
	: driver_(driver)
{
}

/**
 * \brief Add a media entity name to the search pattern
 * \param[in] entity The name of the entity in the media graph
 */
void MediaDeviceMatch::add(const std::string &entity)
{
	entities_.push_back(entity);
}

/**
 * \brief Compare a search pattern with a media device
 * \param[in] device The media device
 *
 * Matching is performed on the Linux device driver name and entity names from
 * the media graph. A match is found if both the driver name matches and the
 * media device contains all the entities listed in the search pattern.
 *
 * \return True if the media device matches the search pattern, false otherwise
 */
bool MediaDeviceMatch::match(const MediaDevice *device) const
{
	if (driver_ != device->driver())
		return false;

	for (const std::string &name : entities_) {
		bool found = false;

		for (const MediaEntity *entity : device->entities()) {
			if (name == entity->name()) {
				found = true;
				break;
			}
		}

		if (!found)
			return false;
	}

	return true;
}

} /* namespace libcamera */
