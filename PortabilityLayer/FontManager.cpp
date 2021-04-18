#include "FontManager.h"
#include "FontFamily.h"
#include "FontRenderer.h"

#include "FileManager.h"
#include "GpIOStream.h"
#include "IGpFileSystem.h"
#include "IGpFont.h"
#include "IGpSystemServices.h"

#include "MemReaderStream.h"
#include "PLBigEndian.h"
#include "PLDrivers.h"
#include "RenderedFontCatalog.h"
#include "RenderedFont.h"
#include "ResTypeID.h"
#include "ResourceManager.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void PL_NotYetImplemented();

namespace PortabilityLayer
{
	class FontManagerImpl final : public FontManager
	{
	public:
		void Init() override;
		void Shutdown() override;

		FontFamily *GetFont(FontFamilyID_t fontFamilyID) const override;
		void GetFontPreset(FontPreset_t fontPreset, FontFamilyID_t *outFamilyID, int *outSize, int *outVariationFlags, bool *outAA) const override;

		RenderedFont *LoadCachedRenderedFont(FontFamilyID_t familyID, int size, bool aa, int flags) override;

		void PurgeCache() override;

		static FontManagerImpl *GetInstance();

	private:
		RenderedFont *LoadAndRenderFontUsingFontHandler(FontFamily *font, int size, bool aa, int flags);
		RenderedFont *LoadAndRenderFont(FontFamilyID_t familyID, int size, bool aa, int flags);

		static const unsigned int kNumCachedRenderedFonts = 32;

		struct CachedRenderedFont
		{
			RenderedFont *m_rfont;
			int m_fontCacheID;
			int m_size;
			uint32_t m_lastUsage;
			bool m_aa;
		};

		struct FontPreset
		{
			FontFamilyID_t m_familyID;
			int m_textSize;
			int m_variationFlags;
			bool m_aa;
		};

		FontManagerImpl();

		bool FindOrReserveCacheSlot(int cacheID, int size, bool aa, CachedRenderedFont *&outCacheSlot, RenderedFont *&outRF);
		void ReplaceCachedRenderedFont(CachedRenderedFont &cacheSlot, RenderedFont *rfont, int cacheID, int size, bool aa, int flags);

		void ResetUsageCounter();
		static int CRFSortPredicate(const void *a, const void *b);

		FontFamily *m_fontFamilies[FontFamilyIDs::kCount];
		uint32_t m_usageCounter;

		CachedRenderedFont m_cachedRenderedFonts[kNumCachedRenderedFonts];

		IResourceArchive *m_fontArchive;
		PortabilityLayer::CompositeFile *m_fontArchiveFile;
		THandle<void> m_fontArchiveCatalogData;

		bool m_hasPreinstalledFonts;

		static FontManagerImpl ms_instance;
		static FontPreset ms_fontPresets[FontPresets::kCount];
	};

	void FontManagerImpl::Init()
	{
		for (int i = 0; i < FontFamilyIDs::kCount; i++)
			m_fontFamilies[static_cast<FontFamilyID_t>(i)] = FontFamily::Create(static_cast<FontFamilyID_t>(i));

		if (m_fontFamilies[FontFamilyIDs::kSystem])
			m_fontFamilies[FontFamilyIDs::kSystem]->AddFont(FontFamilyFlag_None, VirtualDirectories::kFonts, "Fonts/OpenSans/OpenSans-ExtraBold.ttf", 0, FontHacks_None);

		if (m_fontFamilies[FontFamilyIDs::kApplication])
		{
			m_fontFamilies[FontFamilyIDs::kApplication]->AddFont(FontFamilyFlag_None, VirtualDirectories::kFonts, "Fonts/OpenSans/OpenSans-SemiBold.ttf", 0, FontHacks_None);
			m_fontFamilies[FontFamilyIDs::kApplication]->AddFont(FontFamilyFlag_Bold, VirtualDirectories::kFonts, "Fonts/OpenSans/OpenSans-Bold.ttf", 0, FontHacks_None);
		}

		if (m_fontFamilies[FontFamilyIDs::kHandwriting])
			m_fontFamilies[FontFamilyIDs::kHandwriting]->AddFont(FontFamilyFlag_None, VirtualDirectories::kFonts, "Fonts/GochiHand/GochiHand-Regular.ttf", 0, FontHacks_None);

		if (m_fontFamilies[FontFamilyIDs::kMonospace])
			m_fontFamilies[FontFamilyIDs::kMonospace]->AddFont(FontFamilyFlag_None, VirtualDirectories::kFonts, "Fonts/Roboto/RobotoMono-Regular.ttf", 0, FontHacks_None);

		memset(m_cachedRenderedFonts, 0, sizeof(m_cachedRenderedFonts));
	}

