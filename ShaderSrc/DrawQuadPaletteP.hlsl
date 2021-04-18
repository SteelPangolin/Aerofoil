#include "DrawQuad.h"
#include "DrawQuadPixelConstants.h"
#include "Functions.h"

SamplerState nearestNeighborSampler : register(s0);
Texture2D<uint> surfaceTexture : register(t0);
Texture1D<float4> paletteTexture : register(t1);

struct SDrawQuadPixelOutput
{
	float4 color : SV_TARGET;
};

float3 SamplePixel(int2 texCoord)
{
	uint surfaceColor = surfaceTexture.Load(int3(texCoord, 0)).r;
	return paletteTexture.Load(int2(surfaceColor, 0)).rgb;
}

SDrawQuadPixelOutput PSMain(SDrawQuadPixelInput input)
{
	SDrawQuadPixelOutput result;
	int2 pixelCoordinate = int2(floor(input.texCoord.xy));
	result.color = float4(SamplePixel(int2(floor(input.texCoord.xy))), 1.0);
	result.color *= constants_Modulation;
	result.color = ApplyFlicker(pixelCoordinate, constants_FlickerStartThreshold, constants_FlickerEndThreshold, result.color);
	result.color = ApplyDesaturation(constants_Desaturation, result.color);
	
	if (result.color.a <= 0.0)
		discard;

	result.color.rgb = ApplyColorSpaceTransform(result.color.rgb);

	return result;
}
