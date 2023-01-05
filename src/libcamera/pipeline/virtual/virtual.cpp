/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2022, Google Inc.
 *
 * fake.cpp - Pipeline handler for fake cameras
 */

#include <libcamera/base/log.h>

#include <libcamera/camera.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/media_device_virtual.h"
#include "libcamera/internal/pipeline_handler.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(VIRTUAL)

class VirtualCameraData : public Camera::Private
{
public:
	VirtualCameraData(PipelineHandler *pipe)
		: Camera::Private(pipe)
	{
	}

	~VirtualCameraData() = default;

	Stream stream_;
};

class VirtualCameraConfiguration : public CameraConfiguration
{
public:
	VirtualCameraConfiguration();

	Status validate() override;
};

class PipelineHandlerVirtual : public PipelineHandler
{
public:
	PipelineHandlerVirtual(CameraManager *manager);

	std::unique_ptr<CameraConfiguration> generateConfiguration(Camera *camera,
								   const StreamRoles &roles) override;
	int configure(Camera *camera, CameraConfiguration *config) override;

	int exportFrameBuffers(Camera *camera, Stream *stream,
			       std::vector<std::unique_ptr<FrameBuffer>> *buffers) override;

	int start(Camera *camera, const ControlList *controls) override;
	void stopDevice(Camera *camera) override;

	int queueRequestDevice(Camera *camera, Request *request) override;

	bool match(DeviceEnumerator *enumerator) override;

private:
	std::shared_ptr<MediaDeviceVirtual> mediaDeviceVirtual_;
};

VirtualCameraConfiguration::VirtualCameraConfiguration()
	: CameraConfiguration()
{
}

CameraConfiguration::Status VirtualCameraConfiguration::validate()
{
	return Invalid;
}

PipelineHandlerVirtual::PipelineHandlerVirtual(CameraManager *manager)
	: PipelineHandler(manager), mediaDeviceVirtual_(new MediaDeviceVirtual("virtual"))
{
}

std::unique_ptr<CameraConfiguration> PipelineHandlerVirtual::generateConfiguration(Camera *camera,
										   const StreamRoles &roles)
{
	(void)camera;
	(void)roles;
	return std::unique_ptr<VirtualCameraConfiguration>(nullptr);
}

int PipelineHandlerVirtual::configure(Camera *camera, CameraConfiguration *config)
{
	(void)camera;
	(void)config;
	return -1;
}

int PipelineHandlerVirtual::exportFrameBuffers(Camera *camera, Stream *stream,
					       std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	(void)camera;
	(void)stream;
	(void)buffers;
	return -1;
}

int PipelineHandlerVirtual::start(Camera *camera, const ControlList *controls)
{
	(void)camera;
	(void)controls;
	return -1;
}

void PipelineHandlerVirtual::stopDevice(Camera *camera)
{
	(void)camera;
}

int PipelineHandlerVirtual::queueRequestDevice(Camera *camera, Request *request)
{
	(void)camera;
	(void)request;
	return -1;
}

bool PipelineHandlerVirtual::match(DeviceEnumerator *enumerator)
{
	(void)enumerator;
	mediaDevices_.push_back(mediaDeviceVirtual_);

	std::unique_ptr<VirtualCameraData> data = std::make_unique<VirtualCameraData>(this);
	/* Create and register the camera. */
	std::set<Stream *> streams{ &data->stream_ };
	const std::string id = "Virtual0";
	std::shared_ptr<Camera> camera = Camera::create(std::move(data), id, streams);
	registerCamera(std::move(camera));

	return false; // Prevent infinite loops for now
}

REGISTER_PIPELINE_HANDLER(PipelineHandlerVirtual)

} /* namespace libcamera */