	void FontManagerImpl::Shutdown()
	{
		for (int fid = 0; fid < FontFamilyIDs::kCount; fid++)
		{
			if (m_fontFamilies[fid])
				m_fontFamilies[fid]->Destroy();
		}

		IGpFontHandler *hfh = PLDrivers::GetFontHandler();

		for (int i = 0; i < sizeof(m_cachedRenderedFonts) / sizeof(m_cachedRenderedFonts[0]); i++)
		{
			CachedRenderedFont *crf = m_cachedRenderedFonts + i;
			if (crf->m_rfont)
				crf->m_rfont->Destroy();
		}
	}

	FontFamily *FontManagerImpl::GetFont(FontFamilyID_t fontFamilyID) const
	{
		return m_fontFamilies[fontFamilyID];
	}

	void FontManagerImpl::GetFontPreset(FontPreset_t preset, FontFamilyID_t *outFamilyID, int *outSize, int *outVariationFlags, bool *outAA) const
	{
		if (outSize)
			*outSize = ms_fontPresets[preset].m_textSize;

		if (outVariationFlags)
			*outVariationFlags = ms_fontPresets[preset].m_variationFlags;

		if (outAA)
			*outAA = ms_fontPresets[preset].m_aa;

		if (outFamilyID)
		{
			switch (ms_fontPresets[preset].m_familyID)
			{
			case FontFamilyIDs::kApplication:
				if (!m_hasPreinstalledFonts && ms_fontPresets[preset].m_textSize < 11 && (ms_fontPresets[preset].m_variationFlags & FontFamilyFlag_Bold) != 0)
					*outFamilyID = FontFamilyIDs::kSystem;	// Use heavier font below 11pt
				else
					*outFamilyID = FontFamilyIDs::kApplication;
				break;
			default:
				*outFamilyID = ms_fontPresets[preset].m_familyID;
				break;
			}
		}
	}

	bool FontManagerImpl::FindOrReserveCacheSlot(int cacheID, int size, bool aa, CachedRenderedFont *&outCacheSlot, RenderedFont *&outRF)
	{
		CachedRenderedFont *newCacheSlot = &m_cachedRenderedFonts[0];

		for (int i = 0; i < kNumCachedRenderedFonts; i++)
		{
			CachedRenderedFont &crf = m_cachedRenderedFonts[i];
			if (crf.m_rfont == nullptr)
			{
				newCacheSlot = &crf;
				break;
			}

			if (crf.m_fontCacheID == cacheID && crf.m_size == size && crf.m_aa == aa)
			{
				crf.m_lastUsage = m_usageCounter;
				RenderedFont *rf = crf.m_rfont;
				if (m_usageCounter == UINT32_MAX)
					ResetUsageCounter();
				else
					m_usageCounter++;

				outRF = rf;
				return true;
			}

			if (newCacheSlot->m_rfont != nullptr && crf.m_lastUsage < newCacheSlot->m_lastUsage)
				newCacheSlot = &crf;
		}

		outCacheSlot = newCacheSlot;
		return false;
	}

	void FontManagerImpl::ReplaceCachedRenderedFont(CachedRenderedFont &cacheSlot, RenderedFont *rfont, int cacheID, int size, bool aa, int flags)
	{
		if (cacheSlot.m_rfont)
			cacheSlot.m_rfont->Destroy();

		cacheSlot.m_fontCacheID = cacheID;
		cacheSlot.m_lastUsage = m_usageCounter;
		cacheSlot.m_size = size;
		cacheSlot.m_rfont = rfont;
		cacheSlot.m_aa = aa;

		if (m_usageCounter == UINT32_MAX)
			ResetUsageCounter();
		else
			m_usageCounter++;
	}

	RenderedFont *FontManagerImpl::LoadAndRenderFontUsingFontHandler(FontFamily *fontFamily, int size, bool aa, int flags)
	{
		PortabilityLayer::FontManager *fm = PortabilityLayer::FontManager::GetInstance();

		const int variation = fontFamily->GetVariationForFlags(flags);

		IGpFont *hostFont = fontFamily->GetFontForVariation(variation);
		if (!hostFont)
			return nullptr;

		RenderedFont *rfont = FontRenderer::GetInstance()->RenderFont(hostFont, size, aa, fontFamily->GetHacksForVariation(variation));
		fontFamily->UnloadVariation(variation);

		return rfont;
	}

