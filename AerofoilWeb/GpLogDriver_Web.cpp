#include "GpLogDriver_Web.h"
#include "GpFileSystem_Web.h"

#include "GpApplicationName.h"
#include "GpIOStream.h"

#include <time.h>
#include <cstring>

GpLogDriver_Web::GpLogDriver_Web()
{
}

void GpLogDriver_Web::Init()
{
	ms_instance.InitInternal();
}

void GpLogDriver_Web::VPrintf(Category category, const char *fmt, va_list args)
{
	size_t fmtSize = 0;
	bool hasFormatting = false;
	for (const char *fmtCheck = fmt; *fmtCheck; fmtCheck++)
	{
		if (*fmtCheck == '%')
			hasFormatting = true;

		fmtSize++;
	}

	time_t t = time(nullptr);
	struct tm sysTime = *localtime(&t);

	char timestampBuffer[64];
	sprintf(timestampBuffer, "[%02d:%02d:%02d] ", sysTime.tm_hour, sysTime.tm_min, sysTime.tm_sec);

	const char *debugTag = "";

	switch (category)
	{
	case Category_Warning:
		debugTag = "[WARNING] ";
		break;
	case Category_Error:
		debugTag = "[ERROR] ";
		break;
	default:
		break;
	};

	int formattedSize = vsnprintf(nullptr, 0, fmt, args);
	if (formattedSize <= 0)
		return;

	char *charBuff = static_cast<char*>(malloc(formattedSize + 1));
	if (!charBuff)
		return;

	vsnprintf(charBuff, formattedSize + 1, fmt, args);

	fprintf(stderr, "%s%s%s\n", timestampBuffer, debugTag, charBuff);
	fflush(stderr);

	free(charBuff);
}

void GpLogDriver_Web::Shutdown()
{
}

GpLogDriver_Web *GpLogDriver_Web::GetInstance()
{
	return &ms_instance;
}

void GpLogDriver_Web::InitInternal()
{
	time_t t = time(nullptr);
	struct tm utcTime = *gmtime(&t);

	this->Printf(IGpLogDriver::Category_Information, GP_APPLICATION_NAME " build " __TIMESTAMP__);
#if !GP_DEBUG_CONFIG
	this->Printf(IGpLogDriver::Category_Information, "Configuration: Release");
#else
	this->Printf(IGpLogDriver::Category_Information, "Configuration: Debug");
#endif
}

GpLogDriver_Web GpLogDriver_Web::ms_instance;
