/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023 Sophie 'Tyalie' Friedrich
 *
 * mobir-manager.cpp -
 */
#include "mobir-manager.h"

#include <chrono>
#include <thread>

#include "mobir-protocol.h"
#include "mobir-usb.h"

namespace libcamera {

LOG_DECLARE_CATEGORY(MOBIR_AIR)

MobirCameraManager::MobirCameraManager(MediaDeviceUSB *dev)
{
	_usb = std::make_unique<MobirAirUSBWrapper>(dev);
}

MobirCameraManager::~MobirCameraManager()
{
	MobirAirUSBWrapper::Request req;

	LOG(MOBIR_AIR, Debug) << "trying request";
	mobir_protocol::setShutter(req, true);
	_usb->doRequest(&req);

	LOG(MOBIR_AIR, Debug) << "trying request";
	mobir_protocol::setStream(req, false);
	_usb->doRequest(&req);
}

int MobirCameraManager::registerRequest(Request *req)
{
	return 0;
}

int MobirCameraManager::init()
{
	MobirAirUSBWrapper::Request req;

	int ret = _usb->open();
	if (ret) {
		LOG(MOBIR_AIR, Error) << "Couldn't open device (errno: " << ret << ")";
		return ret;
	}

	LOG(MOBIR_AIR, Debug) << "starting stream";
	mobir_protocol::setStream(req, true);
	_usb->doRequest(&req);
	LOG(MOBIR_AIR, Debug) << "starting stream success";

	LOG(MOBIR_AIR, Debug) << "trying request";
	mobir_protocol::setShutter(req, true);
	_usb->doRequest(&req);

	LOG(MOBIR_AIR, Debug) << "trying request";
	mobir_protocol::setChangeR(req, 2);
	_usb->doRequest(&req);

	LOG(MOBIR_AIR, Debug) << "trying request";
	mobir_protocol::doNUC(req);
	_usb->doRequest(&req);

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(2s);

	LOG(MOBIR_AIR, Debug) << "trying request";
	mobir_protocol::setShutter(req, false);
	_usb->doRequest(&req);

	return 0;
}

} // namespace libcamera
