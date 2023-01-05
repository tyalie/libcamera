/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2018, Google Inc.
 *
 * media_device.h - Media device handler
 */

#pragma once

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <linux/media.h>

#include <libcamera/base/log.h>
#include <libcamera/base/signal.h>
#include <libcamera/base/unique_fd.h>

#include "libcamera/internal/media_device_base.h"
#include "libcamera/internal/media_object.h"
#include "libcamera/internal/device_match.h"

namespace libcamera {

class MediaDevice : public MediaDeviceBase
{
public:
	MediaDevice(const std::string &deviceNode);
	~MediaDevice();

	bool lock() override;
	void unlock() override;

	int populate() override;

	const std::string &model() const { return model_; }
	unsigned int version() const { return version_; }
	unsigned int hwRevision() const { return hwRevision_; }

	MediaLink *link(const std::string &sourceName, unsigned int sourceIdx,
			const std::string &sinkName, unsigned int sinkIdx);
	MediaLink *link(const MediaEntity *source, unsigned int sourceIdx,
			const MediaEntity *sink, unsigned int sinkIdx);
	MediaLink *link(const MediaPad *source, const MediaPad *sink);
	int disableLinks();

private:
	int open() override;
	void close() override;

	MediaObject *object(unsigned int id);
	bool addObject(MediaObject *object);
	void clear() override;

	struct media_v2_interface *findInterface(const struct media_v2_topology &topology,
						 unsigned int entityId);
	bool populateEntities(const struct media_v2_topology &topology);
	bool populatePads(const struct media_v2_topology &topology);
	bool populateLinks(const struct media_v2_topology &topology);
	void fixupEntityFlags(struct media_v2_entity *entity);

	friend int MediaLink::setEnabled(bool enable);
	int setupLink(const MediaLink *link, unsigned int flags);

	std::string model_;
	unsigned int version_;
	unsigned int hwRevision_;

	UniqueFD fd_;

	std::map<unsigned int, MediaObject *> objects_;
};

} /* namespace libcamera */
