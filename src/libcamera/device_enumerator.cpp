/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2018, Google Inc.
 *
 * device_enumerator.cpp - Enumeration and matching
 */

#include "libcamera/internal/device_enumerator.h"

#include <algorithm>
#include <string.h>

#include <libcamera/base/log.h>

#ifdef HAVE_LIBUSB
#include <libusb-1.0/libusb.h>
#endif

#include "libcamera/internal/device_enumerator_sysfs.h"
#include "libcamera/internal/device_enumerator_udev.h"
#include "libcamera/internal/media_device.h"
#include "libcamera/internal/usb_device.h"

/**
 * \file device_enumerator.h
 * \brief Enumeration and matching of camera devices
 *
 * The purpose of device enumeration and matching is to find devices in the
 * system from which a camera can be created and map them to pipeline handlers.
 *
 * At the core of the enumeration is the DeviceEnumerator class, responsible for
 * enumerating all devices in the system used to create cameras. It handles all
 * interactions with the operating system in a platform-specific way. For each
 * system device found an instance of the opportune class is created to store
 * information about the device gathered from the kernel through the supported
 * Linux kernel API, which include the Media Controller API, USB-based devices
 * and more.
 *
 * The DeviceEnumerator can enumerate all or specific devices in the system.
 * When a new device is added the enumerator creates a corresponding instance of
 * the opportune class. In example, for each enumerated media device a
 * MediaDevice class instance is created.
 *
 * The enumerator supports searching among enumerated devices based on criteria
 * expressed in a DeviceMatch derived classes instance.
 */

