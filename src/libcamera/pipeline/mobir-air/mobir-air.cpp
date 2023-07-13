/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Sophie Friedrich
 *
 * mobir-air.cpp - Pipeline handler for MobirAir thermal camera
 */

#include <libcamera/base/log.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/media_device_usb.h"
#include "libcamera/internal/pipeline_handler.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(MOBIR_AIR)

class MobirAirCameraConfiguration : public CameraConfiguration
{
public:
	MobirAirCameraConfiguration();

	Status validate() override;
};

class PipelineHandlerMobirAir : public PipelineHandler
{
public:
	PipelineHandlerMobirAir(CameraManager *manager);

	std::unique_ptr<CameraConfiguration> generateConfiguration(Camera *camera,
								   const StreamRoles &roles) override;

	int configure(Camera *camera, CameraConfiguration *config) override;

	int exportFrameBuffers(Camera *camera, Stream *stream,
			       std::vector<std::unique_ptr<FrameBuffer>> *buffers) override;

	int start(Camera *camera, const ControlList *controls) override;
	void stopDevice(Camera *camera) override;

	int queueRequestDevice(Camera *camera, Request *request) override;

	bool match(DeviceEnumerator *enumerator) override;
};

MobirAirCameraConfiguration::MobirAirCameraConfiguration()
	: CameraConfiguration()
{
}

CameraConfiguration::Status MobirAirCameraConfiguration::validate()
{
	return Invalid;
}

PipelineHandlerMobirAir::PipelineHandlerMobirAir(CameraManager *manager)
	: PipelineHandler(manager)
{
}

std::unique_ptr<CameraConfiguration>
PipelineHandlerMobirAir::generateConfiguration(Camera *camera,
					       const StreamRoles &roles)
{
	return std::unique_ptr<MobirAirCameraConfiguration>{};
}

int PipelineHandlerMobirAir::configure(Camera *camera, CameraConfiguration *config)
{
	return -1;
}

int PipelineHandlerMobirAir::exportFrameBuffers(Camera *camera, Stream *stream,
						std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	return -1;
}

int PipelineHandlerMobirAir::start(Camera *camera, [[maybe_unused]] const ControlList *controls)
{
	return -1;
}

void PipelineHandlerMobirAir::stopDevice(Camera *camera)
{
}

int PipelineHandlerMobirAir::queueRequestDevice(Camera *camera, Request *request)
{
	return -1;
}

bool PipelineHandlerMobirAir::match(DeviceEnumerator *enumerator)
{
	MediaDeviceUSB *media;
	DeviceMatch dm("libusb");
	dm.add("0525:a4a0");

	media = dynamic_cast<MediaDeviceUSB *>(acquireMediaDevice(enumerator, dm));
	if (!media)
		return false;

	return false;
}

REGISTER_PIPELINE_HANDLER(PipelineHandlerMobirAir)

} /* namespace libcamera */
