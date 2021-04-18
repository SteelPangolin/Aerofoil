#include "FontRenderer.h"

#include "CoreDefs.h"
#include "IGpFont.h"
#include "GpIOStream.h"
#include "IGpFontRenderedGlyph.h"
#include "MacRomanConversion.h"
#include "MemoryManager.h"
#include "RenderedFont.h"
#include "GpRenderedFontMetrics.h"
#include "GpRenderedGlyphMetrics.h"

#include "PLBigEndian.h"
#include "PLDrivers.h"
#include "PLPasStr.h"
#include "DeflateCodec.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <new>

namespace PortabilityLayer
{
	class RenderedFontImpl final : public RenderedFont
	{
	public:
		bool GetGlyph(unsigned int character, const GpRenderedGlyphMetrics *&outMetricsPtr, const void *&outData) const override;
		const GpRenderedFontMetrics &GetMetrics() const override;
		size_t MeasureString(const uint8_t *chars, size_t len) const override;
		bool IsAntiAliased() const override;

		void Destroy() override;

		void SetCharData(unsigned int charID, const void *data, size_t dataOffset, const GpRenderedGlyphMetrics &metrics);
		void SetFontMetrics(const GpRenderedFontMetrics &metrics);

		static RenderedFont *Load(GpIOStream *stream);
		bool Save(GpIOStream *stream) const;

		static RenderedFontImpl *Create(size_t glyphDataSize, bool aa);

	private:
		struct CacheHeader
		{
			BEUInt32_t m_cacheVersion;
			BEUInt32_t m_glyphDataSize;

			BEUInt32_t m_isAA;
		};

		struct SerializedGlyphMetrics
		{
			BEUInt32_t m_glyphDataPitch;

			BEUInt32_t m_glyphWidth;
			BEUInt32_t m_glyphHeight;

			BEInt16_t m_bearingX;
			BEInt16_t m_bearingY;
			BEInt16_t m_advanceX;
		};

		struct SerializedFontMetrics
		{
			BEInt32_t m_ascent;
			BEInt32_t m_descent;
			BEInt32_t m_linegap;
		};

		static const uint32_t kRFontCacheVersion = 3;

		RenderedFontImpl(void *data, size_t dataSize, bool aa);
		~RenderedFontImpl();

		bool LoadInternal(GpIOStream *stream, size_t dataSize);

		static SerializedGlyphMetrics SerializeGlyphMetrics(const GpRenderedGlyphMetrics &metrics);
		static GpRenderedGlyphMetrics DeserializeGlyphMetrics(const SerializedGlyphMetrics &metrics);

		static SerializedFontMetrics SerializeFontMetrics(const GpRenderedFontMetrics &metrics);
		static GpRenderedFontMetrics DeserializeFontMetrics(const SerializedFontMetrics &metrics);

		size_t m_dataOffsets[256];
		GpRenderedGlyphMetrics m_glyphMetrics[256];

		GpRenderedFontMetrics m_fontMetrics;
		bool m_isAntiAliased;

		void *m_data;
		size_t m_dataSize;
	};

	class FontRendererImpl final : public FontRenderer
	{
	public:
		RenderedFont *RenderFont(IGpFont *font, int size, bool aa, FontHacks fontHacks) override;
		RenderedFont *LoadCache(GpIOStream *stream) override;
		bool SaveCache(const RenderedFont *rfont, GpIOStream *stream) override;

		static FontRendererImpl *GetInstance();

	private:
		static void SynthesizeBoldAA(IGpFontRenderedGlyph *&glyph, unsigned int xScale, unsigned int yScale, bool aa);

		static FontRendererImpl ms_instance;
	};

	class ReRenderedGlyph final : public IGpFontRenderedGlyph
	{
	public:
		const GpRenderedGlyphMetrics &GetMetrics() const override;
		const void *GetData() const override;
		void Destroy() override;

		void *GetMutableData();
		GpRenderedGlyphMetrics &GetMutableMetrics();

