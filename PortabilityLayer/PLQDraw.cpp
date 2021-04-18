#include "PLQDraw.h"
#include "QDManager.h"
#include "BitmapImage.h"
#include "DisplayDeviceManager.h"
#include "EllipsePlotter.h"
#include "FontFamily.h"
#include "FontManager.h"
#include "LinePlotter.h"
#include "MMHandleBlock.h"
#include "MemoryManager.h"
#include "MemReaderStream.h"
#include "RenderedFont.h"
#include "GpRenderedFontMetrics.h"
#include "GpRenderedGlyphMetrics.h"
#include "Rect2i.h"
#include "ResourceManager.h"
#include "ResTypeID.h"
#include "RGBAColor.h"
#include "ScanlineMask.h"
#include "ScanlineMaskConverter.h"
#include "ScanlineMaskIterator.h"
#include "QDGraf.h"
#include "QDStandardPalette.h"
#include "ResolveCachingColor.h"
#include "TextPlacer.h"
#include "WindowManager.h"
#include "QDGraf.h"
#include "QDPixMap.h"
#include "Vec2i.h"

#include "PLPasStr.h"
#include "PLStandardColors.h"

#include <algorithm>
#include <assert.h>

const uint32_t kInvertPixelAlphaMask = PortabilityLayer::RGBAColor::Create(0, 0, 0, 255).AsUInt32();

static inline void InvertPixel8(uint8_t &pixel)
{
	pixel = 255 ^ pixel;
}

static inline void InvertPixel32(uint8_t *pixel)
{
	uint32_t &pixel32 = *reinterpret_cast<uint32_t*>(pixel);
	pixel32 = ~pixel32;
	pixel[3] = 255;
}

void EndUpdate(WindowPtr graf)
{
	graf->GetDrawSurface()->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
}

void SetRect(Rect *rect, short left, short top, short right, short bottom)
{
	rect->left = left;
	rect->top = top;
	rect->bottom = bottom;
	rect->right = right;
}

static void PlotLine(DrawSurface *surface, const PortabilityLayer::Vec2i &pointA, const PortabilityLayer::Vec2i &pointB, PortabilityLayer::ResolveCachingColor &foreColor)
{
	const Rect lineRect = Rect::Create(
		std::min(pointA.m_y, pointB.m_y),
		std::min(pointA.m_x, pointB.m_x),
		std::max(pointA.m_y, pointB.m_y) + 1,
		std::max(pointA.m_x, pointB.m_x) + 1);

	// If the points are a straight line, paint as a rect
	if (pointA.m_y == pointB.m_y || pointA.m_x == pointB.m_x)
	{
		surface->FillRect(lineRect, foreColor);
		return;
	}

	PortabilityLayer::QDPort *port = &surface->m_port;

	GpPixelFormat_t pixelFormat = surface->m_port.GetPixelFormat();

	Rect constrainedRect = port->GetRect();

	constrainedRect = constrainedRect.Intersect(lineRect);

	if (!constrainedRect.IsValid())
		return;

	PortabilityLayer::Vec2i upperPoint = pointA;
	PortabilityLayer::Vec2i lowerPoint = pointB;

	if (pointA.m_y > pointB.m_y)
		std::swap(upperPoint, lowerPoint);

	PortabilityLayer::PixMapImpl *pixMap = static_cast<PortabilityLayer::PixMapImpl*>(*port->GetPixMap());
	const size_t pitch = pixMap->GetPitch();
	uint8_t *pixData = static_cast<uint8_t*>(pixMap->GetPixelData());

	PortabilityLayer::LinePlotter plotter;
	plotter.Reset(upperPoint, lowerPoint);

	PortabilityLayer::Vec2i currentPoint = upperPoint;
	while (currentPoint.m_x < constrainedRect.left || currentPoint.m_y < constrainedRect.top || currentPoint.m_x >= constrainedRect.right)
	{
		PortabilityLayer::PlotDirection plotDir = plotter.PlotNext();
		if (plotDir == PortabilityLayer::PlotDirection_Exhausted)
			return;

		currentPoint = plotter.GetPoint();
	}

	assert(currentPoint.m_y < constrainedRect.bottom);

	const size_t plotRowStartOffset = static_cast<size_t>(currentPoint.m_y) * pitch;
	const size_t plotLimit = pixMap->GetPitch() * (pixMap->m_rect.bottom - pixMap->m_rect.top);

	switch (pixelFormat)
	{
	case GpPixelFormats::k8BitStandard:
		{
			size_t plotOffset = plotRowStartOffset + static_cast<size_t>(currentPoint.m_x);
			const size_t pixelSize = 1;
			const uint8_t color = foreColor.Resolve8(nullptr, 0);

			while (currentPoint.m_x >= constrainedRect.left && currentPoint.m_x < constrainedRect.right && currentPoint.m_y < constrainedRect.bottom)
			{
				assert(plotOffset < plotLimit);
				pixData[plotOffset] = color;

				PortabilityLayer::PlotDirection plotDir = plotter.PlotNext();
				if (plotDir == PortabilityLayer::PlotDirection_Exhausted)
					return;

				switch (plotDir)
				{
				default:
				case PortabilityLayer::PlotDirection_Exhausted:
					return;

				case PortabilityLayer::PlotDirection_NegX_NegY:
				case PortabilityLayer::PlotDirection_0X_NegY:
				case PortabilityLayer::PlotDirection_PosX_NegY:
					// These should never happen, the point order is swapped so that Y is always 0 or positive
					assert(false);
					return;

				case PortabilityLayer::PlotDirection_NegX_PosY:
					currentPoint.m_x--;
					currentPoint.m_y++;
					plotOffset = plotOffset + pitch - pixelSize;
					break;
				case PortabilityLayer::PlotDirection_0X_PosY:
					currentPoint.m_y++;
					plotOffset = plotOffset + pitch;
					break;
				case PortabilityLayer::PlotDirection_PosX_PosY:
					currentPoint.m_x++;
					currentPoint.m_y++;
					plotOffset = plotOffset + pitch + pixelSize;
					break;

				case PortabilityLayer::PlotDirection_NegX_0Y:
					currentPoint.m_x--;
					plotOffset = plotOffset - pixelSize;
					break;
				case PortabilityLayer::PlotDirection_PosX_0Y:
					currentPoint.m_x++;
					plotOffset = plotOffset + pixelSize;
					break;
				}
			}
		}
		break;
	case GpPixelFormats::kRGB32:
		{
			size_t plotOffset = plotRowStartOffset + static_cast<size_t>(currentPoint.m_x) * 4;
			const size_t pixelSize = 4;
			const uint32_t color = foreColor.GetRGBAColor().AsUInt32();

			while (currentPoint.m_x >= constrainedRect.left && currentPoint.m_x < constrainedRect.right && currentPoint.m_y < constrainedRect.bottom)
			{
				assert(plotOffset < plotLimit);
				reinterpret_cast<uint32_t&>(pixData[plotOffset]) = color;

				PortabilityLayer::PlotDirection plotDir = plotter.PlotNext();
				if (plotDir == PortabilityLayer::PlotDirection_Exhausted)
					return;

				switch (plotDir)
				{
				default:
				case PortabilityLayer::PlotDirection_Exhausted:
					return;

				case PortabilityLayer::PlotDirection_NegX_NegY:
				case PortabilityLayer::PlotDirection_0X_NegY:
				case PortabilityLayer::PlotDirection_PosX_NegY:
					// These should never happen, the point order is swapped so that Y is always 0 or positive
					assert(false);
					return;

				case PortabilityLayer::PlotDirection_NegX_PosY:
					currentPoint.m_x--;
					currentPoint.m_y++;
					plotOffset = plotOffset + pitch - pixelSize;
					break;
				case PortabilityLayer::PlotDirection_0X_PosY:
					currentPoint.m_y++;
					plotOffset = plotOffset + pitch;
					break;
				case PortabilityLayer::PlotDirection_PosX_PosY:
					currentPoint.m_x++;
					currentPoint.m_y++;
					plotOffset = plotOffset + pitch + pixelSize;
					break;

				case PortabilityLayer::PlotDirection_NegX_0Y:
					currentPoint.m_x--;
					plotOffset = plotOffset - pixelSize;
					break;
				case PortabilityLayer::PlotDirection_PosX_0Y:
					currentPoint.m_x++;
					plotOffset = plotOffset + pixelSize;
					break;
				}
			}
		}
		break;
	default:
		PL_NotYetImplemented();
		return;
	}

	surface->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
}

