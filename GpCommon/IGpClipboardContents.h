#pragma once

#include "GpClipboardContentsType.h"

#include <stdint.h>
#include <stddef.h>

struct IGpClipboardContents
{
	virtual GpClipboardContentsType_t GetContentsType() const = 0;
	virtual void Destroy() = 0;
	virtual IGpClipboardContents *Clone() const = 0;
};

struct IGpClipboardContentsText : public IGpClipboardContents
{
	virtual const uint8_t *GetBytes() const = 0;
	virtual size_t GetSize() const = 0;				// In bytes
};