		static ReRenderedGlyph *Create(unsigned int width, unsigned int height, bool aa);

	private:
		explicit ReRenderedGlyph(void *data, unsigned int width, unsigned int height, size_t pitch, size_t dataSize);

		GpRenderedGlyphMetrics m_metrics;
		void *m_data;
		size_t m_dataSize;
	};

	bool RenderedFontImpl::GetGlyph(unsigned int character, const GpRenderedGlyphMetrics *&outMetricsPtr, const void *&outData) const
	{
		const size_t dataOffset = m_dataOffsets[character];
		if (!dataOffset)
			return false;

		outMetricsPtr = m_glyphMetrics + character;
		outData = static_cast<const uint8_t*>(m_data) + dataOffset;

		return true;
	}

	const GpRenderedFontMetrics &RenderedFontImpl::GetMetrics() const
	{
		return m_fontMetrics;
	}

	size_t RenderedFontImpl::MeasureString(const uint8_t *chars, size_t len) const
	{
		size_t measure = 0;

		for (size_t i = 0; i < len; i++)
		{
			const GpRenderedGlyphMetrics &metrics = m_glyphMetrics[chars[i]];
			measure += metrics.m_advanceX;
		}

		return measure;
	}

	bool RenderedFontImpl::IsAntiAliased() const
	{
		return m_isAntiAliased;
	}

	void RenderedFontImpl::Destroy()
	{
		this->~RenderedFontImpl();
		free(this);
	}

	void RenderedFontImpl::SetCharData(unsigned int charID, const void *data, size_t dataOffset, const GpRenderedGlyphMetrics &metrics)
	{
		m_dataOffsets[charID] = dataOffset;
		m_glyphMetrics[charID] = metrics;
		memcpy(static_cast<uint8_t*>(m_data) + dataOffset, data, metrics.m_glyphDataPitch * metrics.m_glyphHeight);
	}

	void RenderedFontImpl::SetFontMetrics(const GpRenderedFontMetrics &metrics)
	{
		m_fontMetrics = metrics;
	}

	RenderedFont *RenderedFontImpl::Load(GpIOStream *stream)
	{
		CacheHeader header;
		if (stream->Read(&header, sizeof(header)) != sizeof(header))
			return nullptr;

		if (header.m_cacheVersion != kRFontCacheVersion)
			return nullptr;

		RenderedFontImpl *rfont = RenderedFontImpl::Create(header.m_glyphDataSize, header.m_isAA != 0);
		if (!rfont)
			return nullptr;

		if (!rfont->LoadInternal(stream, header.m_glyphDataSize))
		{
			rfont->Destroy();
			return nullptr;
		}

		return rfont;
	}

	bool RenderedFontImpl::Save(GpIOStream *stream) const
	{
		CacheHeader header;
		header.m_cacheVersion = kRFontCacheVersion;
		header.m_glyphDataSize = static_cast<uint32_t>(this->m_dataSize);
		header.m_isAA = m_isAntiAliased ? 1 : 0;

		GpUFilePos_t headerPos = stream->Tell();

		if (stream->Write(&header, sizeof(header)) != sizeof(header))
			return false;

		if (!stream->WriteExact(m_data, m_dataSize))
			return false;

		for (size_t i = 0; i < sizeof(m_dataOffsets) / sizeof(m_dataOffsets[0]); i++)
		{
			BEUInt32_t dataOffset = BEUInt32_t(static_cast<uint32_t>(m_dataOffsets[i]));
			if (!stream->WriteExact(&dataOffset, sizeof(dataOffset)))
				return false;
		}

		for (size_t i = 0; i < sizeof(m_glyphMetrics) / sizeof(m_glyphMetrics[0]); i++)
		{
			SerializedGlyphMetrics serialized = SerializeGlyphMetrics(m_glyphMetrics[i]);

			if (!stream->WriteExact(&serialized, sizeof(serialized)))
				return false;
		}

		SerializedFontMetrics fontMetrics = SerializeFontMetrics(m_fontMetrics);

		if (!stream->WriteExact(&fontMetrics, sizeof(fontMetrics)))
			return false;

		return true;
	}

