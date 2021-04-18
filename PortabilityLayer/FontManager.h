#pragma once

#include "FontHacks.h"
#include "FontFamilyID.h"
#include "FontPresets.h"

struct IGpFont;

namespace PortabilityLayer
{
	class FontFamily;
	class RenderedFont;

	class FontManager
	{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual FontFamily *GetFont(FontFamilyID_t fontFamilyID) const = 0;
		virtual void GetFontPreset(FontPreset_t fontPreset, FontFamilyID_t *outFamilyID, int *outSize, int *outVariationFlags, bool *outAA) const = 0;

		virtual RenderedFont *LoadCachedRenderedFont(FontFamilyID_t familyID, int size, bool aa, int flags) = 0;

		virtual void PurgeCache() = 0;

		static FontManager *GetInstance();
	};
}
