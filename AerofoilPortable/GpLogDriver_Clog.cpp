#include <iostream>

#include "GpLogDriver_Clog.h"

/// Log the message to `clog`.
/// Note: does not prepend timestamp.
void GpLogDriver_Clog::VPrintf(Category category, const char *fmt, va_list args)
{
    bool hasFormatting = false;
    for (const char *fmtCheck = fmt; *fmtCheck; fmtCheck++)
    {
        if (*fmtCheck == '%')
            hasFormatting = true;
    }

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
        std::clog << debugTag;
    }

    if (!hasFormatting)
    {
        std::clog << fmt;
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

        std::clog << charBuff;

        free(charBuff);
    }

    std::clog << std::endl;
}

/// Doesn't need to do anything.
void GpLogDriver_Clog::Shutdown() {}

GpLogDriver_Clog GpLogDriver_Clog::ms_instance;

GpLogDriver_Clog *GpLogDriver_Clog::GetInstance() {
    return &ms_instance;
}
