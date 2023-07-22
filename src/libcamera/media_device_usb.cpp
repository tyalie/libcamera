/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Sophie Friedrich
 *
 * media_device_usb.cpp - USB media device handler
 */

#include "libcamera/internal/media_device_usb.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>

#include <linux/media.h>
#include <libcamera/base/log.h>

#include <libusb-1.0/libusb.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(MediaDeviceUSB)

MediaDeviceUSB::MediaDeviceUSB(const std::string &deviceNode)
	: MediaDeviceBase(deviceNode)
{
}

MediaDeviceUSB::~MediaDeviceUSB()
{
	MediaDeviceUSB::close();
	fd_.reset();
}

int MediaDeviceUSB::populate()
{
	int ret;
	libusb_device *dev;
	struct libusb_device_descriptor desc;
	MediaEntity *m_entity;

	driver_ = "libusb";

	// TODO: remove this hack
	struct media_v2_entity entity = { 0 };
	struct media_v2_interface iface = { 0 };

	ret = open(O_RDONLY);

	if (ret) {
		LOG(MediaDeviceUSB, Error) << "Couldn't populate USB device";
		goto done;
	}

	dev = libusb_get_device(usb_handle);
	ret = libusb_get_device_descriptor(dev, &desc);
	if (ret) {
		LOG(MediaDeviceUSB, Error) << "Couldn't open device descriptor";
		ret = -EACCES;
		goto done;
	}

	snprintf(entity.name, sizeof(entity.name), "%04x:%04x", desc.idVendor, desc.idProduct);

	m_entity = new MediaEntity(nullptr, &entity, &iface);

	entities_.push_back(m_entity);

done:
	close();
	return ret;
}

int MediaDeviceUSB::open()
{
	return MediaDeviceUSB::open(O_RDWR);
}

int MediaDeviceUSB::open(int flag)
{
	int ret;

	if (fd_.isValid()) {
		LOG(MediaDeviceUSB, Error) << "MediaDeviceUSB already open";
		return -EBUSY;
	}

	fd_ = UniqueFD(::open(deviceNode_.c_str(), flag | O_CLOEXEC));
	if (!fd_.isValid()) {
		ret = -errno;
		LOG(MediaDeviceUSB, Error)
			<< "Failed to open usb device at "
			<< deviceNode_ << ": " << strerror(-ret);
		return ret;
	}

	ret = libusb_wrap_sys_device(NULL, (intptr_t)fd_.get(), &usb_handle);

	if (ret < 0) {
		LOG(MediaDeviceUSB, Error)
			<< "Failed to get libusb device from node";
		return ret;
	}

	return 0;
}

void MediaDeviceUSB::close()
{
	if (usb_handle != nullptr) {
		libusb_close(usb_handle);
		usb_handle = nullptr;
	}
	fd_.reset();
}

void MediaDeviceUSB::clear()
{
}

} // namespace libcamera