static void DrawGlyph(PixMap *pixMap, const Rect &rect, const Point &penPos, const PortabilityLayer::RenderedFont *rfont, unsigned int character, PortabilityLayer::ResolveCachingColor &cacheColor)
{
	assert(rect.IsValid());

	const GpRenderedGlyphMetrics *metrics;
	const void *data;
	if (!rfont->GetGlyph(character, metrics, data))
		return;

	const int32_t leftCoord = penPos.h + metrics->m_bearingX;
	const int32_t topCoord = penPos.v - metrics->m_bearingY;
	const int32_t rightCoord = leftCoord + metrics->m_glyphWidth;
	const int32_t bottomCoord = topCoord + metrics->m_glyphHeight;

	const int32_t clampedLeftCoord = std::max<int32_t>(leftCoord, rect.left);
	const int32_t clampedTopCoord = std::max<int32_t>(topCoord, rect.top);
	const int32_t clampedRightCoord = std::min<int32_t>(rightCoord, rect.right);
	const int32_t clampedBottomCoord = std::min<int32_t>(bottomCoord, rect.bottom);

	if (clampedLeftCoord >= clampedRightCoord || clampedTopCoord >= clampedBottomCoord)
		return;

	const uint32_t firstOutputRow = clampedTopCoord;
	const uint32_t firstOutputCol = clampedLeftCoord;

	const uint32_t firstInputRow = clampedTopCoord - topCoord;
	const uint32_t firstInputCol = clampedLeftCoord - leftCoord;

	const uint32_t numCols = clampedRightCoord - clampedLeftCoord;
	const uint32_t numRows = clampedBottomCoord - clampedTopCoord;

	const size_t inputPitch = metrics->m_glyphDataPitch;
	const size_t outputPitch = pixMap->m_pitch;
	const uint8_t *firstInputRowData = static_cast<const uint8_t*>(data) + firstInputRow * inputPitch;

	const bool isAA = rfont->IsAntiAliased();

	switch (pixMap->m_pixelFormat)
	{
	case GpPixelFormats::k8BitStandard:
		{
			uint8_t *firstOutputRowData = static_cast<uint8_t*>(pixMap->m_data) + firstOutputRow * outputPitch;

			const PortabilityLayer::AntiAliasTable *aaTable = nullptr;

			if (isAA)
				aaTable = &PortabilityLayer::StandardPalette::GetInstance()->GetCachedPaletteAATable(cacheColor.GetRGBAColor());

			const uint8_t color = cacheColor.Resolve8(nullptr, 0);
			for (uint32_t row = 0; row < numRows; row++)
			{
				const uint8_t *inputRowData = firstInputRowData + row * inputPitch;
				uint8_t *outputRowData = firstOutputRowData + row * outputPitch;

				// It should be possible to speed this up, if needed.  The input is guaranteed to be well-aligned and not mutable within this loop.
				if (isAA)
				{
					for (uint32_t col = 0; col < numCols; col++)
					{
						const size_t inputOffset = firstInputCol + col;

						const unsigned int grayLevel = (inputRowData[inputOffset / 2] >> ((inputOffset & 1) * 4)) & 0xf;
						uint8_t &targetPixel = outputRowData[firstOutputCol + col];

						targetPixel = aaTable->m_aaTranslate[targetPixel][grayLevel];
					}
				}
				else
				{
					for (uint32_t col = 0; col < numCols; col++)
					{
						const size_t inputOffset = firstInputCol + col;
						if (inputRowData[inputOffset / 8] & (1 << (inputOffset & 0x7)))
							outputRowData[firstOutputCol + col] = color;
					}
				}
			}
		}
		break;
	case GpPixelFormats::kRGB32:
		{
			uint8_t *firstOutputRowData = static_cast<uint8_t*>(pixMap->m_data) + firstOutputRow * outputPitch;

			const PortabilityLayer::RGBAColor color = cacheColor.GetRGBAColor();
			uint8_t colorRGB[3] = { color.r, color.g, color.b };

			const PortabilityLayer::AntiAliasTable *aaTables[3] = { nullptr, nullptr, nullptr };

			if (isAA)
			{
				PortabilityLayer::RGBAColor rgbaColor = cacheColor.GetRGBAColor();
				uint8_t rgbColor[3] = { rgbaColor.r, rgbaColor.g, rgbaColor.b };

				for (int ch = 0; ch < 3; ch++)
					aaTables[ch] = &PortabilityLayer::StandardPalette::GetInstance()->GetCachedToneAATable(rgbColor[ch]);
			}

			for (uint32_t row = 0; row < numRows; row++)
			{
				const uint8_t *inputRowData = firstInputRowData + row * inputPitch;
				uint8_t *outputRowData = firstOutputRowData + row * outputPitch;

				// It should be possible to speed this up, if needed.  The input is guaranteed to be well-aligned and not mutable within this loop.
				if (isAA)
				{
					for (uint32_t col = 0; col < numCols; col++)
					{
						const size_t inputOffset = firstInputCol + col;

						const unsigned int grayLevel = (inputRowData[inputOffset / 2] >> ((inputOffset & 1) * 4)) & 0xf;
						uint8_t *targetPixel = outputRowData + (firstOutputCol + col) * 4;

						if (grayLevel > 0)
						{
							for (int ch = 0; ch < 3; ch++)
								targetPixel[ch] = aaTables[ch]->m_aaTranslate[targetPixel[ch]][grayLevel];
						}
					}
				}
				else
				{
					for (uint32_t col = 0; col < numCols; col++)
					{
						uint8_t *targetPixel = outputRowData + (firstOutputCol + col) * 4;

						const size_t inputOffset = firstInputCol + col;
						if (inputRowData[inputOffset / 8] & (1 << (inputOffset & 0x7)))
						{
							targetPixel[0] = color.r;
							targetPixel[1] = color.g;
							targetPixel[2] = color.b;
						}
					}
				}
			}
		}
		break;
	default:
		PL_NotYetImplemented();
	}
}

static void DrawText(PortabilityLayer::TextPlacer &placer, PixMap *pixMap, const Rect &rect, const PortabilityLayer::RenderedFont *rfont, PortabilityLayer::ResolveCachingColor &cacheColor)
{
	PortabilityLayer::GlyphPlacementCharacteristics characteristics;
	while (placer.PlaceGlyph(characteristics))
	{
		if (characteristics.m_haveGlyph)
			DrawGlyph(pixMap, rect, Point::Create(characteristics.m_glyphStartPos.m_x, characteristics.m_glyphStartPos.m_y), rfont, characteristics.m_character, cacheColor);
	}
}

void DrawSurface::DrawString(const Point &point, const PLPasStr &str, PortabilityLayer::ResolveCachingColor &cacheColor, PortabilityLayer::RenderedFont *font)
{
	DrawStringConstrained(point, str,  Rect::CreateLargest(), cacheColor, font);
}

void DrawSurface::DrawStringConstrained(const Point &point, const PLPasStr &str, const Rect &constraintRect, PortabilityLayer::ResolveCachingColor &cacheColor, PortabilityLayer::RenderedFont *rfont)
{
	PortabilityLayer::QDPort *port = &m_port;

	PortabilityLayer::FontManager *fontManager = PortabilityLayer::FontManager::GetInstance();

	PixMap *pixMap = *port->GetPixMap();

	const Rect rect = pixMap->m_rect.Intersect(constraintRect);

	if (!rect.IsValid())
		return;

	PortabilityLayer::TextPlacer placer(PortabilityLayer::Vec2i(point.h, point.v), -1, rfont, str);

	DrawText(placer, pixMap, rect, rfont, cacheColor);

	m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
}

void DrawSurface::DrawStringWrap(const Point &point, const Rect &constrainRect, const PLPasStr &str, PortabilityLayer::ResolveCachingColor &cacheColor, PortabilityLayer::RenderedFont *rfont)
{
	PortabilityLayer::QDPort *port = &m_port;

	PortabilityLayer::FontManager *fontManager = PortabilityLayer::FontManager::GetInstance();

	Point penPos = point;
	const size_t len = str.Length();
	const uint8_t *chars = str.UChars();

	PixMap *pixMap = *port->GetPixMap();

	const Rect limitRect = pixMap->m_rect.Intersect(constrainRect);
	const Rect areaRect = constrainRect;

	if (!limitRect.IsValid() || !areaRect.IsValid())
		return;	// ???

	PortabilityLayer::TextPlacer placer(PortabilityLayer::Vec2i(point.h, point.v), areaRect.Width(), rfont, str);

	DrawText(placer, pixMap, limitRect, rfont, cacheColor);

	m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
}

struct ErrorDiffusionWorkPixel
{
	int16_t m_16[3];
	uint8_t m_8[3];
};

static ErrorDiffusionWorkPixel ApplyErrorDiffusion(int16_t *errorDiffusionCurrentRow, uint8_t r, uint8_t g, uint8_t b, size_t col, size_t numCols)
{
	ErrorDiffusionWorkPixel result;

	const uint8_t rgb[] = { r, g, b };

	for (size_t i = 0; i < 3; i++)
	{
		const int16_t targetColorMul16 = static_cast<int16_t>(rgb[i]) * 16 + errorDiffusionCurrentRow[col * 3 + i];
		const int16_t targetColorRounded = (targetColorMul16 + 8) >> 4;

		result.m_16[i] = targetColorRounded;
		result.m_8[i] = static_cast<uint8_t>(std::max<int16_t>(std::min<int16_t>(targetColorRounded, 255), 0));
	}

	return result;
}

static void RedistributeError(int16_t *errorDiffusionNextRow, int16_t *errorDiffusionCurrentRow, int16_t targetR, int16_t targetG, int16_t targetB, int16_t actualR, int16_t actualG, int16_t actualB, size_t col, size_t numCols)
{
	int16_t rDiff = targetR - actualR;
	int16_t gDiff = targetG - actualG;
	int16_t bDiff = targetB - actualB;

	errorDiffusionNextRow += col * 3;
	errorDiffusionCurrentRow += col * 3;

	if (col > 0)
	{
		errorDiffusionNextRow[-3] += rDiff * 3;
		errorDiffusionNextRow[-2] += gDiff * 3;
		errorDiffusionNextRow[-1] += bDiff * 3;
	}

	errorDiffusionNextRow[0] += rDiff * 5;
	errorDiffusionNextRow[1] += gDiff * 5;
	errorDiffusionNextRow[2] += bDiff * 5;

	if (col < numCols - 1)
	{
		errorDiffusionCurrentRow[3] += rDiff * 7;
		errorDiffusionCurrentRow[4] += gDiff * 7;
		errorDiffusionCurrentRow[5] += bDiff * 7;

		errorDiffusionNextRow[3] += rDiff * 1;
		errorDiffusionNextRow[4] += gDiff * 1;
		errorDiffusionNextRow[5] += bDiff * 1;
	}
}

