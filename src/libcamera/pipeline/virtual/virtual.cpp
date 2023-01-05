/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2022, Google Inc.
 *
 * fake.cpp - Pipeline handler for fake cameras
 */

#include <libcamera/base/log.h>

#include <libcamera/camera.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/media_device_virtual.h"
#include "libcamera/internal/pipeline_handler.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(VIRTUAL)

uint64_t CurrentTimestamp()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
		LOG(VIRTUAL, Error) << "Get clock time fails";
		return 0;
	}

	return ts.tv_sec * 1'000'000'000LL + ts.tv_nsec;
}

class VirtualCameraData : public Camera::Private
{
public:
	struct Resolution {
		Size size;
		std::vector<int> frame_rates;
		std::vector<std::string> formats;
	};
	VirtualCameraData(PipelineHandler *pipe)
		: Camera::Private(pipe)
	{
	}

	~VirtualCameraData() = default;

	std::vector<Resolution> supportedResolutions_;

	Stream stream_;
};

class VirtualCameraConfiguration : public CameraConfiguration
{
public:
	static constexpr unsigned int kBufferCount = 4; // 4~6

	VirtualCameraConfiguration(VirtualCameraData *data);

	Status validate() override;

private:
	const VirtualCameraData *data_;
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
	VirtualCameraData *cameraData(Camera *camera)
	{
		return static_cast<VirtualCameraData *>(camera->_d());
	}

	std::shared_ptr<MediaDeviceVirtual> mediaDeviceVirtual_;
};

VirtualCameraConfiguration::VirtualCameraConfiguration(VirtualCameraData *data)
	: CameraConfiguration(), data_(data)
{
}

CameraConfiguration::Status VirtualCameraConfiguration::validate()
{
	Status status = Valid;

	if (config_.empty()) {
		LOG(VIRTUAL, Error) << "Empty config";
		return Invalid;
	}

	// TODO: check if we should limit |config_.size()|

	Size maxSize;
	for (const auto &resolution : data_->supportedResolutions_)
		maxSize = std::max(maxSize, resolution.size);

	for (StreamConfiguration &cfg : config_) {
		// TODO: check |cfg.pixelFormat|.

		bool found = false;
		for (const auto &resolution : data_->supportedResolutions_) {
			if (resolution.size.width >= cfg.size.width && resolution.size.height >= cfg.size.height) {
				found = true;
				break;
			}
		}

		if (!found) {
			cfg.size = maxSize;
			status = Adjusted;
		}

		cfg.setStream(const_cast<Stream *>(&data_->stream_));

		cfg.bufferCount = VirtualCameraConfiguration::kBufferCount;
	}

	return status;
}

PipelineHandlerVirtual::PipelineHandlerVirtual(CameraManager *manager)
	: PipelineHandler(manager), mediaDeviceVirtual_(new MediaDeviceVirtual("virtual"))
{
}

std::unique_ptr<CameraConfiguration> PipelineHandlerVirtual::generateConfiguration(Camera *camera,
										   const StreamRoles &roles)
{
	VirtualCameraData *data = cameraData(camera);
	auto config =
		std::make_unique<VirtualCameraConfiguration>(data);

	if (roles.empty())
		return config;

	Size minSize, sensorResolution;
	for (const auto &resolution : data->supportedResolutions_) {
		if (minSize.isNull() || minSize > resolution.size)
			minSize = resolution.size;

		sensorResolution = std::max(sensorResolution, resolution.size);
	}

	for (const StreamRole role : roles) {
		std::map<PixelFormat, std::vector<SizeRange>> streamFormats;
		unsigned int bufferCount;
		PixelFormat pixelFormat;

		switch (role) {
		case StreamRole::StillCapture:
			pixelFormat = formats::NV12;
			bufferCount = VirtualCameraConfiguration::kBufferCount;
			streamFormats[pixelFormat] = { { minSize, sensorResolution } };

			break;

		case StreamRole::Raw: {
			// TODO: check
			pixelFormat = formats::SBGGR10;
			bufferCount = VirtualCameraConfiguration::kBufferCount;
			streamFormats[pixelFormat] = { { minSize, sensorResolution } };

			break;
		}

		case StreamRole::Viewfinder:
		case StreamRole::VideoRecording: {
			pixelFormat = formats::NV12;
			bufferCount = VirtualCameraConfiguration::kBufferCount;
			streamFormats[pixelFormat] = { { minSize, sensorResolution } };

			break;
		}

		default:
			LOG(VIRTUAL, Error)
				<< "Requested stream role not supported: " << role;
			config.reset();
			return config;
		}

		StreamFormats formats(streamFormats);
		StreamConfiguration cfg(formats);
		cfg.size = sensorResolution;
		cfg.pixelFormat = pixelFormat;
		cfg.bufferCount = bufferCount;
		config->addConfiguration(cfg);
	}

	if (config->validate() == CameraConfiguration::Invalid) {
		config.reset();
		return config;
	}

	return config;
}

int PipelineHandlerVirtual::configure(Camera *camera, CameraConfiguration *config)
{
	(void)camera;
	(void)config;
	// Nothing to be done.
	return 0;
}

int PipelineHandlerVirtual::exportFrameBuffers(Camera *camera, Stream *stream,
					       std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	(void)camera;
	(void)stream;
	(void)buffers;
	// TODO: Use UDMA to allocate buffers.
	return -1;
}

int PipelineHandlerVirtual::start(Camera *camera, const ControlList *controls)
{
	(void)camera;
	(void)controls;
	// TODO: Start reading the virtual video if any.
	return 0;
}

void PipelineHandlerVirtual::stopDevice(Camera *camera)
{
	(void)camera;
	// TODO: Reset the virtual video if any.
}

int PipelineHandlerVirtual::queueRequestDevice(Camera *camera, Request *request)
{
	(void)camera;

	// TODO: Read from the virtual video if any.
	for (auto it : request->buffers())
		completeBuffer(request, it.second);

	request->metadata().set(controls::SensorTimestamp, CurrentTimestamp());
	completeRequest(request);

	return 0;
}

bool PipelineHandlerVirtual::match(DeviceEnumerator *enumerator)
{
	(void)enumerator;
	mediaDevices_.push_back(mediaDeviceVirtual_);

	// TODO: Add virtual cameras according to a config file.

	std::unique_ptr<VirtualCameraData> data = std::make_unique<VirtualCameraData>(this);

	data->supportedResolutions_.resize(2);
	data->supportedResolutions_[0] = { .size = Size(1920, 1080), .frame_rates = { 30 }, .formats = { "YCbCr_420_888" } };
	data->supportedResolutions_[1] = { .size = Size(1280, 720), .frame_rates = { 30, 60 }, .formats = { "YCbCr_420_888" } };

	/* Create and register the camera. */
	std::set<Stream *> streams{ &data->stream_ };
	const std::string id = "Virtual0";
	std::shared_ptr<Camera> camera = Camera::create(std::move(data), id, streams);
	registerCamera(std::move(camera));

	return false; // Prevent infinite loops for now
}

REGISTER_PIPELINE_HANDLER(PipelineHandlerVirtual)

} /* namespace libcamera */