	RenderedFontImpl *RenderedFontImpl::Create(size_t glyphDataSize, bool aa)
	{
		size_t alignedPrefixSize = sizeof(RenderedFontImpl) + GP_SYSTEM_MEMORY_ALIGNMENT - 1;
		alignedPrefixSize -= alignedPrefixSize % GP_SYSTEM_MEMORY_ALIGNMENT;

		if (SIZE_MAX - alignedPrefixSize < glyphDataSize)
			return nullptr;

		const size_t allocSize = alignedPrefixSize + glyphDataSize;

		void *storage = malloc(allocSize);
		if (!storage)
			return nullptr;

		memset(storage, 0, allocSize);

		return new (storage) RenderedFontImpl(static_cast<uint8_t*>(storage) + alignedPrefixSize, glyphDataSize, aa);
	}

	RenderedFontImpl::RenderedFontImpl(void *data, size_t dataSize, bool aa)
		: m_data(data)
		, m_dataSize(dataSize)
		, m_isAntiAliased(aa)
	{
		memset(m_glyphMetrics, 0, sizeof(m_glyphMetrics));
		memset(&m_fontMetrics, 0, sizeof(m_fontMetrics));
		memset(m_dataOffsets, 0, sizeof(m_dataOffsets));
	}

	RenderedFontImpl::~RenderedFontImpl()
	{
	}

	bool RenderedFontImpl::LoadInternal(GpIOStream *stream, size_t dataSize)
	{
		PortabilityLayer::MemoryManager *mm = PortabilityLayer::MemoryManager::GetInstance();

		if (!stream->ReadExact(m_data, m_dataSize))
			return false;

		for (size_t i = 0; i < sizeof(m_dataOffsets) / sizeof(m_dataOffsets[0]); i++)
		{
			BEUInt32_t dataOffset;
			if (!stream->ReadExact(&dataOffset, sizeof(dataOffset)))
				return false;

			m_dataOffsets[i] = dataOffset;
		}

		for (size_t i = 0; i < sizeof(m_glyphMetrics) / sizeof(m_glyphMetrics[0]); i++)
		{
			SerializedGlyphMetrics glyphMetrics;
			if (!stream->ReadExact(&glyphMetrics, sizeof(glyphMetrics)))
				return false;

			m_glyphMetrics[i] = DeserializeGlyphMetrics(glyphMetrics);
		}

		SerializedFontMetrics fontMetrics;
		if (!stream->ReadExact(&fontMetrics, sizeof(fontMetrics)))
			return false;

		m_fontMetrics = DeserializeFontMetrics(fontMetrics);

		return true;
	}

	RenderedFontImpl::SerializedGlyphMetrics RenderedFontImpl::SerializeGlyphMetrics(const GpRenderedGlyphMetrics &metrics)
	{
		SerializedGlyphMetrics result;
		result.m_advanceX = metrics.m_advanceX;
		result.m_bearingX = metrics.m_bearingX;
		result.m_bearingY = metrics.m_bearingY;
		result.m_glyphDataPitch = static_cast<uint32_t>(metrics.m_glyphDataPitch);
		result.m_glyphHeight = metrics.m_glyphHeight;
		result.m_glyphWidth = metrics.m_glyphWidth;

		return result;
	}

	GpRenderedGlyphMetrics RenderedFontImpl::DeserializeGlyphMetrics(const SerializedGlyphMetrics &metrics)
	{
		GpRenderedGlyphMetrics result;
		result.m_advanceX = metrics.m_advanceX;
		result.m_bearingX = metrics.m_bearingX;
		result.m_bearingY = metrics.m_bearingY;
		result.m_glyphDataPitch = static_cast<uint32_t>(metrics.m_glyphDataPitch);
		result.m_glyphHeight = metrics.m_glyphHeight;
		result.m_glyphWidth = metrics.m_glyphWidth;

		return result;
	}

