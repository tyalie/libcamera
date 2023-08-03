/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Sophie 'Tyalie' Friedrich
 *
 * mobir-air.cpp - Pipeline handler for MobirAir thermal camera
 */
#include <math.h>

#include <libcamera/base/log.h>

#include <libcamera/camera.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/framebuffer.h"

#include "mobir-manager.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(MOBIR_AIR)

class MobirAirCameraData : public Camera::Private
{
public:
	MobirAirCameraData(PipelineHandler *pipe, MediaDeviceUSB *device)
		: Camera::Private(pipe), device_(device)
	{
		manager = std::make_unique<MobirCameraManager>(device_);
	}

	int init();
	void bufferReady(FrameBuffer *buffer);


	size_t bufferSize()
	{
		return 120 * 90 * sizeof(uint16_t);
	}

	size_t headerSize()
	{
		return 120 * 3 * sizeof(uint16_t);
	}

	FILE *initFD();

	MediaDeviceUSB *device_;
	std::unique_ptr<MobirCameraManager> manager;
	Stream stream_;
	std::map<PixelFormat, std::vector<SizeRange>> formats_;
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

	// open device
	int ret = data->manager->init();
	if (ret) {
		LOG(MOBIR_AIR, Error) << "Couldn't init device (errno: " << ret << ")";
		return ret;
	}

	cfg.setStream(&data->stream_);

	return 0;
}

int PipelineHandlerMobirAir::exportFrameBuffers(Camera *camera, Stream *stream,
						std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	MobirAirCameraData *data = cameraData(camera);
	int count = stream->configuration().bufferCount;

	LOG(MOBIR_AIR, Debug) << count << " buffer(s) requested";

	for (int i = 0; i < count; ++i) {
		FrameBuffer::Plane p{
			.fd = SharedFD(fileno(data->initFD())),
			.offset = 0,
			.length = (unsigned int)data->bufferSize()
		};
		std::vector<FrameBuffer::Plane> ps{ p };

		std::unique_ptr<FrameBuffer> buffer = std::make_unique<FrameBuffer>(ps);

		buffers->push_back(std::move(buffer));
	}

	return 0;
}

int PipelineHandlerMobirAir::start(Camera *camera, [[maybe_unused]] const ControlList *controls)
{
	return 0;
}

void PipelineHandlerMobirAir::stopDevice(Camera *camera)
{
	MobirAirCameraData *data = cameraData(camera);

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

	static int idx = 0;

	FILE *fd = fdopen(buffer->planes()[0].fd.get(), "w");
	rewind(fd);

	MobirAirUSBWrapper::Request req;
	req.expected_length = data->bufferSize() + data->headerSize();
	data->manager->_usb->doRequest(&req);

	req.output[(idx++ * 2) % (data->bufferSize()) + 1 + data->headerSize()] = 255;

	fwrite(
		reinterpret_cast<const char *>(req.output.data()) + data->headerSize(),
		120 * 90, sizeof(uint16_t), fd);

	fflush(fd);

	FrameMetadata &metadata = buffer->_d()->metadata();
	metadata.status = FrameMetadata::FrameSuccess;
	metadata.planes()[0].bytesused = data->bufferSize();
	metadata.sequence = idx;
	metadata.timestamp = 0;

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

	if (!f)
		return 0;

	for (size_t i = 0; i < bufferSize(); i++)
		fputc(0, f);

	rewind(f);

	return f;
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
