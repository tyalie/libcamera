/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2022, Google Inc.
 *
 * media_device_base.cpp - The base class of media device handler
 */

#include "libcamera/internal/media_device_base.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(MediaDeviceBase)

MediaDeviceBase::MediaDeviceBase(const std::string &deviceNode)
	: deviceNode_(deviceNode), valid_(false), acquired_(false), lock_(false)
{
}

MediaDeviceBase::~MediaDeviceBase()
{
	clear();
}

/**
 * \brief Claim a device for exclusive use
 *
 * The device claiming mechanism offers simple media device access arbitration
 * between multiple users. When the media device is created, it is available to
 * all users. Users can query the media graph to determine whether they can
 * support the device and, if they do, claim the device for exclusive use. Other
 * users are then expected to skip over media devices in use as reported by the
 * busy() function.
 *
 * Once claimed the device shall be released by its user when not needed anymore
 * by calling the release() function. Acquiring the media device opens a file
 * descriptor to the device which is kept open until release() is called.
 *
 * Exclusive access is only guaranteed if all users of the media device abide by
 * the device claiming mechanism, as it isn't enforced by the media device
 * itself.
 *
 * \return true if the device was successfully claimed, or false if it was
 * already in use
 * \sa release(), busy()
 */
bool MediaDeviceBase::acquire()
{
	if (acquired_)
		return false;

	if (open())
		return false;

	acquired_ = true;
	return true;
}

/**
 * \brief Release a device previously claimed for exclusive use
 * \sa acquire(), busy()
 */
void MediaDeviceBase::release()
{
	close();
	acquired_ = false;
}

bool MediaDeviceBase::lock()
{
	if (lock_)
		return false;

	lock_ = true;
	return true;
}

void MediaDeviceBase::unlock()
{
	lock_ = false;
}

int MediaDeviceBase::populate()
{
	valid_ = true;
	return 0;
}

/**
 * \fn MediaDeviceBase::deviceNode()
 * \brief Retrieve the media device node path
 * \return The MediaDeviceBase deviceNode path
 */

/**
 * \fn MediaDeviceBase::entities()
 * \brief Retrieve the list of entities in the media graph
 * \return The list of MediaEntities registered in the MediaDeviceBase
 */

/**
 * \brief Return the MediaEntity with name \a name
 * \param[in] name The entity name
 * \return The entity with \a name, or nullptr if no such entity is found
 */
MediaEntity *MediaDeviceBase::getEntityByName(const std::string &name) const
{
	for (MediaEntity *e : entities_)
		if (e->name() == name)
			return e;

	return nullptr;
}

/**
 * \var MediaDeviceBase::disconnected
 * \brief Signal emitted when the media device is disconnected from the system
 *
 * This signal is emitted when the device enumerator detects that the media
 * device has been removed from the system. For hot-pluggable devices this is
 * usually caused by physical device disconnection, but can also result from
 * driver unloading for most devices. The media device is passed as a parameter.
 */

std::string MediaDeviceBase::logPrefix() const
{
	return deviceNode() + "[" + driver() + "]";
}

void MediaDeviceBase::clear()
{
	entities_.clear();
	valid_ = false;
}

/**
 * \var MediaDeviceBase::entities_
 * \brief Global list of media entities in the media graph
 */

} /* namespace libcamera */
