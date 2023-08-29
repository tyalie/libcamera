/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023 Sophie 'Tyalie' Friedrich
 *
 * usb_device.cpp - Description of a USB device
 */

#include "libcamera/internal/usb_device.h"

#include <errno.h>
#include <iomanip>
#include <sstream>
#include <stdint.h>

#include <libcamera/base/log.h>

#include <libusb-1.0/libusb.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(USBDevice);

USBDevice::~USBDevice()
{
	USBDevice::close();
}

std::string USBDevice::simpleName() const
{
	std::stringstream s;
	s << std::setfill('0') << std::setw(4) << std::hex << vid()
	  << ":"
	  << std::setfill('0') << std::setw(4) << std::hex << pid();
	return s.str();
}

std::string USBDevice::logPrefix() const
{
	return simpleName() + " [ libusb ]";
}

int USBDevice::open(int flag)
{
	if (fd_.isValid()) {
		LOG(USBDevice, Error) << "USBDevice already open";
		return -EBUSY;
	}

	fd_ = UniqueFD(::open(deviceNode().c_str(), flag | O_CLOEXEC));
	if (!fd_.isValid()) {
		int ret = -errno;
		LOG(USBDevice, Error)
			<< "Failed to open usb device at "
			<< deviceNode() << ": " << strerror(-ret);
		return ret;
	}

	libusb_device_handle *handle = nullptr;
	int ret = libusb_wrap_sys_device(NULL, (intptr_t)fd_.get(), &handle);

	if (ret) {
		LOG(USBDevice, Error)
			<< "Failed to get libusb device from node";
		return ret;
	}

	usb_handle_ = std::shared_ptr<libusb_device_handle>(handle, libusb_close);

	return 0;
}

bool USBDevice::acquire()
{
	if (acquired_)
		return false;

	if (open())
		return false;

	acquired_ = true;
	return true;
}
void USBDevice::release()
{
	close();
	acquired_ = false;
}

bool USBDevice::lock()
{
	if (!fd_.isValid())
		return false;

	if (lockf(fd_.get(), F_TLOCK, 0))
		return false;

	return true;
}
void USBDevice::unlock()
{
	if (!fd_.isValid())
		return;

	lockf(fd_.get(), F_ULOCK, 0);
}

std::shared_ptr<libusb_device_handle> USBDevice::getUSBHandle()
{
	return usb_handle_;
}

libusb_device *USBDevice::getUSBDevice()
{
	return libusb_get_device(usb_handle_.get());
}

void USBDevice::close()
{
	usb_handle_.reset();
	fd_.reset();
}

} /* namespace libcamera */
