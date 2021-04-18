#pragma once

class GpIOStream;
struct IGpFont;

struct IGpFontHandler
{
	virtual void Shutdown() = 0;

	virtual IGpFont *LoadFont(GpIOStream *stream, int typeFaceIndex) = 0;
	virtual bool KeepStreamOpen() const = 0;
};
