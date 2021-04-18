#pragma once

#include <stdint.h>

struct IGpFontRenderedGlyph;
struct GpRenderedFontMetrics;

struct IGpFont
{
	virtual void Destroy() = 0;
	virtual IGpFontRenderedGlyph *Render(uint32_t unicodeCodePoint, unsigned int size, unsigned int xScale, unsigned int yScale, bool aa) = 0;
	virtual bool GetLineSpacing(unsigned int size, int32_t &outSpacing) = 0;
	virtual bool SupportScaling() const = 0;
};
