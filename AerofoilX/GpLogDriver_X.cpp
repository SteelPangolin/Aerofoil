#include "GpLogDriver_X.h"
#include "GpFileSystem_X.h"

#include "GpApplicationName.h"
#include "GpIOStream.h"

#include <time.h>
#include <cstring>

GpLogDriver_X::GpLogDriver_X()
	: m_stream(nullptr)
	, m_isInitialized(false)
{
}

void GpLogDriver_X::Init()
{
	ms_instance.InitInternal();
}

void GpLogDriver_X::VPrintf(Category category, const char *fmt, va_list args)
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

	if (m_stream)
		m_stream->Write(timestampBuffer, strlen(timestampBuffer));

	const char *debugTag = "";

	switch (category)
	{
	case Category_Warning:
		debugTag = "[WARNING] ";
		break;
	case Category_Error:
		debugTag = "[ERROR] ";
		break;
	case Category_Information:
		debugTag = "[INFO] ";
		break;
	};

	if (debugTag[0])
	{
		if (m_stream)
			m_stream->Write(debugTag, strlen(debugTag));
	}

	if (!hasFormatting)
	{
		if (m_stream)
			m_stream->Write(fmt, fmtSize);
	}
	else
	{
		va_list lengthArgs;
		va_copy(lengthArgs, args);
		int formattedSize = vsnprintf(nullptr, 0, fmt, lengthArgs);
		va_end(lengthArgs);
		if (formattedSize <= 0)
			return;

		char *charBuff = static_cast<char*>(malloc(formattedSize + 1));
		if (!charBuff)
			return;

		vsnprintf(charBuff, formattedSize + 1, fmt, args);

		if (m_stream)
			m_stream->Write(charBuff, formattedSize);

		free(charBuff);
	}

	if (m_stream)
	{
		m_stream->Write("\n", 1);
		m_stream->Flush();
	}
}

void GpLogDriver_X::Shutdown()
{
	if (m_stream)
		m_stream->Close();
}

GpLogDriver_X *GpLogDriver_X::GetInstance()
{
	if (ms_instance.m_isInitialized)
		return &ms_instance;
	else
		return nullptr;
}

void GpLogDriver_X::InitInternal()
{
	time_t t = time(nullptr);
	struct tm utcTime = *gmtime(&t);

	char logFileName[256];

	sprintf(logFileName, GP_APPLICATION_NAME "-%04d-%02d-%02d_%02d-%02d_%02d.txt", utcTime.tm_year, utcTime.tm_mon, utcTime.tm_mday, utcTime.tm_hour, utcTime.tm_min, utcTime.tm_sec);

	m_stream = GpFileSystem_X::GetInstance()->OpenFile(PortabilityLayer::VirtualDirectories::kLogs, logFileName, true, GpFileCreationDispositions::kCreateOrOverwrite);
	if (m_stream)
	{
		this->Printf(IGpLogDriver::Category_Information, GP_APPLICATION_NAME " build " __TIMESTAMP__);
#if !GP_DEBUG_CONFIG
		this->Printf(IGpLogDriver::Category_Information, "Configuration: Release");
#else
		this->Printf(IGpLogDriver::Category_Information, "Configuration: Debug");
#endif

		m_isInitialized = true;
	}
}

GpLogDriver_X GpLogDriver_X::ms_instance;