namespace libcamera {

LOG_DEFINE_CATEGORY(DeviceEnumerator)

/**
 * \class DeviceEnumerator
 * \brief Enumerate, store and search system devices
 *
 * The DeviceEnumerator class is responsible for all interactions with the
 * operating system related to camera devices. It enumerates the devices in the
 * system from which a camera can be created, and for each device found creates
 * an instance of the opportune class and stores it internally. The list of
 * devices can then be searched using the corresponding DeviceMatch derived
 * class search patterns.
 *
 * The enumerator also associates media device entities with device node paths.
 */

/**
 * \brief Create a new device enumerator matching the systems capabilities
 *
 * Depending on how the operating system handles device detection, hot-plug
 * notification and device node lookup, different device enumerator
 * implementations may be needed. This function creates the best enumerator for
 * the operating system based on the available resources. Not all different
 * enumerator types are guaranteed to support all features.
 *
 * \return A pointer to the newly created device enumerator on success, or
 * nullptr if an error occurs
 */
std::unique_ptr<DeviceEnumerator> DeviceEnumerator::create()
{
	std::unique_ptr<DeviceEnumerator> enumerator;

#ifdef HAVE_LIBUSB
	libusb_set_option(NULL, LIBUSB_OPTION_NO_DEVICE_DISCOVERY, NULL);
	// initialize default libusb context, must be called before libusb usage
	libusb_init(NULL);
#endif

#ifdef HAVE_LIBUDEV
	enumerator = std::make_unique<DeviceEnumeratorUdev>();
	if (!enumerator->init())
		return enumerator;
#endif

	/*
	 * Either udev is not available or udev initialization failed. Fall back
	 * on the sysfs enumerator.
	 */
	enumerator = std::make_unique<DeviceEnumeratorSysfs>();
	if (!enumerator->init())
		return enumerator;

	return nullptr;
}

DeviceEnumerator::~DeviceEnumerator()
{
	for (const std::shared_ptr<MediaDevice> &media : mediaDevices_) {
		if (media->busy())
			LOG(DeviceEnumerator, Error)
				<< "Removing media device " << media->deviceNode()
				<< " while still in use";
	}

#ifdef HAVE_LIBUSB
	libusb_exit(NULL);
#endif
}

/**
 * \fn DeviceEnumerator::init()
 * \brief Initialize the enumerator
 * \return 0 on success or a negative error code otherwise
 * \retval -EBUSY the enumerator has already been initialized
 * \retval -ENODEV the enumerator can't enumerate devices
 */

/**
 * \fn DeviceEnumerator::enumerate()
 * \brief Enumerate all camera devices in the system
 *
 * This function finds and add all camera devices in the system to the
 * enumerator. It shall be implemented by all subclasses of DeviceEnumerator
 * using system-specific methods.
 *
 * Individual devices that can't be properly enumerated shall be skipped with a
 * warning message logged, without returning an error. Only errors that prevent
 * enumeration altogether shall be fatal.
 *
 * \context This function is \threadbound.
 *
 * \return 0 on success or a negative error code otherwise
 */

/**
 * \brief Create a media device instance
 * \param[in] deviceNode path to the media device to create
 *
 * Create a media device for the \a deviceNode, open it, and populate its
 * media graph. The device enumerator shall then populate the media device by
 * associating device nodes with entities using MediaEntity::setDeviceNode().
 * This process is specific to each device enumerator, and the device enumerator
 * shall ensure that device nodes are ready to be used (for instance, if
 * applicable, by waiting for device nodes to be created and access permissions
 * to be set by the system). Once done, it shall add the media device to the
 * system with addMediaDevice().
 *
 * \return Created media device instance on success, or nullptr otherwise
 */
std::unique_ptr<MediaDevice> DeviceEnumerator::createMediaDevice(const std::string &deviceNode)
{
	std::unique_ptr<MediaDevice> media = std::make_unique<MediaDevice>(deviceNode);

	int ret = media->populate();
	if (ret < 0) {
		LOG(DeviceEnumerator, Info)
			<< "Unable to populate media device " << deviceNode
			<< " (" << strerror(-ret) << "), skipping";
		return nullptr;
	}

	LOG(DeviceEnumerator, Debug)
		<< "New media device \"" << media->driver()
		<< "\" created from " << deviceNode;

	return media;
}

/**
* \var DeviceEnumerator::devicesAdded
* \brief Notify of new media devices being found
*
* This signal is emitted when the device enumerator finds new media devices in
* the system. It may be emitted for every newly detected device, or once for
* multiple devices, at the discretion of the device enumerator. Not all device
* enumerator types may support dynamic detection of new devices.
*/

/**
 * \brief Add a media device to the enumerator
 * \param[in] media media device instance to add
 *
 * Store the media device in the internal list for later matching with
 * pipeline handlers. \a media shall be created with createDevice() first.
 * This function shall be called after all members of the entities of the
 * media graph have been confirmed to be initialized.
 */
void DeviceEnumerator::addMediaDevice(std::unique_ptr<MediaDevice> media)
{
	LOG(DeviceEnumerator, Debug)
		<< "Added device " << media->deviceNode() << ": " << media->driver();

	mediaDevices_.push_back(std::move(media));

	/* \todo To batch multiple additions, emit with a small delay here. */
	devicesAdded.emit();
}

void DeviceEnumerator::addUSBDevice(std::unique_ptr<USBDevice> usb)
{
	/*
	 * This is a bit of an hack and could be improved!
	 *
	 * Can't use std::sort() + std::unique() because we're storing
	 * unique_ptr<>
	 */
	for (const auto &dev : usbDevices_) {
		if (dev->pid() == usb->pid() && dev->vid() == usb->vid())
			return;
	}

	LOG(DeviceEnumerator, Debug)
		<< "Added USB device " << usb->vid() << "-" << usb->pid();

	usbDevices_.push_back(std::move(usb));
}

/**
 * \brief Remove a media device from the enumerator
 * \param[in] deviceNode Path to the media device to remove
 *
 * Remove the media device identified by \a deviceNode previously added to the
 * enumerator with addMediaDevice(). The media device's
 * MediaDevice::disconnected signal is emitted.
 */
void DeviceEnumerator::removeMediaDevice(const std::string &deviceNode)
{
	std::shared_ptr<MediaDevice> media;

	for (auto iter = mediaDevices_.begin(); iter != mediaDevices_.end(); ++iter) {
		if ((*iter)->deviceNode() == deviceNode) {
			media = std::move(*iter);
			mediaDevices_.erase(iter);
			break;
		}
	}

	if (!media) {
		LOG(DeviceEnumerator, Warning)
			<< "Media device for node " << deviceNode
			<< " not found";
		return;
	}

	LOG(DeviceEnumerator, Debug)
		<< "Media device for node " << deviceNode << " removed.";

	media->disconnected.emit();
}

/**
 * \brief Search available media devices for a pattern match
 * \param[in] dm Search pattern
 *
 * Search in the enumerated media devices that are not already in use for a
 * match described in \a dm. If a match is found and the caller intends to use
 * it the caller is responsible for acquiring the MediaDevice object and
 * releasing it when done with it.
 *
 * \return pointer to the matching MediaDevice, or nullptr if no match is found
 */
std::shared_ptr<MediaDevice> DeviceEnumerator::search(const MediaDeviceMatch &dm)
{
	for (std::shared_ptr<MediaDevice> &media : mediaDevices_) {
		if (media->busy())
			continue;

		if (dm.match(media.get())) {
			LOG(DeviceEnumerator, Debug)
				<< "Successful match for media device \""
				<< media->driver() << "\"";
			return media;
		}
	}

	return nullptr;
}

std::shared_ptr<USBDevice> DeviceEnumerator::search(const USBDeviceMatch &dm)
{
	for (std::shared_ptr<USBDevice> &usb : usbDevices_) {
		if (dm.match(usb.get())) {
			LOG(DeviceEnumerator, Debug)
				<< "Successful match for USB device "
				<< usb->vid() << "-" << usb->pid();
			return usb;
		}
	}

	return nullptr;
}

} /* namespace libcamera */