void DrawSurface::DrawPicture(THandle<BitmapImage> pictHdl, const Rect &bounds, bool errorDiffusion)
{
	if (!pictHdl)
		return;

	if (!bounds.IsValid() || bounds.Width() == 0 || bounds.Height() == 0)
		return;

	if (pictHdl.MMBlock()->m_size < sizeof(BitmapImage))
		return;

	BitmapImage *bmpPtr = *pictHdl;
	if (!bmpPtr)
		return;

	const size_t bmpSize = bmpPtr->m_fileHeader.m_fileSize;
	const Rect picRect = bmpPtr->GetRect();

	if (picRect.Width() == 0 || picRect.Height() == 0)
		return;

	if (bounds.right - bounds.left != picRect.right - picRect.left || bounds.bottom - bounds.top != picRect.bottom - picRect.top)
	{
		PL_NotYetImplemented_TODO("Palette");

		DrawSurface *scaleSurface = nullptr;
		if (PortabilityLayer::QDManager::GetInstance()->NewGWorld(&scaleSurface, this->m_port.GetPixelFormat(), picRect, nullptr) != PLErrors::kNone)
			return;

		scaleSurface->DrawPicture(pictHdl, picRect);

		const uint16_t newWidth = bounds.Width();
		const uint16_t newHeight = bounds.Height();

		THandle<PortabilityLayer::PixMapImpl> scaled = static_cast<PortabilityLayer::PixMapImpl*>(*scaleSurface->m_port.GetPixMap())->ScaleTo(newWidth, newHeight);
		PortabilityLayer::QDManager::GetInstance()->DisposeGWorld(scaleSurface);

		if (scaled)
			CopyBits(*scaled, *this->m_port.GetPixMap(), &(*scaled)->m_rect, &bounds, srcCopy);

		PortabilityLayer::PixMapImpl::Destroy(scaled);

		m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);

		return;
	}

	PortabilityLayer::QDPort *port = &m_port;

	if (!port)
		return;

	PortabilityLayer::PixMapImpl *pixMap = static_cast<PortabilityLayer::PixMapImpl*>(*port->GetPixMap());

	long handleSize = pictHdl.MMBlock()->m_size;
	PortabilityLayer::MemReaderStream stream(bmpPtr, handleSize);

	// Adjust draw origin
	const PortabilityLayer::Vec2i drawOrigin = PortabilityLayer::Vec2i(bounds.left, bounds.top);
	const Rect targetPixMapRect = pixMap->m_rect;

	const int32_t truncatedTop = std::max<int32_t>(0, targetPixMapRect.top - bounds.top);
	const int32_t truncatedBottom = std::max<int32_t>(0, bounds.bottom - targetPixMapRect.bottom);
	const int32_t truncatedLeft = std::max<int32_t>(0, targetPixMapRect.left - bounds.left);
	const int32_t truncatedRight = std::max<int32_t>(0, bounds.right - targetPixMapRect.right);

	uint8_t paletteMapping[256];
	for (int i = 0; i < 256; i++)
		paletteMapping[i] = 0;

	// Parse bitmap header
	const uint8_t *bmpBytes = reinterpret_cast<const uint8_t*>(bmpPtr);

	PortabilityLayer::BitmapFileHeader fileHeader;
	PortabilityLayer::BitmapInfoHeader infoHeader;

	memcpy(&fileHeader, bmpBytes, sizeof(fileHeader));
	memcpy(&infoHeader, bmpBytes + sizeof(fileHeader), sizeof(infoHeader));

	const uint16_t bpp = infoHeader.m_bitsPerPixel;

	if (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 16 && bpp != 24)
		return;

	uint32_t numColors = infoHeader.m_numColors;
	if (numColors > 256)
		return;

	if (numColors == 0 && bpp <= 8)
		numColors = (1 << bpp);

	const uint8_t *ctabLoc = bmpBytes + sizeof(fileHeader) + infoHeader.m_thisStructureSize;

	const size_t ctabSize = numColors * sizeof(PortabilityLayer::BitmapColorTableEntry);
	const size_t availCTabBytes = bmpSize - sizeof(fileHeader) - infoHeader.m_thisStructureSize;

	if (ctabSize > availCTabBytes)
		return;

	if (bpp <= 8)
	{
		// Perform palette mapping
		if (pixMap->GetPixelFormat() == GpPixelFormats::kBW1)
		{
			const PortabilityLayer::BitmapColorTableEntry *ctab = reinterpret_cast<const PortabilityLayer::BitmapColorTableEntry*>(ctabLoc);
			for (size_t i = 0; i < numColors; i++)
			{
				const PortabilityLayer::BitmapColorTableEntry &ctabEntry = ctab[i];
				if (ctabEntry.m_r + ctabEntry.m_g + ctabEntry.m_b < 383)
					paletteMapping[i] = 255;
				else
					paletteMapping[i] = 0;
			}
		}
		else if (pixMap->GetPixelFormat() == GpPixelFormats::k8BitStandard)
		{
			const PortabilityLayer::BitmapColorTableEntry *ctab = reinterpret_cast<const PortabilityLayer::BitmapColorTableEntry*>(ctabLoc);
			for (size_t i = 0; i < numColors; i++)
			{
				const PortabilityLayer::BitmapColorTableEntry &ctabEntry = ctab[i];
				paletteMapping[i] = PortabilityLayer::StandardPalette::GetInstance()->MapColorLUT(PortabilityLayer::RGBAColor::Create(ctabEntry.m_r, ctabEntry.m_g, ctabEntry.m_b, 255));
				const PortabilityLayer::RGBAColor &resultColor = PortabilityLayer::StandardPalette::GetInstance()->GetColors()[paletteMapping[i]];
			}
		}
		else if (pixMap->GetPixelFormat() == GpPixelFormats::k8BitCustom)
		{
			PL_NotYetImplemented();
		}
	}

	PortabilityLayer::MemoryManager *memManager = PortabilityLayer::MemoryManager::GetInstance();

	const uint32_t imageDataOffset = fileHeader.m_imageDataStart;

	if (imageDataOffset > bmpSize)
		return;

	const size_t availImageDataSize = bmpSize - imageDataOffset;

	const size_t sourcePitch = (bpp * infoHeader.m_width + 31) / 32 * 4;
	const size_t inDataSize = sourcePitch * infoHeader.m_height;

	if (inDataSize > availImageDataSize)
		return;

	const uint8_t *imageDataStart = reinterpret_cast<const uint8_t*>(bmpPtr) + imageDataOffset;
	const uint8_t *sourceFirstImageRowStart = imageDataStart + (infoHeader.m_height - 1) * sourcePitch;

	// Determine rect
	const PortabilityLayer::Rect2i sourceRect = PortabilityLayer::Rect2i(truncatedTop, truncatedLeft, static_cast<int32_t>(infoHeader.m_height) - truncatedBottom, static_cast<int32_t>(infoHeader.m_width) - truncatedRight);

	if (sourceRect.m_topLeft.m_x >= sourceRect.m_bottomRight.m_x || sourceRect.m_topLeft.m_y >= sourceRect.m_bottomRight.m_y)
		return;	// Entire rect was culled away

	const uint32_t numCopyRows = static_cast<uint32_t>(sourceRect.m_bottomRight.m_y - sourceRect.m_topLeft.m_y);
	const uint32_t numCopyCols = static_cast<uint32_t>(sourceRect.m_bottomRight.m_x - sourceRect.m_topLeft.m_x);

	const uint8_t *firstSourceRow = sourceFirstImageRowStart - static_cast<uint32_t>(sourceRect.m_topLeft.m_y) * sourcePitch;
	int32_t firstSourceCol = sourceRect.m_topLeft.m_x;

	const size_t destPitch = pixMap->GetPitch();
	uint8_t *firstDestRow = static_cast<uint8_t*>(pixMap->GetPixelData()) + destPitch * static_cast<uint32_t>(drawOrigin.m_y - targetPixMapRect.top + truncatedTop);
	size_t firstDestCol = static_cast<uint32_t>(drawOrigin.m_x - targetPixMapRect.left + truncatedLeft);

	const PortabilityLayer::StandardPalette *stdPalette = PortabilityLayer::StandardPalette::GetInstance();

	GpPixelFormat_t destFormat = pixMap->GetPixelFormat();
	switch (destFormat)
	{
	case GpPixelFormats::kBW1:
	case GpPixelFormats::k8BitStandard:
		{
			const uint8_t *currentSourceRow = firstSourceRow;
			uint8_t *currentDestRow = firstDestRow;

			int16_t *errorDiffusionBuffer = nullptr;
			int16_t *errorDiffusionNextRow = nullptr;
			int16_t *errorDiffusionCurrentRow = nullptr;

			if ((bpp == 16 || bpp == 24) && errorDiffusion)
			{
				errorDiffusionBuffer = static_cast<int16_t*>(memManager->Alloc(sizeof(int16_t) * numCopyCols * 2 * 3));
				if (!errorDiffusionBuffer)
					return;

				errorDiffusionNextRow = errorDiffusionBuffer;
				errorDiffusionCurrentRow = errorDiffusionBuffer + numCopyCols * 3;

				memset(errorDiffusionNextRow, 0, sizeof(int16_t) * numCopyCols * 3);
			}

			for (uint32_t row = 0; row < numCopyRows; row++)
			{
				if (errorDiffusionBuffer)
				{
					std::swap(errorDiffusionCurrentRow, errorDiffusionNextRow);
					memset(errorDiffusionNextRow, 0, sizeof(int16_t) * numCopyCols * 3);
				}

				assert(currentSourceRow >= imageDataStart && currentSourceRow <= imageDataStart + inDataSize);

				if (bpp == 1)
				{
					for (size_t col = 0; col < numCopyCols; col++)
					{
						const size_t srcColIndex = col + firstSourceCol;
						const size_t destColIndex = col + firstDestCol;

						const unsigned int srcIndex = (currentSourceRow[srcColIndex / 8] >> (8 - ((srcColIndex & 7) + 1))) & 0x01;
						currentDestRow[destColIndex] = paletteMapping[srcIndex];
					}
				}
				else if (bpp == 4)
				{
					for (size_t col = 0; col < numCopyCols; col++)
					{
						const size_t srcColIndex = col + firstSourceCol;
						const size_t destColIndex = col + firstDestCol;

						const unsigned int srcIndex = (currentSourceRow[srcColIndex / 2] >> (8 - ((srcColIndex & 1) + 1) * 4)) & 0x0f;
						currentDestRow[destColIndex] = paletteMapping[srcIndex];
					}
				}
				else if (bpp == 8)
				{
					for (size_t col = 0; col < numCopyCols; col++)
					{
						const size_t srcColIndex = col + firstSourceCol;
						const size_t destColIndex = col + firstDestCol;

						const unsigned int srcIndex = currentSourceRow[srcColIndex];
						currentDestRow[destColIndex] = paletteMapping[srcIndex];
					}
				}
				else if (bpp == 16)
				{
					if (destFormat == GpPixelFormats::kBW1)
					{
						for (size_t col = 0; col < numCopyCols; col++)
						{
							const size_t srcColIndex = col + firstSourceCol;
							const size_t destColIndex = col + firstDestCol;

							const uint8_t srcLow = currentSourceRow[srcColIndex * 2 + 0];
							const uint8_t srcHigh = currentSourceRow[srcColIndex * 2 + 1];

							const unsigned int combinedValue = srcLow | (srcHigh << 8);
							const unsigned int b = (combinedValue & 0x1f);
							const unsigned int g = ((combinedValue >> 5) & 0x1f);
							const unsigned int r = ((combinedValue >> 10) & 0x1f);

							if (r + g + b > 46)
								currentDestRow[destColIndex] = 0;
							else
								currentDestRow[destColIndex] = 1;
						}
					}
					else
					{
						if (!errorDiffusion)
						{
							for (size_t col = 0; col < numCopyCols; col++)
							{
								const size_t srcColIndex = col + firstSourceCol;
								const size_t destColIndex = col + firstDestCol;

								const uint8_t srcLow = currentSourceRow[srcColIndex * 2 + 0];
								const uint8_t srcHigh = currentSourceRow[srcColIndex * 2 + 1];

								const unsigned int combinedValue = srcLow | (srcHigh << 8);
								const unsigned int b = (combinedValue & 0x1f);
								const unsigned int g = ((combinedValue >> 5) & 0x1f);
								const unsigned int r = ((combinedValue >> 10) & 0x1f);

								const unsigned int xr = (r << 5) | (r >> 2);
								const unsigned int xg = (g << 5) | (g >> 2);
								const unsigned int xb = (b << 5) | (b >> 2);

								uint8_t colorIndex = stdPalette->MapColorLUT(PortabilityLayer::RGBAColor::Create(xr, xg, xb, 255));

								currentDestRow[destColIndex] = colorIndex;
							}
						}
						else
						{
							for (size_t col = 0; col < numCopyCols; col++)
							{
								const size_t srcColIndex = col + firstSourceCol;
								const size_t destColIndex = col + firstDestCol;

								const uint8_t srcLow = currentSourceRow[srcColIndex * 2 + 0];
								const uint8_t srcHigh = currentSourceRow[srcColIndex * 2 + 1];

								const unsigned int combinedValue = srcLow | (srcHigh << 8);
								const unsigned int b = (combinedValue & 0x1f);
								const unsigned int g = ((combinedValue >> 5) & 0x1f);
								const unsigned int r = ((combinedValue >> 10) & 0x1f);

								const unsigned int xr = (r << 5) | (r >> 2);
								const unsigned int xg = (g << 5) | (g >> 2);
								const unsigned int xb = (b << 5) | (b >> 2);

								ErrorDiffusionWorkPixel wp = ApplyErrorDiffusion(errorDiffusionCurrentRow, xr, xg, xb, col, numCopyCols);

								uint8_t colorIndex = stdPalette->MapColorLUT(PortabilityLayer::RGBAColor::Create(wp.m_8[0], wp.m_8[1], wp.m_8[2], 255));
								PortabilityLayer::RGBAColor resultColor = stdPalette->GetColors()[colorIndex];

								RedistributeError(errorDiffusionNextRow, errorDiffusionCurrentRow, wp.m_16[0], wp.m_16[1], wp.m_16[2], resultColor.r, resultColor.g, resultColor.b, col, numCopyCols);

								currentDestRow[destColIndex] = colorIndex;
							}
						}
					}
				}
				else if (bpp == 24)
				{
					if (destFormat == GpPixelFormats::kBW1)
					{
						for (size_t col = 0; col < numCopyCols; col++)
						{
							const size_t srcColIndex = col + firstSourceCol;
							const size_t destColIndex = col + firstDestCol;

							const uint8_t srcLow = currentSourceRow[srcColIndex * 2 + 0];
							const uint8_t srcHigh = currentSourceRow[srcColIndex * 2 + 1];

							const unsigned int combinedValue = srcLow | (srcHigh << 8);
							const unsigned int b = (combinedValue & 0x1f);
							const unsigned int g = ((combinedValue >> 5) & 0x1f);
							const unsigned int r = ((combinedValue >> 10) & 0x1f);

							if (r + g + b > 46)
								currentDestRow[destColIndex] = 0;
							else
								currentDestRow[destColIndex] = 1;
						}
					}
					else
					{
						if (!errorDiffusion)
						{
							for (size_t col = 0; col < numCopyCols; col++)
							{
								const size_t srcColIndex = col + firstSourceCol;
								const size_t destColIndex = col + firstDestCol;

								const uint8_t r = currentSourceRow[srcColIndex * 3 + 2];
								const uint8_t g = currentSourceRow[srcColIndex * 3 + 1];
								const uint8_t b = currentSourceRow[srcColIndex * 3 + 0];

								uint8_t colorIndex = stdPalette->MapColorLUT(PortabilityLayer::RGBAColor::Create(r, g, b, 255));

								currentDestRow[destColIndex] = colorIndex;
							}
						}
						else
						{
							for (size_t col = 0; col < numCopyCols; col++)
							{
								const size_t srcColIndex = col + firstSourceCol;
								const size_t destColIndex = col + firstDestCol;

								ErrorDiffusionWorkPixel wp = ApplyErrorDiffusion(errorDiffusionCurrentRow, currentSourceRow[srcColIndex * 3 + 2], currentSourceRow[srcColIndex * 3 + 1], currentSourceRow[srcColIndex * 3 + 0], col, numCopyCols);

								uint8_t colorIndex = stdPalette->MapColorLUT(PortabilityLayer::RGBAColor::Create(wp.m_8[0], wp.m_8[1], wp.m_8[2], 255));
								PortabilityLayer::RGBAColor resultColor = stdPalette->GetColors()[colorIndex];

								RedistributeError(errorDiffusionNextRow, errorDiffusionCurrentRow, wp.m_16[0], wp.m_16[1], wp.m_16[2], resultColor.r, resultColor.g, resultColor.b, col, numCopyCols);

								currentDestRow[destColIndex] = colorIndex;
							}
						}
					}
				}

				currentSourceRow -= sourcePitch;
				currentDestRow += destPitch;
			}

			if (errorDiffusionBuffer)
				memManager->Release(errorDiffusionBuffer);
		}
		break;
	case GpPixelFormats::kRGB32:
		{
			const uint8_t *currentSourceRow = firstSourceRow;
			uint8_t *currentDestRowBytes = firstDestRow;

			uint32_t blackColor32 = StdColors::Black().AsUInt32();
			uint32_t whiteColor32 = StdColors::White().AsUInt32();

			uint32_t unpackedColors[256];

			const PortabilityLayer::BitmapColorTableEntry *ctab = reinterpret_cast<const PortabilityLayer::BitmapColorTableEntry*>(ctabLoc);
			for (size_t i = 0; i < numColors; i++)
				unpackedColors[i] = PortabilityLayer::RGBAColor::Create(ctab[i].m_r, ctab[i].m_g, ctab[i].m_b, 255).AsUInt32();

			for (size_t i = numColors; i < 256; i++)
				unpackedColors[i] = StdColors::Black().AsUInt32();

			for (uint32_t row = 0; row < numCopyRows; row++)
			{
				uint32_t *currentDestRow32 = reinterpret_cast<uint32_t*>(currentDestRowBytes);

				assert(currentSourceRow >= imageDataStart && currentSourceRow <= imageDataStart + inDataSize);

				if (bpp == 1)
				{
					for (size_t col = 0; col < numCopyCols; col++)
					{
						const size_t srcColIndex = col + firstSourceCol;
						const size_t destColIndex = col + firstDestCol;

						const unsigned int srcIndex = (currentSourceRow[srcColIndex / 8] >> (8 - ((srcColIndex & 7) + 1))) & 0x01;
						currentDestRow32[destColIndex] = srcIndex ? blackColor32 : whiteColor32;
					}
				}
				else if (bpp == 4)
				{
					for (size_t col = 0; col < numCopyCols; col++)
					{
						const size_t srcColIndex = col + firstSourceCol;
						const size_t destColIndex = col + firstDestCol;

						const unsigned int srcIndex = (currentSourceRow[srcColIndex / 2] >> (8 - ((srcColIndex & 1) + 1) * 4)) & 0x0f;
						currentDestRow32[destColIndex] = unpackedColors[srcIndex];
					}
				}
				else if (bpp == 8)
				{
					for (size_t col = 0; col < numCopyCols; col++)
					{
						const size_t srcColIndex = col + firstSourceCol;
						const size_t destColIndex = col + firstDestCol;

						const unsigned int srcIndex = currentSourceRow[srcColIndex];
						currentDestRow32[destColIndex] = unpackedColors[srcIndex];
					}
				}
				else if (bpp == 16)
				{
					for (size_t col = 0; col < numCopyCols; col++)
					{
						const size_t srcColIndex = col + firstSourceCol;
						const size_t destColIndex = col + firstDestCol;

						const uint8_t srcLow = currentSourceRow[srcColIndex * 2 + 0];
						const uint8_t srcHigh = currentSourceRow[srcColIndex * 2 + 1];

						const unsigned int combinedValue = srcLow | (srcHigh << 8);
						const unsigned int b = (combinedValue & 0x1f);
						const unsigned int g = ((combinedValue >> 5) & 0x1f);
						const unsigned int r = ((combinedValue >> 10) & 0x1f);

						const unsigned int xr = (r << 5) | (r >> 2);
						const unsigned int xg = (g << 5) | (g >> 2);
						const unsigned int xb = (b << 5) | (b >> 2);

						currentDestRowBytes[destColIndex * 4 + 0] = xr;
						currentDestRowBytes[destColIndex * 4 + 1] = xg;
						currentDestRowBytes[destColIndex * 4 + 2] = xb;
						currentDestRowBytes[destColIndex * 4 + 3] = 255;
					}
				}
				else if (bpp == 24)
				{
					for (size_t col = 0; col < numCopyCols; col++)
					{
						const size_t srcColIndex = col + firstSourceCol;
						const size_t destColIndex = col + firstDestCol;

						currentDestRowBytes[destColIndex * 4 + 0] = currentSourceRow[srcColIndex * 3 + 2];
						currentDestRowBytes[destColIndex * 4 + 1] = currentSourceRow[srcColIndex * 3 + 1];
						currentDestRowBytes[destColIndex * 4 + 2] = currentSourceRow[srcColIndex * 3 + 0];
						currentDestRowBytes[destColIndex * 4 + 3] = 255;
					}
				}

				currentSourceRow -= sourcePitch;
				currentDestRowBytes += destPitch;
			}
		}
		break;
	default:
		// TODO: Implement higher-resolution pixel blitters
		assert(false);
		return;
	};

	m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
}

