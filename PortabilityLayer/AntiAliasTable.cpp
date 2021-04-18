#include "AntiAliasTable.h"
#include "RGBAColor.h"
#include "GpIOStream.h"
#include "IGpFileSystem.h"

#include "PLBigEndian.h"
#include "PLDrivers.h"
#include "QDStandardPalette.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>

namespace PortabilityLayer
{
	template<unsigned int TTreeSize, unsigned int TScaleSize>
	unsigned int BinTreeQuantize(const uint32_t(&tree)[TTreeSize], const uint32_t(&scale)[TScaleSize], uint32_t value)
	{
		unsigned int minIndexInclusive = 0;
		unsigned int maxIndexExclusive = TScaleSize;

		while (maxIndexExclusive - minIndexInclusive > 1)
		{
			unsigned int partitionIndex = (maxIndexExclusive + minIndexInclusive) / 2;
			uint32_t partitionValue = tree[partitionIndex - 1];

			if (value < partitionValue)
				maxIndexExclusive = partitionIndex;
			else if (value > partitionValue)
				minIndexInclusive = partitionIndex;
			else
				return partitionIndex;
		}

		return minIndexInclusive;
	}

	void AntiAliasTable::GenerateForPaletteFast(const RGBAColor &baseColorRef)
	{
		const RGBAColor baseColor = baseColorRef;
		const unsigned int kDivisions = 16;
		const unsigned int numColors = 256;

		unsigned int baseCh[3] = { baseColor.r, baseColor.g, baseColor.b };
		unsigned int baseChLinear[3];

		uint32_t rgbScaleLinear[6];
		for (unsigned int i = 0; i < 6; i++)
		{
			unsigned int upscaled = i * 51;
			rgbScaleLinear[i] = upscaled * upscaled * (kDivisions - 1);
		}

		uint32_t rgbScaleTree[5];
		for (unsigned int i = 0; i < 5; i++)
			rgbScaleTree[i] = (rgbScaleLinear[i] + rgbScaleLinear[i + 1]) / 2;

		uint32_t toneScaleLinear[16];
		uint32_t grayScaleLinear[16];
		for (unsigned int i = 0; i < 16; i++)
		{
			unsigned int upscaled = i * 17;
			toneScaleLinear[i] = upscaled * upscaled * (kDivisions - 1);
			grayScaleLinear[i] = toneScaleLinear[i] * 3;
		}

		uint32_t toneScaleTree[15];
		uint32_t grayScaleTree[15];
		for (unsigned int i = 0; i < 15; i++)
		{
			toneScaleTree[i] = (toneScaleLinear[i] + toneScaleLinear[i + 1]) / 2;
			grayScaleTree[i] = (grayScaleLinear[i] + grayScaleLinear[i + 1]) / 2;
		}

		for (int i = 0; i < 3; i++)
			baseChLinear[i] = baseCh[i] * baseCh[i];

		for (size_t i = 0; i < numColors; i++)
		{
			const RGBAColor existingColor = StandardPalette::GetInstance()->GetColors()[i];

			unsigned int existingCh[3] = { existingColor.r, existingColor.g, existingColor.b };
			unsigned int existingChLinear[3];

			for (int i = 0; i < 3; i++)
				existingChLinear[i] = existingCh[i] * existingCh[i];

			// 0 alpha is always the same color
			m_aaTranslate[i][0] = static_cast<uint8_t>(i);

			for (unsigned int b = 1; b < kDivisions; b++)
			{
				uint32_t newChLinear[3];

				for (unsigned int ch = 0; ch < 3; ch++)
					newChLinear[ch] = (15 - b) * existingChLinear[ch] + b * baseChLinear[ch];

				
				unsigned int toneIndexes[3];
				unsigned int rgbIndexes[3];
				unsigned int grayScaleIndex;

				for (int i = 0; i < 3; i++)
				{
					toneIndexes[i] = BinTreeQuantize(toneScaleTree, toneScaleLinear, newChLinear[i]);
					rgbIndexes[i] = BinTreeQuantize(rgbScaleTree, rgbScaleLinear, newChLinear[i]);
				}
				grayScaleIndex = BinTreeQuantize(grayScaleTree, grayScaleLinear, newChLinear[0] + newChLinear[1] + newChLinear[2]);

				uint64_t toneZeroError[3];
				uint64_t toneQuantizedError[3];
				uint64_t grayError = 0;
				uint64_t rgbError = 0;
				for (int i = 0; i < 3; i++)
				{
					toneZeroError[i] = static_cast<uint64_t>(newChLinear[i]);
					toneZeroError[i] *= toneZeroError[i];

					int32_t toneDelta = static_cast<int32_t>(toneScaleLinear[toneIndexes[i]]) - static_cast<int32_t>(newChLinear[i]);
					toneQuantizedError[i] = static_cast<uint64_t>(static_cast<int64_t>(toneDelta) * static_cast<int64_t>(toneDelta));

					int32_t rgbDelta = static_cast<int32_t>(rgbScaleLinear[rgbIndexes[i]]) - static_cast<int32_t>(newChLinear[i]);
					rgbError += static_cast<uint64_t>(static_cast<int64_t>(rgbDelta) * static_cast<int64_t>(rgbDelta));

					int32_t grayDelta = static_cast<int32_t>(toneScaleLinear[grayScaleIndex]) - static_cast<int32_t>(newChLinear[i]);
					grayError += static_cast<uint64_t>(static_cast<int64_t>(grayDelta) * static_cast<int64_t>(grayDelta));
				}

				uint64_t possibleErrors[5];
				possibleErrors[0] = toneQuantizedError[0] + toneZeroError[1] + toneZeroError[2];
				possibleErrors[1] = toneZeroError[0] + toneQuantizedError[1] + toneZeroError[2];
				possibleErrors[2] = toneZeroError[0] + toneZeroError[1] + toneQuantizedError[2];
				possibleErrors[3] = grayError;
				possibleErrors[4] = rgbError;

				int bestErrorIndex = 0;
				for (int i = 1; i < 5; i++)
				{
					if (possibleErrors[i] < possibleErrors[bestErrorIndex])
						bestErrorIndex = i;
				}

				uint8_t bestColor = 0;
				if (bestErrorIndex == 0)
					bestColor = StandardPalette::GetInstance()->MapColorAnalyticTruncated(toneIndexes[0], 0, 0);
				else if (bestErrorIndex == 1)
					bestColor = StandardPalette::GetInstance()->MapColorAnalyticTruncated(0, toneIndexes[1], 0);
				else if (bestErrorIndex == 2)
					bestColor = StandardPalette::GetInstance()->MapColorAnalyticTruncated(0, 0, toneIndexes[2]);
				else if (bestErrorIndex == 3)
					bestColor = StandardPalette::GetInstance()->MapColorAnalyticTruncated(grayScaleIndex, grayScaleIndex, grayScaleIndex);
				else //if (bestErrorIndex == 4)
					bestColor = StandardPalette::GetInstance()->MapColorAnalyticTruncated(rgbIndexes[0] * 3, rgbIndexes[1] * 3, rgbIndexes[2] * 3);

				m_aaTranslate[i][b] = bestColor;
			}
		}
	}

