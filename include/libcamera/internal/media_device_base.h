/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2022, Google Inc.
 *
 * media_device.h - The base class of media device handler
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

#include "libcamera/internal/media_object.h"

namespace libcamera {

class MediaDeviceBase : protected Loggable
{
public:
	MediaDeviceBase(const std::string &deviceNode);
	~MediaDeviceBase();

	bool acquire();
	void release();
	bool busy() const { return acquired_; }

	virtual bool lock();
	virtual void unlock();

	virtual int populate();
	bool isValid() const { return valid_; }

	const std::string &driver() const { return driver_; }
	const std::string &deviceNode() const { return deviceNode_; }

	const std::vector<MediaEntity *> &entities() const { return entities_; }
	MediaEntity *getEntityByName(const std::string &name) const;

	Signal<> disconnected;

protected:
	std::string logPrefix() const override;

	virtual int open() { return 0; }
	virtual void close() {}
	virtual void clear();

	std::string driver_;
	std::string deviceNode_;

	bool valid_;
	bool acquired_;

	std::vector<MediaEntity *> entities_;

private:
	bool lock_;
};

} /* namespace libcamera */
