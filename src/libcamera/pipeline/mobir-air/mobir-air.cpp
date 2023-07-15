/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Sophie Friedrich
 *
 * mobir-air.cpp - Pipeline handler for MobirAir thermal camera
 */
#include <libcamera/base/log.h>

#include <libcamera/camera.h>
#include <libcamera/formats.h>
#include <libcamera/controls.h>
#include <libcamera/control_ids.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/media_device_usb.h"
#include "libcamera/internal/pipeline_handler.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(MOBIR_AIR)

class MobirAirCameraData : public Camera::Private
{
public:
	MobirAirCameraData(PipelineHandler *pipe, MediaDeviceUSB *device)
		: Camera::Private(pipe), device_(device), tmp_fd_(nullptr)
	{
	}

	int init();
	void bufferReady(FrameBuffer *buffer);

	FILE *getFD();

	size_t bufferSize()
	{
		return 120 * 90 * sizeof(uint16_t);
	}

	void closeFD()
	{
		fclose(tmp_fd_);
		tmp_fd_ = nullptr;
	}

	MediaDeviceUSB *device_;
	Stream stream_;
	std::map<PixelFormat, std::vector<SizeRange>> formats_;

private:
	FILE *initFD();

	FILE *tmp_fd_;
};

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

private:
	MobirAirCameraData *cameraData(Camera *camera)
	{
		return static_cast<MobirAirCameraData *>(camera->_d());
	}

};

MobirAirCameraConfiguration::MobirAirCameraConfiguration()
	: CameraConfiguration()
{
}

CameraConfiguration::Status MobirAirCameraConfiguration::validate()
{
	Status status = Valid;

	if (config_.empty())
		return Invalid;

	if (config_.size() > 1) {
		config_.resize(1);
		status = Adjusted;
	}

	StreamConfiguration &cfg = config_[0];

	const std::vector<PixelFormat> formats = cfg.formats().pixelformats();
	auto iter = std::find(formats.begin(), formats.end(), cfg.pixelFormat);
	if (iter == formats.end()) {
		cfg.pixelFormat = formats.front();
		LOG(MOBIR_AIR, Debug)
			<< "Adjusted pixel format to " << cfg.pixelFormat.toString();
		status = Adjusted;
	}

	const std::vector<Size> &formatSizes = cfg.formats().sizes(cfg.pixelFormat);
	auto iter2 = std::find(formatSizes.begin(), formatSizes.end(), cfg.size);
	if (iter2 == formatSizes.end()) {
		cfg.size = formatSizes.front();
		LOG(MOBIR_AIR, Debug)
			<< "Adjusted size to " << cfg.size;
		status = Adjusted;
	}

	cfg.bufferCount = 1;
	cfg.stride = cfg.size.width * uint16_t(2);

	return status;
}

PipelineHandlerMobirAir::PipelineHandlerMobirAir(CameraManager *manager)
	: PipelineHandler(manager)
{
}

std::unique_ptr<CameraConfiguration>
PipelineHandlerMobirAir::generateConfiguration(Camera *camera,
					       const StreamRoles &roles)
{
	MobirAirCameraData *data = cameraData(camera);
	std::unique_ptr<CameraConfiguration> config =
		std::make_unique<MobirAirCameraConfiguration>();

	if (roles.empty())
		return config;

	StreamFormats formats(data->formats_);
	StreamConfiguration cfg(formats);

	cfg.pixelFormat = formats.pixelformats().front();
	cfg.size = formats.sizes(cfg.pixelFormat).back();
	cfg.bufferCount = 4;

	config->addConfiguration(cfg);
	config->validate();

	return config;
}

int PipelineHandlerMobirAir::configure(Camera *camera, CameraConfiguration *config)
{
	// TODO: do I need to configure something??
	MobirAirCameraData *data = cameraData(camera);
	StreamConfiguration &cfg = config->at(0);

	cfg.setStream(&data->stream_);

	return 0;
}

