#include "FontFamily.h"
#include "GpIOStream.h"
#include "IGpFileSystem.h"
#include "IGpFontHandler.h"
#include "IGpFont.h"
#include "IGpSystemServices.h"

#include "MemReaderStream.h"
#include "MemoryManager.h"
#include "PLDrivers.h"

#include <stdlib.h>
#include <new>

namespace PortabilityLayer
{
	FontFamily::FontSpec::FontSpec()
		: m_fontVDir(VirtualDirectories::kUnspecified)
		, m_fontPath(nullptr)
		, m_font(nullptr)
		, m_hacks(FontHacks_None)
		, m_typeFaceIndex(0)
		, m_isRegistered(false)
	{
	}

	void FontFamily::AddFont(int flags, VirtualDirectory_t vDir, const char *path, int typeFaceIndex, FontHacks fontHacks)
	{
		m_fontSpecs[flags].m_fontVDir = vDir;
		m_fontSpecs[flags].m_fontPath = path;
		m_fontSpecs[flags].m_typeFaceIndex = typeFaceIndex;
		m_fontSpecs[flags].m_hacks = fontHacks;
		m_fontSpecs[flags].m_isRegistered = true;

		if (!m_fontSpecs[0].m_isRegistered)
			m_defaultVariation = flags;
	}

	void FontFamily::SetDefaultVariation(int defaultVariation)
	{
		if (m_fontSpecs[defaultVariation].m_isRegistered)
			m_defaultVariation = defaultVariation;
	}

	int FontFamily::GetVariationForFlags(int flags) const
	{
		if (m_fontSpecs[flags].m_isRegistered)
			return flags;

		if (m_fontSpecs[0].m_isRegistered)
			return 0;

		return m_defaultVariation;
	}

	IGpFont *FontFamily::GetFontForVariation(int variation)
	{
		FontSpec &spec = m_fontSpecs[variation];
		if (spec.m_font)
			return spec.m_font;

		GpIOStream *sysFontStream = PLDrivers::GetFileSystem()->OpenFile(spec.m_fontVDir, spec.m_fontPath, false, GpFileCreationDispositions::kOpenExisting);
		if (!sysFontStream)
			return nullptr;

		if (!PLDrivers::GetSystemServices()->AreFontResourcesSeekable())
		{
			PortabilityLayer::MemoryManager *mm = PortabilityLayer::MemoryManager::GetInstance();

			size_t fontSize = sysFontStream->Size();
			void *buffer = mm->Alloc(fontSize);
			if (!buffer)
			{
				sysFontStream->Close();
				return nullptr;
			}

			MemBufferReaderStream *bufferStream = MemBufferReaderStream::Create(buffer, fontSize);
			if (!bufferStream)
			{
				mm->Release(buffer);
				sysFontStream->Close();
				return nullptr;
			}

			if (sysFontStream->Read(buffer, fontSize) != fontSize)
			{
				mm->Release(buffer);
				sysFontStream->Close();
				return nullptr;
			}

			sysFontStream->Close();

			sysFontStream = bufferStream;
		}

		IGpFontHandler *fontHandler = PLDrivers::GetFontHandler();

		IGpFont *font = fontHandler->LoadFont(sysFontStream, spec.m_typeFaceIndex);

		if (!fontHandler->KeepStreamOpen())
			sysFontStream->Close();

		if (!font)
			return nullptr;

		spec.m_font = font;

		return font;
	}

	void FontFamily::UnloadVariation(int variation)
	{
		FontSpec &spec = m_fontSpecs[variation];
		if (spec.m_font)
		{
			spec.m_font->Destroy();
			spec.m_font = nullptr;
		}
	}

	PortabilityLayer::FontHacks FontFamily::GetHacksForVariation(int variation) const
	{
		return m_fontSpecs[variation].m_hacks;
	}

	bool FontFamily::GetFontSpec(int variation, FontHacks &outHacks, VirtualDirectory_t &outVDir, const char *&outPath, int &outTypeFaceIndex)
	{
		const FontSpec &spec = m_fontSpecs[variation];
		if (!spec.m_isRegistered)
			return false;

		outHacks = spec.m_hacks;
		outVDir = spec.m_fontVDir;
		outPath = spec.m_fontPath;
		outTypeFaceIndex = spec.m_typeFaceIndex;

		return true;
	}

	FontFamilyID_t FontFamily::GetFamilyID() const
	{
		return m_familyID;
	}

	void FontFamily::PurgeCache()
	{
		for (unsigned int i = 0; i < kNumVariations; i++)
		{
			if (IGpFont *font = m_fontSpecs[i].m_font)
			{
				font->Destroy();
				m_fontSpecs[i].m_font = nullptr;
			}
		}
	}

	FontFamily *FontFamily::Create(FontFamilyID_t familyID)
	{
		void *storage = malloc(sizeof(FontFamily));
		if (!storage)
			return nullptr;

		return new (storage) FontFamily(familyID);
	}

	void FontFamily::Destroy()
	{
		this->~FontFamily();
		free(this);
	}

	FontFamily::FontFamily(FontFamilyID_t familyID)
		: m_defaultVariation(0)
		, m_familyID(familyID)
	{
	}

	FontFamily::~FontFamily()
	{
		PurgeCache();
	}
}