	void AntiAliasTable::GenerateForPalette(const RGBAColor &baseColorRef, const RGBAColor *colors, size_t numColors)
	{
		const RGBAColor baseColor = baseColorRef;

		if (numColors > 256)
			numColors = 256;

		unsigned int baseCh[3] = { baseColor.r, baseColor.g, baseColor.b };
		unsigned int baseChLinear[3];

		for (int i = 0; i < 3; i++)
			baseChLinear[i] = baseCh[i] * baseCh[i];

		for (size_t i = 0; i < numColors; i++)
		{
			const RGBAColor existingColor = colors[i];

			unsigned int existingCh[3] = { existingColor.r, existingColor.g, existingColor.b };
			unsigned int existingChLinear[3];

			for (int i = 0; i < 3; i++)
				existingChLinear[i] = existingCh[i] * existingCh[i];

			// 0 alpha is always the same color
			m_aaTranslate[i][0] = static_cast<uint8_t>(i);

			for (unsigned int b = 1; b < 16; b++)
			{
				uint32_t newChLinear[3];

				for (unsigned int ch = 0; ch < 3; ch++)
					newChLinear[ch] = (15 - b) * existingChLinear[ch] + b * baseChLinear[ch];

				uint64_t bestError = UINT64_MAX;
				size_t bestColor = 0;
				for (size_t cmp = 0; cmp < numColors; cmp++)
				{
					int32_t compareColor[3] = { colors[cmp].r, colors[cmp].g, colors[cmp].b };
					for (unsigned int ch = 0; ch < 3; ch++)
					{
						compareColor[ch] = compareColor[ch] * compareColor[ch] * 15;
					}

					uint64_t error = 0;
					for (unsigned int ch = 0; ch < 3; ch++)
					{
						int64_t delta = static_cast<int32_t>(newChLinear[ch]) - compareColor[ch];
						error += static_cast<uint64_t>(delta * delta);
					}

					if (error < bestError)
					{
						bestError = error;
						bestColor = cmp;
					}
				}

				m_aaTranslate[i][b] = static_cast<uint8_t>(bestColor);
			}
		}
	}

	void AntiAliasTable::GenerateForSimpleScale(uint8_t colorChannel)
	{
		const double gamma = 1.8;
		const double rcpGamma = 1.0 / gamma;
		const double rcp255 = 1.0 / 255.0;
		const double rcp15 = 1.0 / 15.0;
		double colorChannelLinear = pow(colorChannel * rcp255, gamma);

		for (size_t baseColor = 0; baseColor < 256; baseColor++)
		{
			const double baseColorLinear = pow(baseColor * rcp255, gamma);

			for (size_t opacity = 0; opacity < 16; opacity++)
			{
				const double opacityF = static_cast<double>(opacity) * rcp15;
				const double blendedColor = colorChannelLinear * opacityF + (1.0 - opacityF) * baseColorLinear;

				const double blendedColorGammaSpace = pow(std::min(std::max(0.0, blendedColor), 1.0), rcpGamma);

				m_aaTranslate[baseColor][opacity] = static_cast<uint8_t>(floor(blendedColorGammaSpace * 255.0 + 0.5));
			}
		}
	}
}