	RenderedFont *FontManagerImpl::LoadCachedRenderedFont(FontFamilyID_t familyID, int size, bool aa, int flags)
	{
		CachedRenderedFont *cacheSlot = nullptr;
		RenderedFont *rfont = nullptr;

		if (this->FindOrReserveCacheSlot(familyID, size, aa, cacheSlot, rfont))
			return rfont;

		rfont = LoadAndRenderFont(familyID, size, aa, flags);
		if (rfont)
			ReplaceCachedRenderedFont(*cacheSlot, rfont, familyID, size, aa, flags);

		return rfont;
	}

	RenderedFont *FontManagerImpl::LoadAndRenderFont(FontFamilyID_t familyID, int size, bool aa, int flags)
	{
		FontFamily *fontFamily = this->GetFont(familyID);

		RenderedFont *rfont = nullptr;
		if (PLDrivers::GetFontHandler() != nullptr)
		{
			rfont = LoadAndRenderFontUsingFontHandler(fontFamily, size, aa, flags);
			if (rfont != nullptr)
				return rfont;
		}

		if (!m_fontArchive)
		{
			m_fontArchiveFile = PortabilityLayer::FileManager::GetInstance()->OpenCompositeFile(VirtualDirectories::kApplicationData, PSTR("Fonts"));
			if (!m_fontArchiveFile)
				return nullptr;

			m_fontArchive = PortabilityLayer::ResourceManager::GetInstance()->LoadResFile(m_fontArchiveFile);
			if (!m_fontArchive)
			{
				m_fontArchiveFile->Close();
				return nullptr;
			}

			m_fontArchiveCatalogData = m_fontArchive->LoadResource('RFCT', 1000);
			if (!m_fontArchiveCatalogData)
			{
				m_fontArchive->Destroy();
				m_fontArchiveFile->Close();
				return nullptr;
			}
		}

		int variation = fontFamily->GetVariationForFlags(flags);

		FontHacks hacks = FontHacks_None;
		VirtualDirectory_t vDir = VirtualDirectories::kUnspecified;
		const char *path = nullptr;
		int typeFaceIndex = 0;
		if (!fontFamily->GetFontSpec(variation, hacks, vDir, path, typeFaceIndex))
			return nullptr;

		// Only single TTF fonts are supported by the prerendered font system currently
		if (vDir != VirtualDirectories::kFonts || typeFaceIndex != 0)
			return nullptr;

		size_t pathLen = strlen(path);

		// Parse font cache header
		const uint8_t *catalogBytes = static_cast<const uint8_t*>(*m_fontArchiveCatalogData);

		RenderedFontCatalogHeader catHeader;
		memcpy(&catHeader, catalogBytes, sizeof(catHeader));

		if (catHeader.m_version != RenderedFontCatalogHeader::kVersion)
			return nullptr;

		size_t numPaths = catHeader.m_numPaths;
		size_t numFonts = catHeader.m_numRFonts;

		const uint8_t *pathsBytes = catalogBytes + catHeader.m_pathsOffset;
		const uint8_t *pathEntryBytes = catalogBytes + sizeof(RenderedFontCatalogHeader);
		const uint8_t *fontEntryBytes = pathEntryBytes + sizeof(RenderedFontCatalogPathEntry) * numPaths;

		size_t pathIndex = numPaths;
		for (size_t i = 0; i < numPaths; i++)
		{
			RenderedFontCatalogPathEntry pathEntry;
			memcpy(&pathEntry, pathEntryBytes + i * sizeof(RenderedFontCatalogPathEntry), sizeof(RenderedFontCatalogPathEntry));

			if (pathEntry.m_pathSize != pathLen)
				continue;

			if (!memcmp(pathsBytes + pathEntry.m_pathOffset, path, pathLen))
			{
				pathIndex = i;
				break;
			}
		}

		if (pathIndex == numPaths)
			return nullptr;

		size_t fontIndex = numFonts;
		for (size_t i = 0; i < numFonts; i++)
		{
			RenderedFontCatalogRFontEntry fontEntry;
			memcpy(&fontEntry, fontEntryBytes + i * sizeof(RenderedFontCatalogRFontEntry), sizeof(RenderedFontCatalogRFontEntry));

			if (fontEntry.m_pathIndex == pathIndex && static_cast<FontHacks>(fontEntry.m_hacks) == hacks && fontEntry.m_fontSize == size && (fontEntry.m_isAA != 0) == aa)
			{
				fontIndex = i;
				break;
			}
		}

		if (fontIndex == numFonts)
			return nullptr;

		THandle<void> res = m_fontArchive->LoadResource('RFNT', 1000 + static_cast<int>(fontIndex));
		if (res)
		{
			PortabilityLayer::MemReaderStream stream(*res, res.MMBlock()->m_size);

			rfont = PortabilityLayer::FontRenderer::GetInstance()->LoadCache(&stream);
			res.Dispose();
		}

		return rfont;
	}

