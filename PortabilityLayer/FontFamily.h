#pragma once

#include "FontFamilyID.h"
#include "FontHacks.h"
#include "VirtualDirectory.h"

#include <stdint.h>

class PLPasStr;
struct IGpFont;

namespace PortabilityLayer
{
	enum FontFamilyFlags
	{
		FontFamilyFlag_None = 0,

		FontFamilyFlag_Bold = 1,

		FontFamilyFlag_All = 1,
	};

	class FontFamily final
	{
	public:
		static const unsigned int kNumVariations = FontFamilyFlag_All + 1;

		void AddFont(int flags, VirtualDirectory_t vDir, const char *path, int typeFaceIndex, FontHacks fontHacks);
		void SetDefaultVariation(int defaultVariation);
		bool GetFontSpec(int variation, FontHacks &outHacks, VirtualDirectory_t &outVDir, const char *&outPath, int &outTypeFaceIndex);

		int GetVariationForFlags(int flags) const;
		IGpFont *GetFontForVariation(int variation);
		void UnloadVariation(int variation);
		FontHacks GetHacksForVariation(int variation) const;

		FontFamilyID_t GetFamilyID() const;

		void PurgeCache();

		static FontFamily *Create(FontFamilyID_t familyID);
		void Destroy();

	private:
		struct FontSpec
		{
			FontSpec();

			IGpFont *m_font;
			FontHacks m_hacks;
			VirtualDirectory_t m_fontVDir;
			const char *m_fontPath;
			int m_typeFaceIndex;
			bool m_isRegistered;
		};

		FontSpec m_fontSpecs[kNumVariations];
		uint8_t m_defaultVariation;
		FontFamilyID_t m_familyID;

		explicit FontFamily(FontFamilyID_t familyID);
		~FontFamily();
	};
}