	RenderedFontImpl::SerializedFontMetrics RenderedFontImpl::SerializeFontMetrics(const GpRenderedFontMetrics &metrics)
	{
		SerializedFontMetrics result;
		result.m_ascent = metrics.m_ascent;
		result.m_descent = metrics.m_descent;
		result.m_linegap = metrics.m_linegap;

		return result;
	}

	GpRenderedFontMetrics RenderedFontImpl::DeserializeFontMetrics(const SerializedFontMetrics &metrics)
	{
		GpRenderedFontMetrics result;
		result.m_ascent = metrics.m_ascent;
		result.m_descent = metrics.m_descent;
		result.m_linegap = metrics.m_linegap;

		return result;
	}


	RenderedFont *FontRendererImpl::RenderFont(IGpFont *font, int size, bool aa, FontHacks fontHacks)
	{
		const unsigned int numCharacters = 256;

		if (size < 1)
			return nullptr;

		int32_t lineSpacing;
		if (!font->GetLineSpacing(size, lineSpacing))
			return nullptr;

		IGpFontRenderedGlyph *glyphs[numCharacters];

		for (unsigned int i = 0; i < numCharacters; i++)
			glyphs[i] = nullptr;

		unsigned int xScale = 1;
		unsigned int yScale = 1;
		bool syntheticBoldAA = false;
		if (fontHacks == FontHacks_SyntheticBold)
		{
			if (aa)
			{
				syntheticBoldAA = true;
				xScale = 16;
				yScale = 16;
			}
		}

		for (unsigned int i = 0; i < numCharacters; i++)
		{
			uint16_t unicodeCodePoint = MacRoman::ToUnicode(i);
			if (unicodeCodePoint == 0xffff)
				continue;

			glyphs[i] = font->Render(unicodeCodePoint, size, xScale, yScale, aa);
		}

		if (fontHacks == FontHacks_SyntheticBold)
		{
			if (syntheticBoldAA)
			{
				for (unsigned int i = 0; i < numCharacters; i++)
				{
					if (glyphs[i])
						SynthesizeBoldAA(glyphs[i], xScale, yScale, syntheticBoldAA);
				}
			}
		}

		size_t glyphDataSize = GP_SYSTEM_MEMORY_ALIGNMENT;	// So we can use 0 to mean no data
		size_t numUsedGlyphs = 0;
		for (unsigned int i = 0; i < numCharacters; i++)
		{
			if (glyphs[i])
			{
				const GpRenderedGlyphMetrics &metrics = glyphs[i]->GetMetrics();
				glyphDataSize += metrics.m_glyphDataPitch * metrics.m_glyphHeight;
			}
		}

		RenderedFontImpl *rfont = RenderedFontImpl::Create(glyphDataSize, aa);
		if (rfont)
		{
			size_t fillOffset = GP_SYSTEM_MEMORY_ALIGNMENT;

			size_t numUsedGlyphs = 0;
			for (unsigned int i = 0; i < numCharacters; i++)
			{
				if (glyphs[i])
				{
					IGpFontRenderedGlyph *glyph = glyphs[i];
					
					GpRenderedGlyphMetrics metrics = glyph->GetMetrics();
					const void *data = glyph->GetData();

					if (fontHacks == FontHacks_Roboto && !aa)
					{
						if (size < 32)
						{
							// 'r' is shifted up 1 pixel
							if (i == 'r' && size < 32)
							{
								metrics.m_bearingY--;
							}

							// ':' doesn't have enough spacing
							if (i == ':')
							{
								metrics.m_bearingX++;
								metrics.m_advanceX++;
							}
						}
					}

					rfont->SetCharData(i, data, fillOffset, metrics);

					fillOffset += metrics.m_glyphDataPitch * metrics.m_glyphHeight;
				}
			}
		}

		// Compute metrics
		GpRenderedFontMetrics fontMetrics;
		fontMetrics.m_linegap = lineSpacing;
		fontMetrics.m_ascent = 0;
		fontMetrics.m_descent = 0;

		bool measuredAnyGlyphs = false;
		for (char capChar = 'A'; capChar <= 'Z'; capChar++)
		{
			const GpRenderedGlyphMetrics *glyphMetrics;
			const void *glyphData;
			if (rfont->GetGlyph(static_cast<unsigned int>(capChar), glyphMetrics, glyphData) && glyphMetrics != nullptr)
			{
				const int32_t ascent = glyphMetrics->m_bearingY;
				const int32_t descent = static_cast<int32_t>(glyphMetrics->m_glyphHeight) - ascent;

				if (!measuredAnyGlyphs)
				{
					fontMetrics.m_ascent = ascent;
					fontMetrics.m_descent = descent;
					measuredAnyGlyphs = true;
				}
				else
				{
					if (ascent > fontMetrics.m_ascent)
						fontMetrics.m_ascent = ascent;

					if (descent > fontMetrics.m_descent)
						fontMetrics.m_descent = descent;
				}
			}
		}

		rfont->SetFontMetrics(fontMetrics);

		for (unsigned int i = 0; i < numCharacters; i++)
		{
			if (glyphs[i])
				glyphs[i]->Destroy();
		}

		return rfont;
	}

