//* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023 Sophie 'Tyalie' Friedrich
 *
 * mobir-manager.h -
 */

#include <libcamera/request.h>

#include "mobir-usb.h"

namespace libcamera {

class MobirCameraManager
{
public:
	MobirCameraManager(MediaDeviceUSB *dev);
	~MobirCameraManager();

	int init();
	int registerRequest(Request *request);

	std::unique_ptr<MobirAirUSBWrapper> _usb;
};

} // namespace libcamera
