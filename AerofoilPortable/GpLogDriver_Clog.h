#pragma once

#include "IGpLogDriver.h"

/// Log driver that wraps `clog`, the C++ equivalent of `stderr`.
/// Doesn't have `Init()` like other `IGpLogDriver` implementations because it has no state of its own.
class GpLogDriver_Clog : public IGpLogDriver
{
public:
    void VPrintf(Category category, const char *fmt, va_list args) override;
    void Shutdown() override;
    
    static GpLogDriver_Clog *GetInstance();

private:
    static GpLogDriver_Clog ms_instance;
};