int PipelineHandlerMobirAir::exportFrameBuffers(Camera *camera, Stream *stream,
						std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	MobirAirCameraData *data = cameraData(camera);
	int count = stream->configuration().bufferCount;
	assert(count == 1);

	for (int i = 0; i < count; ++i) {
		FrameBuffer::Plane p{
			.fd = SharedFD(fileno(data->getFD())),
			.offset = 0,
			.length = (int)data->bufferSize()
		};
		std::vector<FrameBuffer::Plane> ps{ p };

		std::unique_ptr<FrameBuffer> buffer = std::make_unique<FrameBuffer>(ps);

		buffers->push_back(std::move(buffer));
	}

	return 0;
}

int PipelineHandlerMobirAir::start(Camera *camera, [[maybe_unused]] const ControlList *controls)
{
	MobirAirCameraData *data = cameraData(camera);

	data->getFD();

	return 0;
}

void PipelineHandlerMobirAir::stopDevice(Camera *camera)
{
	MobirAirCameraData *data = cameraData(camera);
	data->closeFD();

	LOG(MOBIR_AIR, Debug) << "Unregistering device";
}

int PipelineHandlerMobirAir::queueRequestDevice(Camera *camera, Request *request)
{
	MobirAirCameraData *data = cameraData(camera);
	FrameBuffer *buffer = request->findBuffer(&data->stream_);
	if (!buffer) {
		LOG(MOBIR_AIR, Error)
			<< "Attempt to queue request with invalid stream";
		return -ENOENT;
	}

	FILE *fd = data->getFD();
	rewind(fd);

	uint16_t tbuffer[120 * 90] = { 0 };
	int32_t dx, dy;
	for (uint16_t x = 0; x < 90; x++) {
		for (uint16_t y = 0; y < 120; y++) {
			// (x*x + y*y - 10*10 == 0) ? 24000 : 0;
			dx = (int16_t)x - 45;
			dy = (int16_t)y - 60;
			tbuffer[x * 120 + y] = (uint16_t)(128 - ((dx * dx + dy * dy - 30 * 30) / 20));
		}
	}
	fwrite(reinterpret_cast<const char *>(tbuffer), 120 * 90, sizeof(uint16_t), fd);
	fflush(fd);

	data->bufferReady(buffer);

	return 0;
}

bool PipelineHandlerMobirAir::match(DeviceEnumerator *enumerator)
{
	MediaDeviceUSB *media;
	DeviceMatch dm("libusb");
	dm.add("0525:a4a0");

	media = dynamic_cast<MediaDeviceUSB *>(acquireMediaDevice(enumerator, dm));
	if (!media)
		return false;

	std::unique_ptr<MobirAirCameraData> data = std::make_unique<MobirAirCameraData>(this, media);

	if (data->init())
		return false;

	/* Create and register camera */
	std::string id = "test device";
	std::set<Stream *> streams{ &data->stream_ };
	std::shared_ptr<Camera> camera = Camera::create(std::move(data), id, streams);
	registerCamera(std::move(camera));

	// enable hotplug notification
	hotplugMediaDevice(media);

	return true;
}

FILE *MobirAirCameraData::initFD()
{
	FILE *f = tmpfile();

	for (size_t i = 0; i < bufferSize(); i++)
		fputc(0, f);

	rewind(f);

	return f;
}

FILE *MobirAirCameraData::getFD()
{
	if (!tmp_fd_)
		tmp_fd_ = initFD();

	return tmp_fd_;
}

void MobirAirCameraData::bufferReady(FrameBuffer *buffer)
{
	Request *request = buffer->request();

	pipe()->completeBuffer(request, buffer);
	pipe()->completeRequest(request);
}

int MobirAirCameraData::init()
{
	// initialize ctrl map
	ControlInfoMap::Map ctrls;
	// TODO
	controlInfo_ = ControlInfoMap(std::move(ctrls), controls::controls);

	// TODO: set properties

	// TODO: improve formats read
	formats_[PixelFormat::fromString("R16")]
		.push_back(SizeRange({120, 90}));

	return 0;
}

REGISTER_PIPELINE_HANDLER(PipelineHandlerMobirAir)

} /* namespace libcamera */