void DrawSurface::FillRect(const Rect &rect, PortabilityLayer::ResolveCachingColor &cacheColor)
{
	if (!rect.IsValid())
		return;

	PortabilityLayer::QDPort *qdPort = &m_port;

	GpPixelFormat_t pixelFormat = qdPort->GetPixelFormat();

	Rect constrainedRect = rect;

	constrainedRect = constrainedRect.Intersect(qdPort->GetRect());

	if (!constrainedRect.IsValid())
		return;

	PortabilityLayer::PixMapImpl *pixMap = static_cast<PortabilityLayer::PixMapImpl*>(*qdPort->GetPixMap());
	const size_t pitch = pixMap->GetPitch();
	const size_t firstRowStartIndex = static_cast<size_t>(constrainedRect.top) * pitch;
	const size_t numLines = static_cast<size_t>(constrainedRect.bottom - constrainedRect.top);
	const size_t numCols = static_cast<size_t>(constrainedRect.right - constrainedRect.left);
	uint8_t *pixData = static_cast<uint8_t*>(pixMap->GetPixelData());

	switch (pixelFormat)
	{
	case GpPixelFormats::k8BitStandard:
		{
			const size_t firstIndex = firstRowStartIndex + static_cast<size_t>(constrainedRect.left);
			const uint8_t color = cacheColor.Resolve8(nullptr, 0);

			size_t scanlineIndex = 0;
			for (size_t ln = 0; ln < numLines; ln++)
			{
				const size_t firstLineIndex = firstIndex + ln * pitch;
				for (size_t col = 0; col < numCols; col++)
					pixData[firstLineIndex + col] = color;
			}
		}
		break;
	case GpPixelFormats::kRGB32:
		{
			const size_t firstIndex = firstRowStartIndex + static_cast<size_t>(constrainedRect.left) * 4;
			const uint32_t color = cacheColor.GetRGBAColor().AsUInt32();

			size_t scanlineIndex = 0;
			for (size_t ln = 0; ln < numLines; ln++)
			{
				const size_t firstLineIndex = firstIndex + ln * pitch;
				for (size_t col = 0; col < numCols; col++)
					reinterpret_cast<uint32_t&>(pixData[firstLineIndex + col * 4]) = color;
			}
		}
		break;
	default:
		PL_NotYetImplemented();
		return;
	}

	m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
}

