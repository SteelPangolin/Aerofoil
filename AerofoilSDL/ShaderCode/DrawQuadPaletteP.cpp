#include "Functions.h"
#include "DrawQuadPixelConstants.h"

#define GP_GL_SHADER_CODE_DRAWQUADPALETTEP_GLSL	\
"\n"\
"varying vec4 texCoord;\n"\
"uniform sampler2D surfaceTexture;\n"\
"uniform sampler2D paletteTexture;\n"\
"\n"\
"vec3 SamplePixel(vec2 tc)\n"\
"{\n"\
"	float surfaceColor = texture2D(surfaceTexture, tc).r;\n"\
"	return texture2D(paletteTexture, vec2(surfaceColor * (255.0 / 256.0) + (0.5 / 256.0), 0.5)).rgb;\n"\
"}\n"\
"\n"\
"void main()\n"\
"{\n"\
"	vec4 resultColor = vec4(SamplePixel(texCoord.xy), 1.0);\n"\
"	resultColor *= constants_Modulation;\n"\
"#ifdef ENABLE_FLICKER\n"\
"	resultColor = ApplyFlicker(constants_FlickerAxis, texCoord.zw, constants_FlickerStartThreshold, constants_FlickerEndThreshold, resultColor);\n"\
"	if (resultColor.a < 1.0)\n"\
"		discard;\n"\
"#endif\n"\
"	resultColor = ApplyDesaturation(constants_Desaturation, resultColor);\n"\
"\n"\
"	gl_FragColor = vec4(ApplyColorSpaceTransform(resultColor.rgb), resultColor.a);\n"\
"}\n"

namespace GpBinarizedShaders
{
	const char *g_drawQuadPalettePF_GL2 = "#define ENABLE_FLICKER\n" GP_GL_SHADER_CODE_MEDIUM_PRECISION_PREFIX GP_GL_SHADER_CODE_DRAWQUADPIXELCONSTANTS_H GP_GL_SHADER_CODE_FUNCTIONS_H GP_GL_SHADER_CODE_DRAWQUADPALETTEP_GLSL;
	const char *g_drawQuadPalettePNF_GL2 = GP_GL_SHADER_CODE_MEDIUM_PRECISION_PREFIX GP_GL_SHADER_CODE_DRAWQUADPIXELCONSTANTS_H GP_GL_SHADER_CODE_FUNCTIONS_H GP_GL_SHADER_CODE_DRAWQUADPALETTEP_GLSL;
	const char *g_drawQuadPaletteICCPF_GL2 = "#define USE_ICC_PROFILE\n" "#define ENABLE_FLICKER\n" GP_GL_SHADER_CODE_MEDIUM_PRECISION_PREFIX GP_GL_SHADER_CODE_DRAWQUADPIXELCONSTANTS_H GP_GL_SHADER_CODE_FUNCTIONS_H GP_GL_SHADER_CODE_DRAWQUADPALETTEP_GLSL;
	const char *g_drawQuadPaletteICCPNF_GL2 = "#define USE_ICC_PROFILE\n" GP_GL_SHADER_CODE_MEDIUM_PRECISION_PREFIX GP_GL_SHADER_CODE_DRAWQUADPIXELCONSTANTS_H GP_GL_SHADER_CODE_FUNCTIONS_H GP_GL_SHADER_CODE_DRAWQUADPALETTEP_GLSL;
}
