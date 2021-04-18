#pragma once

#include "IGpLogDriver.h"

#include <stdio.h>

class GpIOStream;

class GpLogDriver_Web : public IGpLogDriver
{
public:
	GpLogDriver_Web();

	static void Init();

	void VPrintf(Category category, const char *fmt, va_list args) override;
	void Shutdown() override;

	static GpLogDriver_Web *GetInstance();

private:
	void InitInternal();

	static GpLogDriver_Web ms_instance;
};