	RenderedFont *FontRendererImpl::LoadCache(GpIOStream *stream)
	{
		return RenderedFontImpl::Load(stream);
	}

	bool FontRendererImpl::SaveCache(const RenderedFont *rfont, GpIOStream *stream)
	{
		if (!static_cast<const RenderedFontImpl*>(rfont)->Save(stream))
			return false;

		return true;
	}

	void FontRendererImpl::SynthesizeBoldAA(IGpFontRenderedGlyph *&glyph, unsigned int xScale, unsigned int yScale, bool aa)
	{
		GpRenderedGlyphMetrics metrics = glyph->GetMetrics();
		const void *existingData = glyph->GetData();

		uint32_t existingWidth = metrics.m_glyphWidth;
		uint32_t existingHeight = metrics.m_glyphHeight;

		uint32_t newWidth = (existingWidth + xScale - 1) / xScale + 1;
		uint32_t newHeight = (existingHeight + yScale - 1) / yScale;

		ReRenderedGlyph *newGlyph = ReRenderedGlyph::Create(newWidth, newHeight, aa);
		if (!newGlyph)
			return;

		uint16_t *fattenAccumulateBuffer = static_cast<uint16_t*>(MemoryManager::GetInstance()->Alloc(newWidth * sizeof(uint16_t)));
		if (!fattenAccumulateBuffer)
		{
			newGlyph->Destroy();
			return;
		}

		GpRenderedGlyphMetrics &newMetrics = newGlyph->GetMutableMetrics();
		newMetrics.m_advanceX = metrics.m_advanceX + 1;
		newMetrics.m_bearingX = metrics.m_bearingX;
		newMetrics.m_bearingY = metrics.m_bearingY;

		const size_t existingDataPitch = metrics.m_glyphDataPitch;
		const size_t newPitch = newMetrics.m_glyphDataPitch;
		uint8_t *newData = static_cast<uint8_t*>(newGlyph->GetMutableData());

		for (unsigned int outY = 0; outY < newHeight; outY++)
		{
			memset(fattenAccumulateBuffer, 0, newWidth * sizeof(uint16_t));

			for (unsigned int subY = 0; subY < yScale; subY++)
			{
				unsigned int originalY = subY + outY * yScale;
				if (originalY >= existingHeight)
					break;

				const uint8_t *existingRow = static_cast<const uint8_t*>(existingData) + originalY * existingDataPitch;

				unsigned int streakCounter = 0;
				unsigned int outX = 0;
				unsigned int outSubX = 0;
				for (unsigned int originalX = 0; originalX < newWidth * xScale; originalX++)
				{
					if (originalX < existingWidth)
					{
						//if (existingRow[originalX / 8] & (1 << (originalX & 7)))
						uint8_t v = ((existingRow[originalX / 2] >> ((originalX & 1) * 4)) & 0xf);
						if (v >= 8)
							streakCounter = xScale + 1;
					}

					if (streakCounter > 0)
					{
						streakCounter--;
						fattenAccumulateBuffer[outX]++;
					}

					outSubX++;
					if (outSubX == xScale)
					{
						outSubX = 0;
						outX++;
					}
				}
			}

			uint8_t *outRow = newData + outY * newPitch;
			unsigned int divisor = xScale * yScale;
			for (unsigned int x = 0; x < newWidth; x++)
			{
				uint32_t fraction = static_cast<uint32_t>(fattenAccumulateBuffer[x]) * 15;
				fraction = (fraction * 2 + divisor) / (divisor * 2);

				uint8_t *outByte = outRow + (x / 2);
				(*outByte) |= fraction << ((x & 1) * 4);

				size_t outOffset = outByte - newData;
			}
		}

		MemoryManager::GetInstance()->Release(fattenAccumulateBuffer);

		glyph->Destroy();
		glyph = newGlyph;
	}