void DrawSurface::FillRectWithMaskPattern8x8(const Rect &rect, const uint8_t *pattern, PortabilityLayer::ResolveCachingColor &cacheColor)
{
	if (!rect.IsValid())
		return;

	PortabilityLayer::QDPort *qdPort = &m_port;

	GpPixelFormat_t pixelFormat = qdPort->GetPixelFormat();

	Rect constrainedRect = rect;
	const Rect portRect = qdPort->GetRect();

	constrainedRect = constrainedRect.Intersect(qdPort->GetRect());

	if (!constrainedRect.IsValid())
		return;

	assert(portRect.left == 0 && portRect.top == 0);

	PortabilityLayer::PixMapImpl *pixMap = static_cast<PortabilityLayer::PixMapImpl*>(*qdPort->GetPixMap());
	const size_t pitch = pixMap->GetPitch();
	const size_t rowFirstIndex = static_cast<size_t>(constrainedRect.top) * pitch;
	const size_t numLines = static_cast<size_t>(constrainedRect.bottom - constrainedRect.top);
	const size_t numCols = static_cast<size_t>(constrainedRect.right - constrainedRect.left);
	uint8_t *pixData = static_cast<uint8_t*>(pixMap->GetPixelData());

	const int patternFirstRow = (constrainedRect.top & 7);
	const int patternFirstCol = (constrainedRect.left & 7);

	switch (pixelFormat)
	{
	case GpPixelFormats::k8BitStandard:
		{
			const size_t firstIndex = rowFirstIndex + static_cast<size_t>(constrainedRect.left);
			const uint8_t color = cacheColor.Resolve8(nullptr, 0);

			size_t scanlineIndex = 0;
			for (size_t ln = 0; ln < numLines; ln++)
			{
				const int patternRow = static_cast<int>((patternFirstRow + ln) & 7);
				const size_t firstLineIndex = firstIndex + ln * pitch;

				for (size_t col = 0; col < numCols; col++)
				{
					const int patternCol = static_cast<int>((patternFirstCol + col) & 7);
					if ((pattern[patternRow] >> patternCol) & 1)
						pixData[firstLineIndex + col] = color;
				}
			}
		}
		break;
	case GpPixelFormats::kRGB32:
		{
			const size_t firstIndex = rowFirstIndex + static_cast<size_t>(constrainedRect.left) * 4;
			const uint32_t color = cacheColor.GetRGBAColor().AsUInt32();

			size_t scanlineIndex = 0;
			for (size_t ln = 0; ln < numLines; ln++)
			{
				const int patternRow = static_cast<int>((patternFirstRow + ln) & 7);
				const size_t firstLineIndex = firstIndex + ln * pitch;

				for (size_t col = 0; col < numCols; col++)
				{
					const int patternCol = static_cast<int>((patternFirstCol + col) & 7);
					if ((pattern[patternRow] >> patternCol) & 1)
						reinterpret_cast<uint32_t&>(pixData[firstLineIndex + col * 4]) = color;
				}
			}
		}
		break;
	default:
		PL_NotYetImplemented();
		return;
	}

	m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
}

void DrawSurface::FillEllipse(const Rect &rect, PortabilityLayer::ResolveCachingColor &cacheColor)
{
	if (!rect.IsValid() || rect.Width() < 1 || rect.Height() < 1)
		return;

	if (rect.Width() <= 2 || rect.Height() <= 2)
	{
		FillRect(rect, cacheColor);
		return;
	}

	PortabilityLayer::ScanlineMask *mask = PortabilityLayer::ScanlineMaskConverter::CompileEllipse(PortabilityLayer::Rect2i(rect.top, rect.left, rect.bottom, rect.right));
	if (mask)
	{
		FillScanlineMask(mask, cacheColor);
		mask->Destroy();
	}
}

void DrawSurface::FillEllipseWithMaskPattern(const Rect &rect, const uint8_t *pattern, PortabilityLayer::ResolveCachingColor &cacheColor)
{
	if (!rect.IsValid() || rect.Width() < 1 || rect.Height() < 1)
		return;

	if (rect.Width() <= 2 || rect.Height() <= 2)
	{
		FillRectWithMaskPattern8x8(rect, pattern, cacheColor);
		return;
	}

	PortabilityLayer::ScanlineMask *mask = PortabilityLayer::ScanlineMaskConverter::CompileEllipse(PortabilityLayer::Rect2i(rect.top, rect.left, rect.bottom, rect.right));
	if (mask)
	{
		FillScanlineMaskWithMaskPattern(mask, pattern, cacheColor);
		mask->Destroy();
	}
}

void DrawSurface::FrameEllipse(const Rect &rect, PortabilityLayer::ResolveCachingColor &cacheColor)
{
	if (!rect.IsValid())
		return;

	if (rect.Width() <= 2 || rect.Height() <= 2)
	{
		FillRect(rect, cacheColor);
		return;
	}

	Rect constrainedRect = rect;

	PortabilityLayer::QDPort *qdPort = &m_port;

	const Rect portRect = qdPort->GetRect();

	constrainedRect = constrainedRect.Intersect(portRect);

	if (!constrainedRect.IsValid())
		return;

	GpPixelFormat_t pixelFormat = qdPort->GetPixelFormat();

	PortabilityLayer::PixMapImpl *pixMap = static_cast<PortabilityLayer::PixMapImpl*>(*qdPort->GetPixMap());
	const size_t pitch = pixMap->GetPitch();
	const size_t firstIndex = static_cast<size_t>(constrainedRect.top) * pitch + static_cast<size_t>(constrainedRect.left);
	const size_t numLines = static_cast<size_t>(constrainedRect.bottom - constrainedRect.top);
	const size_t numCols = static_cast<size_t>(constrainedRect.right - constrainedRect.left);
	uint8_t *pixData = static_cast<uint8_t*>(pixMap->GetPixelData());

	PortabilityLayer::EllipsePlotter plotter;
	plotter.Reset(PortabilityLayer::Rect2i(rect.top, rect.left, rect.bottom, rect.right));

	PortabilityLayer::Rect2i constraintRect32 = PortabilityLayer::Rect2i(constrainedRect.top, constrainedRect.left, constrainedRect.bottom, constrainedRect.right);

	switch (pixelFormat)
	{
	case GpPixelFormats::k8BitStandard:
		{
			const uint8_t color = cacheColor.Resolve8(nullptr, 0);

			for (;;)
			{
				const PortabilityLayer::Vec2i pt = plotter.GetPoint();

				if (constraintRect32.Contains(pt))
				{
					const size_t pixelIndex = static_cast<size_t>(pt.m_y - portRect.top) * pitch + static_cast<size_t>(pt.m_x - portRect.left);
					pixData[pixelIndex] = color;
				}

				if (plotter.PlotNext() == PortabilityLayer::PlotDirection_Exhausted)
					break;
			}
		}
		break;
	case GpPixelFormats::kRGB32:
		{
			const uint32_t color32 = cacheColor.GetRGBAColor().AsUInt32();

			for (;;)
			{
				const PortabilityLayer::Vec2i pt = plotter.GetPoint();

				if (constraintRect32.Contains(pt))
				{
					const size_t pixelIndex = static_cast<size_t>(pt.m_y - portRect.top) * pitch + static_cast<size_t>(pt.m_x - portRect.left) * 4;
					*reinterpret_cast<uint32_t*>(pixData + pixelIndex) = color32;
				}

				if (plotter.PlotNext() == PortabilityLayer::PlotDirection_Exhausted)
					break;
			}
		}
		break;
	default:
		PL_NotYetImplemented();
		return;
	}

	m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
}

