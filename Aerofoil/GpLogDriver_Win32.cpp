#include "GpLogDriver_Win32.h"
#include "GpFileSystem_Win32.h"

#include "GpApplicationName.h"
#include "GpIOStream.h"

GpLogDriver_Win32::GpLogDriver_Win32()
	: m_stream(nullptr)
	, m_isInitialized(false)
{
}

void GpLogDriver_Win32::Init()
{
	ms_instance.InitInternal();
}

void GpLogDriver_Win32::VPrintf(Category category, const char *fmt, va_list args)
{
	size_t fmtSize = 0;
	bool hasFormatting = false;
	for (const char *fmtCheck = fmt; *fmtCheck; fmtCheck++)
	{
		if (*fmtCheck == '%')
			hasFormatting = true;

		fmtSize++;
	}

	SYSTEMTIME sysTime;
	GetSystemTime(&sysTime);

	char timestampBuffer[64];
	sprintf(timestampBuffer, "[%02d:%02d:%02d:%03d] ", sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);

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
		m_stream->Write(debugTag, strlen(debugTag));

	if (!hasFormatting)
		m_stream->Write(fmt, fmtSize);
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

		m_stream->Write(charBuff, formattedSize);
		free(charBuff);
	}

	m_stream->Write("\n", 1);

	m_stream->Flush();
}

void GpLogDriver_Win32::Shutdown()
{
	if (m_stream)
		m_stream->Close();
}

GpLogDriver_Win32 *GpLogDriver_Win32::GetInstance()
{
	if (ms_instance.m_isInitialized)
		return &ms_instance;
	else
		return nullptr;
}

void GpLogDriver_Win32::InitInternal()
{
	SYSTEMTIME utcTime;
	GetSystemTime(&utcTime);

	char logFileName[256];

	sprintf(logFileName, GP_APPLICATION_NAME "-%04d-%02d-%02d_%02d-%02d_%02d.txt", utcTime.wYear, utcTime.wMonth, utcTime.wDay, utcTime.wHour, utcTime.wMinute, utcTime.wSecond);

	m_stream = GpFileSystem_Win32::GetInstance()->OpenFile(PortabilityLayer::VirtualDirectories::kLogs, logFileName, true, GpFileCreationDispositions::kCreateOrOverwrite);
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

GpLogDriver_Win32 GpLogDriver_Win32::ms_instance;