	FontRendererImpl *FontRendererImpl::GetInstance()
	{
		return &ms_instance;
	}

	FontRendererImpl FontRendererImpl::ms_instance;

	const GpRenderedGlyphMetrics &ReRenderedGlyph::GetMetrics() const
	{
		return m_metrics;
	}

	const void *ReRenderedGlyph::GetData() const
	{
		return m_data;
	}

	void ReRenderedGlyph::Destroy()
	{
		this->~ReRenderedGlyph();
		PortabilityLayer::MemoryManager::GetInstance()->Release(this);
	}

	void *ReRenderedGlyph::GetMutableData()
	{
		return m_data;
	}

	GpRenderedGlyphMetrics &ReRenderedGlyph::GetMutableMetrics()
	{
		return m_metrics;
	}

	ReRenderedGlyph *ReRenderedGlyph::Create(unsigned int width, unsigned int height, bool aa)
	{
		size_t pitchRequired = 0;
		if (aa)
			pitchRequired = (width + 1) / 2;
		else
			pitchRequired = (width + 7) / 8;

		pitchRequired = pitchRequired + (GP_SYSTEM_MEMORY_ALIGNMENT - 1);
		pitchRequired -= pitchRequired % GP_SYSTEM_MEMORY_ALIGNMENT;

		size_t baseRequired = sizeof(ReRenderedGlyph);
		baseRequired = baseRequired + (GP_SYSTEM_MEMORY_ALIGNMENT - 1);
		baseRequired -= baseRequired % GP_SYSTEM_MEMORY_ALIGNMENT;

		size_t totalRequired = baseRequired + pitchRequired * height;
		void *storage = MemoryManager::GetInstance()->Alloc(totalRequired);
		if (!storage)
			return nullptr;

		uint8_t *data = static_cast<uint8_t*>(storage) + baseRequired;
		memset(data, 0, pitchRequired * height);

		return new (storage) ReRenderedGlyph(data, width, height, pitchRequired, totalRequired - baseRequired);
	}

	ReRenderedGlyph::ReRenderedGlyph(void *data, unsigned int width, unsigned int height, size_t pitch, size_t dataSize)
		: m_data(data)
		, m_dataSize(dataSize)
	{
		m_metrics.m_glyphWidth = width;
		m_metrics.m_glyphHeight = height;
		m_metrics.m_glyphDataPitch = pitch;
	}

	FontRenderer *FontRenderer::GetInstance()
	{
		return FontRendererImpl::GetInstance();
	}
}
