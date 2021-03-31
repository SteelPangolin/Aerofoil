#include "GpSystemServices_X.h"

#include "IGpClipboardContents.h"
#include "IGpThreadEvent.h"
#include "SDL.h"

#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string>

struct GpSystemServices_X_ThreadStartParams
{
	GpSystemServices_X::ThreadFunc_t m_threadFunc;
	void *m_threadContext;
	IGpThreadEvent *m_threadStartEvent;
};

static void *StaticStartThread(void *lpThreadParameter)
{
	const GpSystemServices_X_ThreadStartParams *threadParams = static_cast<const GpSystemServices_X_ThreadStartParams*>(lpThreadParameter);

	GpSystemServices_X::ThreadFunc_t threadFunc = threadParams->m_threadFunc;
	void *threadContext = threadParams->m_threadContext;
	IGpThreadEvent *threadStartEvent = threadParams->m_threadStartEvent;

	threadStartEvent->Signal();

	return reinterpret_cast<void*>(static_cast<intptr_t>(threadFunc(threadContext)));
}

GpSystemServices_X::GpSystemServices_X()
	: m_textInputEnabled(false)
	, m_clipboardContents(nullptr)
{
}

GpSystemServices_X::~GpSystemServices_X()
{
	if (m_clipboardContents)
		m_clipboardContents->Destroy();
}

void *GpSystemServices_X::CreateThread(ThreadFunc_t threadFunc, void *context)
{
	IGpThreadEvent *evt = CreateThreadEvent(true, false);
	if (!evt)
		return nullptr;

	GpSystemServices_X_ThreadStartParams startParams;
	startParams.m_threadContext = context;
	startParams.m_threadFunc = threadFunc;
	startParams.m_threadStartEvent = evt;

	pthread_t thread = nullptr;
	if (pthread_create(&thread, nullptr, StaticStartThread, &startParams) != 0)
	{
		evt->Destroy();
		return nullptr;
	}

	evt->Wait();
	evt->Destroy();

	return thread;
}

void GpSystemServices_X::Beep() const
{
}

bool GpSystemServices_X::IsTouchscreen() const
{
	return false;
}

bool GpSystemServices_X::IsUsingMouseAsTouch() const
{
	return false;
}

bool GpSystemServices_X::IsTextInputObstructive() const
{
	return false;
}

bool GpSystemServices_X::IsFullscreenPreferred() const
{
	return true;
}

bool GpSystemServices_X::IsFullscreenOnStartup() const
{
	return false;
}

unsigned int GpSystemServices_X::GetCPUCount() const
{
	return SDL_GetCPUCount();
}

void GpSystemServices_X::SetTextInputEnabled(bool isEnabled)
{
	m_textInputEnabled = isEnabled;
}

bool GpSystemServices_X::IsTextInputEnabled() const
{
	return m_textInputEnabled;
}

bool GpSystemServices_X::AreFontResourcesSeekable() const
{
	return true;
}

IGpClipboardContents *GpSystemServices_X::GetClipboardContents() const
{
	return m_clipboardContents;
}

void GpSystemServices_X::SetClipboardContents(IGpClipboardContents *contents)
{
	if (contents != m_clipboardContents)
	{
		if (m_clipboardContents)
			m_clipboardContents->Destroy();

		m_clipboardContents = contents;
	}
}

GpSystemServices_X *GpSystemServices_X::GetInstance()
{
	return &ms_instance;
}

GpSystemServices_X GpSystemServices_X::ms_instance;
