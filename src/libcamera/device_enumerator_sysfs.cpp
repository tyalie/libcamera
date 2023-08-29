/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * device_enumerator_sysfs.cpp - sysfs-based device enumerator
 */

#include "libcamera/internal/device_enumerator_sysfs.h"

#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <libcamera/base/log.h>

#include "libcamera/internal/media_device.h"
#include "libcamera/internal/usb_device.h"

namespace libcamera {

LOG_DECLARE_CATEGORY(DeviceEnumerator)

int DeviceEnumeratorSysfs::init()
{
	return 0;
}

int DeviceEnumeratorSysfs::enumerate()
{
	int ret;

	if ((ret = enumerateMediaDevice()) < 0)
		return ret;

	if ((ret = enumerateUSBDevice()) < 0)
		return ret;

	return 0;
}

int DeviceEnumeratorSysfs::enumerateMediaDevice()
{
	struct dirent *ent;
	DIR *dir;

	static const char * const sysfs_dirs[] = {
		"/sys/subsystem/media/devices",
		"/sys/bus/media/devices",
		"/sys/class/media/devices",
	};

	for (const char *dirname : sysfs_dirs) {
		dir = opendir(dirname);
		if (dir)
			break;
	}

	if (!dir) {
		LOG(DeviceEnumerator, Error)
			<< "No valid sysfs media device directory";
		return -ENODEV;
	}

	while ((ent = readdir(dir)) != nullptr) {
		if (strncmp(ent->d_name, "media", 5))
			continue;

		char *end;
		unsigned int idx = strtoul(ent->d_name + 5, &end, 10);
		if (*end != '\0')
			continue;

		std::string devnode = "/dev/media" + std::to_string(idx);

		/* Verify that the device node exists. */
		struct stat devstat;
		if (stat(devnode.c_str(), &devstat) < 0) {
			LOG(DeviceEnumerator, Warning)
				<< "Device node /dev/media" << idx
				<< " should exist but doesn't";
			continue;
		}

		std::unique_ptr<MediaDevice> media = createDevice<MediaDevice>(devnode);
		if (!media)
			continue;

		if (populateMediaDevice(media.get()) < 0) {
			LOG(DeviceEnumerator, Warning)
				<< "Failed to populate media device "
				<< media->deviceNode()
				<< " (" << media->driver() << "), skipping";
			continue;
		}

		addMediaDevice(std::move(media));
	}

	closedir(dir);

	return 0;
}

int DeviceEnumeratorSysfs::enumerateUSBDevice()
{
	struct dirent *ent;
	DIR *dir;

	static const char * const sysfs_dir = "/dev/bus/usb";

	dir = opendir(sysfs_dir);
	if (!dir) {
		LOG(DeviceEnumerator, Error)
			<< "No valid sysfs media device directory";
		return -ENODEV;
	}

	while ((ent = readdir(dir)) != nullptr) {
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			continue;

		std::string buspath =
			std::string(sysfs_dir) + "/" + std::string(ent->d_name);

		if (ent->d_type != DT_DIR) {
			LOG(DeviceEnumerator, Error)
				<< "Invalid bus directory " << buspath << " is not a folder";
			continue;
		}

		enumerateUSBBusDevices(buspath);
	}
	closedir(dir);

	return 0;
}

int DeviceEnumeratorSysfs::enumerateUSBBusDevices(const std::string &busdir)
{
	struct dirent *ent;
	DIR *dir;

	dir = opendir(busdir.c_str());

	if (!dir) {
		LOG(DeviceEnumerator, Error)
			<< "No valid sysfs media bus device directory (" << busdir << ")";
		return -ENODEV;
	}

	while ((ent = readdir(dir)) != nullptr) {
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			continue;

		// this assumes that the main bus devices are always enlisted as dev 001
		if (!strcmp(ent->d_name, "001"))
			continue;

		std::string devnode = busdir + "/" + std::string(ent->d_name);

		if (ent->d_type != DT_CHR) {
			LOG(DeviceEnumerator, Warning)
				<< "Invalid char device " << devnode;
			continue;
		}

		std::unique_ptr<USBDevice> usbDev = createDevice<USBDevice>(devnode);
		if (!usbDev)
			continue;

		addUSBDevice(std::move(usbDev));
	}

	closedir(dir);
	return 0;
}

int DeviceEnumeratorSysfs::populateMediaDevice(MediaDevice *media)
{
	/* Associate entities to device node paths. */
	for (MediaEntity *entity : media->entities()) {
		if (entity->deviceMajor() == 0 && entity->deviceMinor() == 0)
			continue;

		std::string deviceNode = lookupDeviceNode(entity->deviceMajor(),
							  entity->deviceMinor());
		if (deviceNode.empty())
			return -EINVAL;

		int ret = entity->setDeviceNode(deviceNode);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 * \brief Lookup device node path from device number
 * \param[in] major The device major number
 * \param[in] minor The device minor number
 *
 * Translate a device number given as \a major and \a minor to a device node
 * path.
 *
 * \return The device node path on success, or an empty string if the lookup
 * fails
 */
std::string DeviceEnumeratorSysfs::lookupDeviceNode(int major, int minor)
{
	std::string deviceNode;
	std::string line;
	std::ifstream ueventFile;

	ueventFile.open("/sys/dev/char/" + std::to_string(major) + ":" +
			std::to_string(minor) + "/uevent");
	if (!ueventFile)
		return std::string();

	while (ueventFile >> line) {
		if (line.find("DEVNAME=") == 0) {
			deviceNode = "/dev/" + line.substr(strlen("DEVNAME="));
			break;
		}
	}

	ueventFile.close();

	return deviceNode;
}

} /* namespace libcamera */
