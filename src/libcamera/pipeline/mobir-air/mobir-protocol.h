/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023 Sophie 'Tyalie' Friedrich
 *
 * mobir-protocol.h -
 */

#include <ctype.h>

#include "mobir-usb.h"

namespace libcamera {
namespace mobir_protocol {

inline void setStream(MobirAirUSBWrapper::Request &req, bool state)
{
	std::string cmd = state ? "StartX=1" : "StopX=1";
	req.expected_length = 0;
	req.setInput(cmd);
}

inline void setShutter(MobirAirUSBWrapper::Request &req, bool state)
{
	std::string cmd = state ? "ShutterOff=1" : "ShutterOn=1";
	req.expected_length = 0;
	req.setInput(cmd);
}

inline void setChangeR(MobirAirUSBWrapper::Request &req, uint8_t ridx)
{
	std::string cmd = "SetDetectIndex=";
	req.setInput(cmd);
	req.input.push_back(ridx & 0xFF);
	req.input.push_back((ridx >> 2) & 0xFF);

	req.expected_length = 0;
}

inline void doNUC(MobirAirUSBWrapper::Request &req)
{
	std::string cmd = "DoNUC=1";
	req.expected_length = 0;
	req.setInput(cmd);
}

inline void getArmParam(MobirAirUSBWrapper::Request &req, uint address, uint length)
{
}

} // namespace mobir_protocol
} // namespace libcamera
