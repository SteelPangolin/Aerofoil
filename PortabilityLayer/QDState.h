#pragma once

#include "RGBAColor.h"
#include "SharedTypes.h"

struct Region;

namespace PortabilityLayer
{
	struct QDState
	{
		QDState();

		int m_fontID;
		int m_textFace;
		int m_textSize;
		Region **m_clipRegion;
		Point m_penPos;

		void SetForeColor(const RGBAColor &color);
		void SetBackColor(const RGBAColor &color);

		const RGBAColor &GetForeColor() const;
		const RGBAColor &GetBackColor() const;

		uint8_t ResolveForeColor8(const RGBAColor *palette, unsigned int numColors);
		uint8_t ResolveBackColor8(const RGBAColor *palette, unsigned int numColors);

	private:
		static uint8_t ResolveColor8(const RGBAColor &color, uint8_t &cached, bool &isCached, const RGBAColor *palette, unsigned int numColors);

		RGBAColor m_foreUnresolvedColor;
		RGBAColor m_backUnresolvedColor;

		uint16_t m_foreResolvedColor16;
		uint16_t m_backResolvedColor16;
		uint8_t m_foreResolvedColor8;
		uint8_t m_backResolvedColor8;

		bool m_isForeResolved16;
		bool m_isForeResolved8;
		bool m_isBackResolved16;
		bool m_isBackResolved8;
	};
}