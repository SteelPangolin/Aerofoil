#include "GpFiber_SDL.h"
#include "HostSystemServices.h"
#include "HostThreadEvent.h"

GpFiber_SDL::GpFiber_SDL(SDL_Thread *thread, PortabilityLayer::HostThreadEvent *threadEvent)
	: m_event(threadEvent)
	, m_thread(thread)
{
}

GpFiber_SDL::~GpFiber_SDL()
{
	m_event->Destroy();
}

void GpFiber_SDL::YieldTo(IGpFiber *toFiber)
{
	static_cast<GpFiber_SDL*>(toFiber)->m_event->Signal();
	m_event->Wait();
}

void GpFiber_SDL::YieldToTerminal(IGpFiber *toFiber)
{
	static_cast<GpFiber_SDL*>(toFiber)->m_event->Signal();
}

void GpFiber_SDL::Destroy()
{
	delete this;
}