static void FillScanlineSpan8(uint8_t *rowStart, size_t startCol, size_t endCol, uint8_t patternByte, uint8_t foreColor)
{
	if (patternByte == 0xff)
	{
		for (size_t col = startCol; col < endCol; col++)
			rowStart[col] = foreColor;
	}
	else
	{
		for (size_t col = startCol; col < endCol; col++)
		{
			if (patternByte & (0x80 >> (col & 7)))
				rowStart[col] = foreColor;
		}
	}
}

static void FillScanlineSpan32(uint8_t *rowStartBytes, size_t startCol, size_t endCol, uint8_t patternByte, uint32_t foreColor)
{
	uint32_t *rowStart = reinterpret_cast<uint32_t*>(rowStartBytes);

	if (patternByte == 0xff)
	{
		for (size_t col = startCol; col < endCol; col++)
			rowStart[col] = foreColor;
	}
	else
	{
		for (size_t col = startCol; col < endCol; col++)
		{
			if (patternByte & (0x80 >> (col & 7)))
				rowStart[col] = foreColor;
		}
	}
}

void DrawSurface::FillScanlineMask(const PortabilityLayer::ScanlineMask *scanlineMask, PortabilityLayer::ResolveCachingColor &cacheColor)
{
	FillScanlineMaskWithMaskPattern(scanlineMask, nullptr, cacheColor);
}

