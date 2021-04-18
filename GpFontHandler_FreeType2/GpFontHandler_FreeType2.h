#pragma once

#include "IGpFontHandler.h"

#include <ft2build.h>
#include FT_SYSTEM_H
#include FT_FREETYPE_H

class GpIOStream;

namespace PortabilityLayer
{
	class HostFont;
}

class GpFontHandler_FreeType2 final : public IGpFontHandler
{
public:
	IGpFont *LoadFont(GpIOStream *stream, int typeFaceIndex) override;
	void Shutdown() override;

	bool KeepStreamOpen() const override;

	static GpFontHandler_FreeType2 *Create();

private:
	GpFontHandler_FreeType2();
	~GpFontHandler_FreeType2();

	static void *FTAllocThunk(FT_Memory memory, long size);
	static void FTFreeThunk(FT_Memory memory, void* block);
	static void *FTReallocThunk(FT_Memory memory, long curSize, long newSize, void *block);

	static unsigned long FTStreamIo(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count);
	static void FTStreamClose(FT_Stream stream);

	void *FTAlloc(long size);
	void FTFree(void* block);
	void *FTRealloc(long curSize, long newSize, void *block);

	bool Init();

	FT_MemoryRec_ m_mem;
	FT_Library m_library;
	unsigned int m_currentSize;
	bool m_ftIsInitialized;
};
