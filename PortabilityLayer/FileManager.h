#pragma once
#ifndef __PL_FILE_MANAGER_H__
#define __PL_FILE_MANAGER_H__

#include "FilePermission.h"
#include "CoreDefs.h"
#include "FilePos.h"

#include <stdint.h>

class PLPasStr;

namespace PortabilityLayer
{
	class IOStream;
	struct MacFileProperties;

	class FileManager
	{
	public:
		virtual bool FileExists(uint32_t dirID, const PLPasStr &filename) = 0;

		virtual int OpenFileDF(uint32_t dirID, const PLPasStr &filename, EFilePermission filePermission, short *outRefNum) = 0;
		virtual int OpenFileRF(uint32_t dirID, const PLPasStr &filename, EFilePermission filePermission, short *outRefNum) = 0;
		virtual bool ReadFileProperties(uint32_t dirID, const PLPasStr &filename, MacFileProperties &properties) = 0;
		virtual IOStream *GetFileStream(int fileID) = 0;

		virtual int RawOpenFileDF(uint32_t dirID, const PLPasStr &filename, EFilePermission filePermission, bool ignoreMeta, IOStream **outStream) = 0;
		virtual int RawOpenFileRF(uint32_t dirID, const PLPasStr &filename, EFilePermission filePermission, bool ignoreMeta, IOStream **outStream) = 0;

		static FileManager *GetInstance();
	};
}

#endif