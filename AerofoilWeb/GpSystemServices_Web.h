#pragma once

#include "IGpSystemServices.h"
#include "GpCoreDefs.h"

struct IGpClipboardContents;

class GpSystemServices_Web final : public IGpSystemServices
{
public:
	GpSystemServices_Web();
	~GpSystemServices_Web();

	void *CreateThread(ThreadFunc_t threadFunc, void *context) override;
	void Beep() const override;
	bool IsTouchscreen() const override;
	bool IsUsingMouseAsTouch() const override;
	bool IsTextInputObstructive() const override;
	bool IsFullscreenPreferred() const override;
	bool IsFullscreenOnStartup() const override;
	unsigned int GetCPUCount() const override;
	void SetTextInputEnabled(bool isEnabled) override;
	bool IsTextInputEnabled() const override;
	bool AreFontResourcesSeekable() const override;
	IGpClipboardContents *GetClipboardContents() const override;
	void SetClipboardContents(IGpClipboardContents *contents) override;
	int64_t GetTime() const override;
	void GetLocalDateTime(unsigned int &year, unsigned int &month, unsigned int &day, unsigned int &hour, unsigned int &minute, unsigned int &second) const override;
	IGpMutex *CreateMutex() override;
	IGpMutex *CreateRecursiveMutex() override;
	IGpThreadEvent *CreateThreadEvent(bool autoReset, bool startSignaled) override;
	uint64_t GetFreeMemoryCosmetic() const override;

	static GpSystemServices_Web *GetInstance();

private:
	static GpSystemServices_Web ms_instance;

	IGpClipboardContents *m_clipboardContents;
	bool m_textInputEnabled;
};
