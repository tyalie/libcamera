/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Ideas On Board Oy
 *
 * usb_test.cpp - Test USB device matching
 */

#include <libcamera/base/log.h>

#include <libcamera/camera.h>
#include <libcamera/stream.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/pipeline_handler.h"
#include "libcamera/internal/usb_device.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(USBTest)

class USBCameraData : public Camera::Private
{
public:
	USBCameraData(PipelineHandler *pipe)
		: Camera::Private(pipe)
	{
	}

	Stream stream_;
};

class PipelineHandlerUSB : public PipelineHandler
{
public:
	PipelineHandlerUSB(CameraManager *manager)
		: PipelineHandler(manager)
	{
	}

	std::unique_ptr<CameraConfiguration> generateConfiguration([[maybe_unused]] Camera *camera,
								   [[maybe_unused]] Span<const StreamRole> roles) override
	{
		return {};
	}
	int configure([[maybe_unused]] Camera *camera, [[maybe_unused]] CameraConfiguration *config) override
	{
		return 0;
	}

	int exportFrameBuffers([[maybe_unused]] Camera *camera, [[maybe_unused]] Stream *stream,
			       [[maybe_unused]] std::vector<std::unique_ptr<FrameBuffer>> *buffers) override
	{
		return 0;
	}

	int start([[maybe_unused]] Camera *camera, [[maybe_unused]] const ControlList *controls) override
	{
		return 0;
	}
	void stopDevice([[maybe_unused]] Camera *camera) override
	{
	}

	int queueRequestDevice([[maybe_unused]] Camera *camera, [[maybe_unused]] Request *request) override
	{
		return 0;
	}

	bool match(DeviceEnumerator *enumerator) override;
};

bool PipelineHandlerUSB::match(DeviceEnumerator *enumerator)
{
	USBDeviceMatch dm("046d", "c52b");
	USBDevice *usbDev = acquireUSBDevice(enumerator, dm);
	if (!usbDev)
		return false;

	std::unique_ptr<USBCameraData> data = std::make_unique<USBCameraData>(this);
	std::string id = usbDev->vid() + "/" + usbDev->pid();
	std::set<Stream *> streams{ &data->stream_ };

	std::shared_ptr<Camera> camera =
		Camera::create(std::move(data), id, streams);
	registerCamera(std::move(camera));

	return true;
}

REGISTER_PIPELINE_HANDLER(PipelineHandlerUSB)

} /* namespace libcamera */