	void FontManagerImpl::PurgeCache()
	{
		for (int fid = 0; fid < FontFamilyIDs::kCount; fid++)
		{
			if (m_fontFamilies[fid])
				m_fontFamilies[fid]->PurgeCache();
		}
	}

	FontManagerImpl *FontManagerImpl::GetInstance()
	{
		return &ms_instance;
	}

	FontManagerImpl::FontManagerImpl()
		: m_fontArchive(nullptr)
		, m_fontArchiveFile(nullptr)
		, m_hasPreinstalledFonts(false)
	{
		for (int fid = 0; fid < FontFamilyIDs::kCount; fid++)
			m_fontFamilies[fid] = nullptr;
	}

	void FontManagerImpl::ResetUsageCounter()
	{
		// Resets the usage counter if it would overflow by sorting by ascending last usage and then resetting all counts to simple +1 increments
		qsort(m_cachedRenderedFonts, kNumCachedRenderedFonts, sizeof(CachedRenderedFont), FontManagerImpl::CRFSortPredicate);

		m_usageCounter = 0;

		for (unsigned int i = 0; i < kNumCachedRenderedFonts; i++)
		{
			CachedRenderedFont &crf = m_cachedRenderedFonts[i];

			if (!crf.m_rfont)
				break;

			crf.m_lastUsage = m_usageCounter++;
		}
	}

	int FontManagerImpl::CRFSortPredicate(const void *a, const void *b)
	{
		const CachedRenderedFont *crfA = static_cast<const CachedRenderedFont *>(a);
		const CachedRenderedFont *crfB = static_cast<const CachedRenderedFont *>(b);

		if (crfA->m_rfont == nullptr && crfB->m_rfont == nullptr)
			return 0;

		if (crfA->m_rfont == nullptr && crfB->m_rfont != nullptr)
			return 1;

		if (crfA->m_rfont != nullptr && crfB->m_rfont == nullptr)
			return -1;

		if (crfA->m_lastUsage < crfB->m_lastUsage)
			return -1;

		if (crfA->m_lastUsage > crfB->m_lastUsage)
			return 1;

		return 0;
	}

	FontManagerImpl::FontPreset FontManagerImpl::ms_fontPresets[FontPresets::kCount] =
	{
		{ FontFamilyIDs::kSystem, 12, FontFamilyFlag_None, true },
		{ FontFamilyIDs::kSystem, 12, FontFamilyFlag_Bold, true },

		{ FontFamilyIDs::kApplication, 8, FontFamilyFlag_None, true },
		{ FontFamilyIDs::kApplication, 9, FontFamilyFlag_None, true },
		{ FontFamilyIDs::kApplication, 9, FontFamilyFlag_Bold, true },
		{ FontFamilyIDs::kApplication, 10, FontFamilyFlag_Bold, true },
		{ FontFamilyIDs::kApplication, 12, FontFamilyFlag_Bold, true },
		{ FontFamilyIDs::kApplication, 14, FontFamilyFlag_None, true },
		{ FontFamilyIDs::kApplication, 14, FontFamilyFlag_Bold, true },
		{ FontFamilyIDs::kApplication, 18, FontFamilyFlag_None, true },
		{ FontFamilyIDs::kApplication, 40, FontFamilyFlag_None, true },

		{ FontFamilyIDs::kMonospace, 10, FontFamilyFlag_None, true },

		{ FontFamilyIDs::kHandwriting, 24, FontFamilyFlag_None, true },
		{ FontFamilyIDs::kHandwriting, 48, FontFamilyFlag_None, true },
	};

	FontManagerImpl FontManagerImpl::ms_instance;

	FontManager *FontManager::GetInstance()
	{
		return FontManagerImpl::GetInstance();
	}
}
