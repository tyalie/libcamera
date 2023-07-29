/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023 Sophie 'Tyalie' Friedrich
 *
 * mobir-usb.cpp -
 */

#include "mobir-usb.h"

#include <errno.h>

#include <libusb-1.0/libusb.h>

namespace libcamera {

LOG_DECLARE_CATEGORY(MOBIR_AIR)

MobirAirUSBWrapper::~MobirAirUSBWrapper()
{
	LOG(MOBIR_AIR, Debug) << "closing device";
	exit();
	wait();
}

int MobirAirUSBWrapper::open()
{
	struct libusb_config_descriptor *config = nullptr;
	Request request;
	int err = 0;
	int ret;

	LOG(MOBIR_AIR, Debug) << "opening device";

	libusb_reset_device(handle());

	if (libusb_get_config_descriptor(device(), 0, &config)) {
		err = EIO;
		goto close;
	}

	LOG(MOBIR_AIR, Debug)
		<< "device has " << (int)config->bNumInterfaces << " interface(s)";

	// we rely on the MobirAir to have this USB config
	// TODO: generalize / improve this check
	ASSERT(config->bNumInterfaces == 2);
	ASSERT(config->interface[1].num_altsetting == 2);
	ASSERT(config->interface[1].altsetting[1].bNumEndpoints == 2);

	if ((ret = claimIf(1, 1))) {
		err = ret;
		goto close;
	}

	Thread::start();

close:
	if (config != nullptr)
		libusb_free_config_descriptor(config);
	return -err;
}

int MobirAirUSBWrapper::claimIf(int ifNum, int altNum)
{
	int err = 0;
	int ret;

	ret = libusb_detach_kernel_driver(handle(), ifNum);

	if (ret && ret != LIBUSB_ERROR_NOT_FOUND && ret != LIBUSB_ERROR_NOT_SUPPORTED) {
		LOG(MOBIR_AIR, Error)
			<< "not claiming interface " << ifNum
			<< ": unable to detach kernel driver (" << ret << ")";
		err = EIO;
		goto fail;
	}

	LOG(MOBIR_AIR, Debug) << "Claiming interface " << ifNum;
	ret = libusb_claim_interface(handle(), ifNum);
	if (ret) {
		LOG(MOBIR_AIR, Error) << "failed claiming interface";
		err = EIO;
		goto fail;
	}

	ret = libusb_set_interface_alt_setting(handle(), ifNum, altNum);
	if (ret) {
		LOG(MOBIR_AIR, Error) << "failed setting alt mode";
		err = EIO;
		goto fail;
	}

fail:

	return -err;
}

int MobirAirUSBWrapper::doRequest(Request *request)
{
	request->status = USB_REQUEST_INFLIGHT;

	{
		MutexLocker locker(mutex_);
		requests.push(request);
	}

	{
		MutexLocker locker(request->mutex_);
		request->lock_.wait(locker, [&]() LIBCAMERA_TSA_REQUIRES(request->mutex_) {
			return request->status != USB_REQUEST_INFLIGHT;
		});
	}

	return 0;
}

void MobirAirUSBWrapper::sendRequest(Request *r)
{
	int ret;
	int transferred;

	MutexLocker lock(r->mutex_);

	if (r->input.size() > 0) {
		LOG(MOBIR_AIR, Debug) << "sending out with length: " << r->input.size();
		ret = libusb_bulk_transfer(
			handle(), 0x1, r->input.data(),
			r->input.size(), &transferred, 0);

		if (transferred != r->input.size() || ret) {
			LOG(MOBIR_AIR, Debug) << "expected data send and actual send mismatch " << ret;
			r->status = USB_REQUEST_ERROR;
			return;
		}
	}

	if (r->expected_length > 0) {
		r->output = std::vector<unsigned char>();
		r->output.resize(r->expected_length);

		int cum_transferred = 0;
		do {
			auto _d = std::min((size_t)64 * 64 * 2, r->expected_length - cum_transferred);

			ret = libusb_bulk_transfer(
				handle(), 0x81, r->output.data() + cum_transferred,
				_d, &transferred, 300);
			cum_transferred += transferred;
		} while (!ret && cum_transferred < r->expected_length);

		if (cum_transferred == r->expected_length || ret) {
			LOG(MOBIR_AIR, Debug) << "expected data recv and actual recv mismatch " << libusb_error_name(ret) << " (" << cum_transferred << " / " << r->expected_length << ")";
			r->status = USB_REQUEST_ERROR;
			return;
		}
	}

	r->status = USB_REQUEST_COMPLETE;

}

void MobirAirUSBWrapper::run()
{
	LOG(MOBIR_AIR, Debug) << "Starting USB thread";

	while (!should_exit_.load()) {
		{
			MutexLocker locker(mutex_);
			if (!requests.empty()) {
				LOG(MOBIR_AIR, Debug) << "processing request";
				Request *r = requests.front();
				requests.pop();

				sendRequest(r);

				r->lock_.notify_all();
			}
		}
	}
}

void MobirAirUSBWrapper::exit(int code)
{
	should_exit_.store(true);
}

} // namespace libcamera