void DrawSurface::FillScanlineMaskWithMaskPattern(const PortabilityLayer::ScanlineMask *scanlineMask, const uint8_t *pattern, PortabilityLayer::ResolveCachingColor &cacheColor)
{
	if (!scanlineMask)
		return;

	PortabilityLayer::QDPort *port = &m_port;

	PixMap *pixMap = *port->GetPixMap();
	const Rect portRect = port->GetRect();
	const Rect maskRect = scanlineMask->GetRect();

	const Rect constrainedRect = portRect.Intersect(maskRect);
	if (!constrainedRect.IsValid())
		return;

	const size_t firstMaskRow = static_cast<size_t>(constrainedRect.top - maskRect.top);
	const size_t firstMaskCol = static_cast<size_t>(constrainedRect.left - maskRect.left);
	const size_t firstPortRow = static_cast<size_t>(constrainedRect.top - portRect.top);
	const size_t firstPortCol = static_cast<size_t>(constrainedRect.left - portRect.left);
	const size_t pitch = pixMap->m_pitch;
	const size_t maskSpanWidth = scanlineMask->GetRect().right - scanlineMask->GetRect().left;

	// Skip mask rows
	PortabilityLayer::ScanlineMaskIterator iter = scanlineMask->GetIterator();
	for (size_t i = 0; i < firstMaskRow; i++)
	{
		size_t spanRemaining = maskSpanWidth;
		while (spanRemaining > 0)
			spanRemaining -= iter.Next();
	}

	uint8_t foreColor8 = 0;
	uint32_t foreColor32 = 0;

	const GpPixelFormat_t pixelFormat = pixMap->m_pixelFormat;

	size_t pixelSize = 0;
	switch (pixelFormat)
	{
	case GpPixelFormats::k8BitStandard:
		foreColor8 = cacheColor.Resolve8(nullptr, 256);
		break;
	case GpPixelFormats::kRGB32:
		foreColor32 = cacheColor.GetRGBAColor().AsUInt32();
		break;
	default:
		PL_NotYetImplemented();
		return;
	}

	uint8_t pattern8x8[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	bool havePattern = false;
	if (pattern)
	{
		memcpy(pattern8x8, pattern, 8);
		for (int i = 0; i < 8; i++)
		{
			if (pattern8x8[i] != 0xff)
			{
				havePattern = true;
				break;
			}
		}
	}

	const size_t constrainedRectWidth = static_cast<size_t>(constrainedRect.right - constrainedRect.left);

	uint8_t *pixMapData = static_cast<uint8_t*>(pixMap->m_data);
	uint8_t *firstRowStart = pixMapData + (constrainedRect.top * pitch);
	const size_t numRows = static_cast<size_t>(constrainedRect.bottom - constrainedRect.top);
	for (size_t row = 0; row < numRows; row++)
	{
		uint8_t *thisRowStart = firstRowStart + row * pitch;
		uint8_t thisRowPatternRow = pattern8x8[row & 7];

		bool spanState = false;

		size_t currentSpan = iter.Next();
		{
			// Skip prefix cols.  If the span ends at the first col, this must not advance the iterator to it (currentSpan should be 0 instead)
			size_t prefixColsRemaining = firstMaskCol;

			while (prefixColsRemaining > 0)
			{
				if (prefixColsRemaining <= currentSpan)
				{
					currentSpan -= prefixColsRemaining;
					break;
				}
				else
				{
					prefixColsRemaining -= currentSpan;
					currentSpan = iter.Next();
					spanState = !spanState;
				}
			}
		}

		// Paint in-bound cols.  If the span ends at the end of the mask, this must not advance the iterator beyond it (currentSpan should be 0 instead)
		size_t paintColsRemaining = constrainedRectWidth;
		size_t spanStartCol = firstPortCol;

		while (paintColsRemaining > 0)
		{
			if (paintColsRemaining <= currentSpan)
			{
				currentSpan -= paintColsRemaining;
				break;
			}
			else
			{
				const size_t spanEndCol = spanStartCol + currentSpan;
				if (spanState)
				{
					if (pixelFormat == GpPixelFormats::k8BitStandard)
						FillScanlineSpan8(thisRowStart, spanStartCol, spanEndCol, thisRowPatternRow, foreColor8);
					else if (pixelFormat == GpPixelFormats::kRGB32)
						FillScanlineSpan32(thisRowStart, spanStartCol, spanEndCol, thisRowPatternRow, foreColor32);
					else
						PL_NotYetImplemented();
				}

				spanStartCol = spanEndCol;
				paintColsRemaining -= currentSpan;
				currentSpan = iter.Next();
				spanState = !spanState;
			}
		}

		// Flush any lingering span
		if (spanState)
		{
			const size_t spanEndCol = firstPortCol + constrainedRectWidth;
			if (pixelFormat == GpPixelFormats::k8BitStandard)
				FillScanlineSpan8(thisRowStart, spanStartCol, spanEndCol, thisRowPatternRow, foreColor8);
			else if (pixelFormat == GpPixelFormats::kRGB32)
				FillScanlineSpan32(thisRowStart, spanStartCol, spanEndCol, thisRowPatternRow, foreColor32);
			else
				PL_NotYetImplemented();
		}

		if (row != numRows - 1)
		{
			size_t terminalColsRemaining = maskSpanWidth - constrainedRectWidth - firstMaskCol;

			assert(currentSpan <= terminalColsRemaining);

			terminalColsRemaining -= currentSpan;

			while (terminalColsRemaining > 0)
			{
				currentSpan = iter.Next();

				assert(currentSpan <= terminalColsRemaining);
				terminalColsRemaining -= currentSpan;
			}
		}
	}

	m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
}


void DrawSurface::DrawLine(const Point &a, const Point &b, PortabilityLayer::ResolveCachingColor &cacheColor)
{
	PlotLine(this, PortabilityLayer::Vec2i(a.h, a.v), PortabilityLayer::Vec2i(b.h, b.v), cacheColor);
}

void DrawSurface::FrameRect(const Rect &rect, PortabilityLayer::ResolveCachingColor &cacheColor)
{
	if (!rect.IsValid())
		return;

	uint16_t width = rect.right - rect.left;
	uint16_t height = rect.bottom - rect.top;

	if (width <= 2 || height <= 2)
		FillRect(rect, cacheColor);
	else
	{
		// This is stupid, especially in the vertical case, but oh well
		Rect edgeRect;

		edgeRect = rect;
		edgeRect.right = edgeRect.left + 1;
		FillRect(edgeRect, cacheColor);

		edgeRect = rect;
		edgeRect.left = edgeRect.right - 1;
		FillRect(edgeRect, cacheColor);

		edgeRect = rect;
		edgeRect.bottom = edgeRect.top + 1;
		FillRect(edgeRect, cacheColor);

		edgeRect = rect;
		edgeRect.top = edgeRect.bottom - 1;
		FillRect(edgeRect, cacheColor);
	}

	m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
}

void DrawSurface::FrameRoundRect(const Rect &rect, int quadrantWidth, int quadrantHeight, PortabilityLayer::ResolveCachingColor &cacheColor)
{
	PL_NotYetImplemented_TODO("RoundRect");
	this->FrameRect(rect, cacheColor);
}

void DrawSurface::InvertFrameRect(const Rect &rect, const uint8_t *pattern)
{
	if (!rect.IsValid())
		return;

	if (rect.Width() <= 2 || rect.Height() <= 2)
	{
		InvertFillRect(rect, pattern);
		return;
	}
	else
	{
		InvertFillRect(Rect::Create(rect.top, rect.left, rect.top + 1, rect.right), pattern);
		InvertFillRect(Rect::Create(rect.top + 1, rect.left, rect.bottom - 1, rect.left + 1), pattern);
		InvertFillRect(Rect::Create(rect.bottom - 1, rect.left, rect.bottom, rect.right), pattern);
		InvertFillRect(Rect::Create(rect.top + 1, rect.right - 1, rect.bottom - 1, rect.right), pattern);
	}
}

void DrawSurface::InvertFillRect(const Rect &rect, const uint8_t *pattern)
{
	if (!rect.IsValid())
		return;

	PortabilityLayer::QDPort *qdPort = &m_port;

	GpPixelFormat_t pixelFormat = qdPort->GetPixelFormat();

	Rect constrainedRect = rect;
	const Rect portRect = qdPort->GetRect();

	constrainedRect = constrainedRect.Intersect(qdPort->GetRect());

	if (!constrainedRect.IsValid())
		return;

	assert(portRect.left == 0 && portRect.top == 0);

	PortabilityLayer::PixMapImpl *pixMap = static_cast<PortabilityLayer::PixMapImpl*>(*qdPort->GetPixMap());
	const size_t pitch = pixMap->GetPitch();
	const size_t rowFirstIndex = static_cast<size_t>(constrainedRect.top) * pitch;
	const size_t numLines = static_cast<size_t>(constrainedRect.bottom - constrainedRect.top);
	const size_t numCols = static_cast<size_t>(constrainedRect.right - constrainedRect.left);
	uint8_t *pixData = static_cast<uint8_t*>(pixMap->GetPixelData());

	const int patternFirstRow = (constrainedRect.top & 7);
	const int patternFirstCol = (constrainedRect.left & 7);

	switch (pixelFormat)
	{
	case GpPixelFormats::k8BitStandard:
		{
			const size_t firstIndex = rowFirstIndex + static_cast<size_t>(constrainedRect.left);
			size_t scanlineIndex = 0;
			for (size_t ln = 0; ln < numLines; ln++)
			{
				const int patternRow = static_cast<int>((patternFirstRow + ln) & 7);
				const size_t firstLineIndex = firstIndex + ln * pitch;

				for (size_t col = 0; col < numCols; col++)
				{
					const int patternCol = static_cast<int>((patternFirstCol + col) & 7);
					if ((pattern[patternRow] >> patternCol) & 1)
						InvertPixel8(pixData[firstLineIndex + col]);
				}
			}
		}
		break;
	case GpPixelFormats::kRGB32:
		{
			const size_t firstIndex = rowFirstIndex + static_cast<size_t>(constrainedRect.left) * 4;
			size_t scanlineIndex = 0;
			for (size_t ln = 0; ln < numLines; ln++)
			{
				const int patternRow = static_cast<int>((patternFirstRow + ln) & 7);
				const size_t firstLineIndex = firstIndex + ln * pitch;

				for (size_t col = 0; col < numCols; col++)
				{
					const int patternCol = static_cast<int>((patternFirstCol + col) & 7);
					if ((pattern[patternRow] >> patternCol) & 1)
						InvertPixel32(pixData + (firstLineIndex + col * 4));
				}
			}
		}
		break;
	default:
		PL_NotYetImplemented();
		return;
	}

	m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
}

void InsetRect(Rect *rect, int x, int y)
{
	rect->left += x;
	rect->right -= x;
	rect->top += y;
	rect->bottom -= y;
}

Pattern *GetQDGlobalsGray(Pattern *pattern)
{
	uint8_t *patternBytes = *pattern;
	for (int i = 0; i < 8; i += 2)
	{
		patternBytes[i] = 0xaa;
		patternBytes[i + 1] = 0x55;
	}

	return pattern;
}

Pattern *GetQDGlobalsBlack(Pattern *pattern)
{
	uint8_t *patternBytes = *pattern;
	for (int i = 0; i < 8; i++)
		patternBytes[i] = 255;

	return pattern;
}

void GetIndPattern(Pattern *pattern, int patListID, int index)
{
	if (index < 1)
		return;

	THandle<uint8_t> patternList = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('PAT#', patListID).StaticCast<uint8_t>();
	const uint8_t *patternRes = *patternList;

	int numPatterns = (patternRes[0] << 8) | patternRes[1];
	if (index > numPatterns)
		return;

	memcpy(pattern, patternRes + 2 + (index - 1) * 8, 8);
}

static void CopyBitsComplete(const BitMap *srcBitmap, const BitMap *maskBitmap8, const BitMap *maskBitmap32, BitMap *destBitmap, const Rect *srcRectBase, const Rect *maskRectBase, const Rect *destRectBase, const Rect *maskConstraintRect)
{
	assert(srcBitmap->m_pixelFormat == destBitmap->m_pixelFormat);

	const Rect &srcBounds = srcBitmap->m_rect;
	const Rect &destBounds = destBitmap->m_rect;
	const GpPixelFormat_t pixelFormat = srcBitmap->m_pixelFormat;
	const size_t srcPitch = srcBitmap->m_pitch;
	const size_t destPitch = destBitmap->m_pitch;
	size_t maskPitch = 0;

	const BitMap *maskBitmapSelected = nullptr;
	if (maskBitmap8)
	{
		assert(maskBitmap32 == nullptr);
		maskPitch = maskBitmap8->m_pitch;
		maskBitmapSelected = maskBitmap8;
	}
	if (maskBitmap32)
	{
		assert(maskBitmap8 == nullptr);
		maskPitch = maskBitmap32->m_pitch;
		maskBitmapSelected = maskBitmap32;
	}

	assert((maskBitmapSelected == nullptr) == (maskRectBase == nullptr));

	if (srcRectBase->right - srcRectBase->left != destRectBase->right - destRectBase->left ||
		srcRectBase->bottom - srcRectBase->top != destRectBase->bottom - destRectBase->top)
	{
		PL_NotYetImplemented_TODO("ScaledBlit");
		return;
	}

	if (maskBitmap8)
	{
		assert(maskRectBase);
		assert(maskRectBase->right - maskRectBase->left == srcRectBase->right - srcRectBase->left);
		assert(maskBitmap8->m_pixelFormat == GpPixelFormats::kBW1 || maskBitmap8->m_pixelFormat == GpPixelFormats::k8BitStandard);
	}

	if (maskBitmap32)
	{
		assert(maskRectBase);
		assert(maskRectBase->right - maskRectBase->left == srcRectBase->right - srcRectBase->left);
		assert(maskBitmap32->m_pixelFormat == GpPixelFormats::kRGB32);
	}

	Rect srcRect;
	Rect destRect;
	Rect maskRect;

	{
		const Rect constrainedSrcRect = srcRectBase->Intersect(srcBounds);
		Rect constrainedDestRect = destRectBase->Intersect(destBounds);

		if (maskConstraintRect)
			constrainedDestRect = constrainedDestRect.Intersect(*maskConstraintRect);

		const int32_t leftNudge = std::max(constrainedSrcRect.left - srcRectBase->left, constrainedDestRect.left - destRectBase->left);
		const int32_t topNudge = std::max(constrainedSrcRect.top - srcRectBase->top, constrainedDestRect.top - destRectBase->top);
		const int32_t bottomNudge = std::min(constrainedSrcRect.bottom - srcRectBase->bottom, constrainedDestRect.bottom - destRectBase->bottom);
		const int32_t rightNudge = std::min(constrainedSrcRect.right - srcRectBase->right, constrainedDestRect.right - destRectBase->right);

		const int32_t srcLeft = srcRectBase->left + leftNudge;
		const int32_t srcRight = srcRectBase->right + rightNudge;
		const int32_t srcTop = srcRectBase->top + topNudge;
		const int32_t srcBottom = srcRectBase->bottom + bottomNudge;

		maskRect = Rect::Create(0, 0, 0, 0);
		if (maskRectBase)
		{
			maskRect.left = maskRectBase->left + leftNudge;
			maskRect.right = maskRectBase->right + rightNudge;
			maskRect.top = maskRectBase->top + topNudge;
			maskRect.bottom = maskRectBase->bottom + bottomNudge;
		}

		if (srcTop >= srcBottom)
			return;

		if (srcLeft >= srcRight)
			return;

		srcRect.left = srcLeft;
		srcRect.right = srcRight;
		srcRect.top = srcTop;
		srcRect.bottom = srcBottom;

		destRect.left = destRectBase->left + leftNudge;
		destRect.right = destRectBase->right + rightNudge;
		destRect.top = destRectBase->top + topNudge;
		destRect.bottom = destRectBase->bottom + bottomNudge;

		if (maskRectBase)
		{
			maskRect.left = maskRectBase->left + leftNudge;
			maskRect.right = maskRectBase->right + rightNudge;
			maskRect.top = maskRectBase->top + topNudge;
			maskRect.bottom = maskRectBase->bottom + bottomNudge;
		}
	}

	assert(srcRect.top >= srcBounds.top);
	assert(srcRect.bottom <= srcBounds.bottom);
	assert(srcRect.left >= srcBounds.left);
	assert(srcRect.right <= srcBounds.right);

	assert(destRect.top >= destBounds.top);
	assert(destRect.bottom <= destBounds.bottom);
	assert(destRect.left >= destBounds.left);
	assert(destRect.right <= destBounds.right);

	Rect constrainedSrcRect = srcRect;
	constrainedSrcRect.left += destRect.left - destRect.left;
	constrainedSrcRect.right += destRect.right - destRect.right;
	constrainedSrcRect.top += destRect.top - destRect.top;
	constrainedSrcRect.bottom += destRect.bottom - destRect.bottom;

	Rect constrainedMaskRect = maskRect;
	if (maskRectBase != nullptr)
	{
		constrainedMaskRect.left += destRect.left - destRect.left;
		constrainedMaskRect.right += destRect.right - destRect.right;
		constrainedMaskRect.top += destRect.top - destRect.top;
		constrainedMaskRect.bottom += destRect.bottom - destRect.bottom;
	}

	const size_t srcFirstCol = constrainedSrcRect.left - srcBitmap->m_rect.left;
	const size_t srcFirstRow = constrainedSrcRect.top - srcBitmap->m_rect.top;

	const size_t destFirstCol = destRect.left - destBitmap->m_rect.left;
	const size_t destFirstRow = destRect.top - destBitmap->m_rect.top;

	const size_t maskFirstCol = maskBitmapSelected ? constrainedMaskRect.left - maskBitmapSelected->m_rect.left : 0;
	const size_t maskFirstRow = maskBitmapSelected ? constrainedMaskRect.top - maskBitmapSelected->m_rect.top : 0;

	{
		size_t pixelSizeBytes = 0;

		switch (pixelFormat)
		{
		case GpPixelFormats::k8BitCustom:
		case GpPixelFormats::k8BitStandard:
		case GpPixelFormats::kBW1:
			pixelSizeBytes = 1;
			break;
		case GpPixelFormats::kRGB555:
			pixelSizeBytes = 2;
			break;
		case GpPixelFormats::kRGB24:
			pixelSizeBytes = 3;
			break;
		case GpPixelFormats::kRGB32:
			pixelSizeBytes = 4;
			break;
		default:
			return;
		};

		const uint8_t *srcBytes = static_cast<const uint8_t*>(srcBitmap->m_data);
		uint8_t *destBytes = static_cast<uint8_t*>(destBitmap->m_data);
		const uint8_t *maskBytes = maskBitmapSelected ? static_cast<uint8_t*>(maskBitmapSelected->m_data) : nullptr;

		const size_t firstSrcByte = srcFirstRow * srcPitch + srcFirstCol * pixelSizeBytes;
		const size_t firstDestByte = destFirstRow * destPitch + destFirstCol * pixelSizeBytes;
		const size_t firstMaskRowByte = maskBitmapSelected ? maskFirstRow * maskPitch : 0;

		const size_t numCopiedRows = srcRect.bottom - srcRect.top;
		const size_t numCopiedCols = srcRect.right - srcRect.left;
		const size_t numCopiedBytesPerScanline = numCopiedCols * pixelSizeBytes;

		if (maskBitmap8)
		{
			for (size_t i = 0; i < numCopiedRows; i++)
			{
				uint8_t *destRow = destBytes + firstDestByte + i * destPitch;
				const uint8_t *srcRow = srcBytes + firstSrcByte + i * srcPitch;
				const uint8_t *rowMaskBytes = maskBytes + firstMaskRowByte + i * maskPitch;

				size_t span = 0;

				for (size_t col = 0; col < numCopiedCols; col++)
				{
					const size_t maskBitOffset = maskFirstCol + col;
					//const bool maskBit = ((maskBytes[maskBitOffset / 8] & (0x80 >> (maskBitOffset & 7))) != 0);
					const bool maskBit = (rowMaskBytes[maskBitOffset] != 0);
					if (maskBit)
						span += pixelSizeBytes;
					else
					{
						if (span != 0)
							memcpy(destRow + col * pixelSizeBytes - span, srcRow + col * pixelSizeBytes - span, span);

						span = 0;
					}
				}

				if (span != 0)
					memcpy(destRow + numCopiedCols * pixelSizeBytes - span, srcRow + numCopiedCols * pixelSizeBytes - span, span);
			}
		}
		else if (maskBitmap32)
		{
			for (size_t i = 0; i < numCopiedRows; i++)
			{
				uint8_t *destRow = destBytes + firstDestByte + i * destPitch;
				const uint8_t *srcRow = srcBytes + firstSrcByte + i * srcPitch;
				const uint8_t *rowMaskBytes = maskBytes + firstMaskRowByte + i * maskPitch;

				size_t span = 0;

				for (size_t col = 0; col < numCopiedCols; col++)
				{
					const size_t maskBitOffset = maskFirstCol + col;
					//const bool maskBit = ((maskBytes[maskBitOffset / 8] & (0x80 >> (maskBitOffset & 7))) != 0);
					const bool maskBit = (reinterpret_cast<const uint32_t*>(rowMaskBytes)[maskBitOffset] != 0xffffffffU);
					if (maskBit)
						span += pixelSizeBytes;
					else
					{
						if (span != 0)
							memcpy(destRow + col * pixelSizeBytes - span, srcRow + col * pixelSizeBytes - span, span);

						span = 0;
					}
				}

				if (span != 0)
					memcpy(destRow + numCopiedCols * pixelSizeBytes - span, srcRow + numCopiedCols * pixelSizeBytes - span, span);
			}
		}
		else
		{
			for (size_t i = 0; i < numCopiedRows; i++)
				memcpy(destBytes + firstDestByte + i * destPitch, srcBytes + firstSrcByte + i * srcPitch, numCopiedBytesPerScanline);
		}
	}
}

void CopyBits(const BitMap *srcBitmap, BitMap *destBitmap, const Rect *srcRectBase, const Rect *destRectBase, CopyBitsMode copyMode)
{
	CopyBitsConstrained(srcBitmap, destBitmap, srcRectBase, destRectBase, copyMode, nullptr);
}

void CopyBitsConstrained(const BitMap *srcBitmap, BitMap *destBitmap, const Rect *srcRectBase, const Rect *destRectBase, CopyBitsMode copyMode, const Rect *constrainRect)
{
	const BitMap *maskBitmap8 = nullptr;
	const BitMap *maskBitmap32 = nullptr;
	const Rect *maskRect = nullptr;
	if (copyMode == transparent && srcBitmap->m_pixelFormat == GpPixelFormats::k8BitStandard)
	{
		maskBitmap8 = srcBitmap;
		maskRect = srcRectBase;
	}
	if (copyMode == transparent && srcBitmap->m_pixelFormat == GpPixelFormats::kRGB32)
	{
		maskBitmap32 = srcBitmap;
		maskRect = srcRectBase;
	}

	CopyBitsComplete(srcBitmap, maskBitmap8, maskBitmap32, destBitmap, srcRectBase, maskRect, destRectBase, constrainRect);
}

void CopyMaskConstrained(const BitMap *srcBitmap, const BitMap *maskBitmap, BitMap *destBitmap, const Rect *srcRectBase, const Rect *maskRectBase, const Rect *destRectBase, const Rect *constrainRect)
{
	CopyBitsComplete(srcBitmap, maskBitmap, nullptr, destBitmap, srcRectBase, maskRectBase, destRectBase, constrainRect);
}

// This doesn't bounds-check the source (because it's only used in one place)
void ImageInvert(const PixMap *invertMask, PixMap *targetBitmap, const Rect &srcRect, const Rect &destRect)
{
	assert(srcRect.Width() == destRect.Width());
	assert(srcRect.Height() == destRect.Height());

	assert(invertMask->m_pixelFormat == GpPixelFormats::kBW1);

	const Rect invertBitmapRect = invertMask->m_rect;
	const Rect targetBitmapRect = targetBitmap->m_rect;

	const Rect constrainedDestRect = targetBitmapRect.Intersect(destRect);
	if (!constrainedDestRect.IsValid())
		return;

	const int32_t leftInset = constrainedDestRect.left - destRect.left;
	const int32_t topInset = constrainedDestRect.top - destRect.top;
	const int32_t rightInset = destRect.right - constrainedDestRect.right;
	const int32_t bottomInset = destRect.bottom - constrainedDestRect.bottom;

	const int32_t firstSrcRow = srcRect.top - invertBitmapRect.top + topInset;
	const int32_t firstSrcCol = srcRect.left - invertBitmapRect.left + leftInset;
	const int32_t firstDestRow = destRect.top - targetBitmapRect.top + topInset;
	const int32_t firstDestCol = destRect.left - targetBitmapRect.left + leftInset;

	const uint16_t numRows = constrainedDestRect.Height();
	const uint16_t numCols = constrainedDestRect.Width();

	const size_t invertPitch = invertMask->m_pitch;
	const uint8_t *invertPixelDataFirstRow = static_cast<const uint8_t*>(invertMask->m_data) + firstSrcRow * invertPitch;
	const size_t targetPitch = targetBitmap->m_pitch;
	uint8_t *targetPixelDataFirstRow = static_cast<uint8_t*>(targetBitmap->m_data) + firstDestRow * targetPitch;

	const GpPixelFormat_t targetPixelFormat = targetBitmap->m_pixelFormat;

	for (uint16_t r = 0; r < numRows; r++)
	{
		const uint8_t *invertRowStart = invertPixelDataFirstRow + r * invertPitch;
		uint8_t *targetRowStart = targetPixelDataFirstRow + r * targetPitch;

		switch (targetPixelFormat)
		{
		case GpPixelFormats::k8BitStandard:
			for (uint16_t c = 0; c < numCols; c++)
			{
				const int32_t srcCol = c + firstSrcCol;
				const int32_t destCol = c + firstDestCol;
				if (invertRowStart[srcCol] != 0)
					InvertPixel8(targetRowStart[destCol]);
			}
			break;
		case GpPixelFormats::kRGB32:
			for (uint16_t c = 0; c < numCols; c++)
			{
				const int32_t srcCol = c + firstSrcCol;
				const int32_t destCol = c + firstDestCol;
				if (invertRowStart[srcCol] != 0)
					InvertPixel32(targetRowStart + destCol * 4);
			}
			break;
		default:
			PL_NotYetImplemented();
			break;
		}
	}
}

bool PointInScanlineMask(Point point, PortabilityLayer::ScanlineMask *scanlineMask)
{
	PL_NotYetImplemented();
	return false;
}

void CopyMask(const BitMap *srcBitmap, const BitMap *maskBitmap, BitMap *destBitmap, const Rect *srcRectBase, const Rect *maskRectBase, const Rect *destRectBase)
{
	CopyBitsComplete(srcBitmap, maskBitmap, nullptr, destBitmap, srcRectBase, maskRectBase, destRectBase, nullptr);
}

PixMap *GetPortBitMapForCopyBits(DrawSurface *grafPtr)
{
	return *grafPtr->m_port.GetPixMap();
}

Boolean SectRect(const Rect *rectA, const Rect *rectB, Rect *outIntersection)
{
	*outIntersection = rectA->Intersect(*rectB);

	return outIntersection->IsValid() ? PL_TRUE : PL_FALSE;
}


void BitMap::Init(const Rect &rect, GpPixelFormat_t pixelFormat, size_t pitch, void *dataPtr)
{
	m_rect = rect;
	m_pixelFormat = pixelFormat;
	m_pitch = pitch;
	m_data = dataPtr;
}

PortabilityLayer::RenderedFont *GetFont(PortabilityLayer::FontPreset_t fontPreset)
{
	PortabilityLayer::FontManager *fontManager = PortabilityLayer::FontManager::GetInstance();

	int size = 0;
	int variationFlags = 0;
	bool aa = false;
	PortabilityLayer::FontFamilyID_t familyID = PortabilityLayer::FontFamilyIDs::kCount;

	fontManager->GetFontPreset(fontPreset, &familyID, &size, &variationFlags, &aa);
	if (familyID == PortabilityLayer::FontFamilyIDs::kCount)
		return nullptr;

	return fontManager->LoadCachedRenderedFont(familyID, size, aa, variationFlags);
}

#include "stb_image_write.h"

void DebugPixMap(PixMap **pixMapH, const char *outName)
{
	PixMap *pixMap = *pixMapH;
	char outPath[1024];
	strcpy(outPath, outName);
	strcat(outPath, ".png");

	stbi_write_png(outPath, pixMap->m_rect.right - pixMap->m_rect.left, pixMap->m_rect.bottom - pixMap->m_rect.top, 1, pixMap->m_data, pixMap->m_pitch);
}

