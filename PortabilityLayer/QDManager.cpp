#include "QDManager.h"

#include "MemoryManager.h"
#include "PLCore.h"
#include "PLQDOffscreen.h"
#include "QDGraf.h"
#include "QDState.h"

namespace PortabilityLayer
{
	class QDManagerImpl final : public QDManager
	{
	public:
		QDManagerImpl();

		void Init() override;
		void GetPort(QDPort **port, GDevice ***gdHandle) override;
		void SetPort(QDPort *gw, GDevice **gdHandle) override;
		int NewGWorld(CGraf **gw, int depth, const Rect &bounds, ColorTable **colorTable, GDevice **device, int flags) override;
		QDState *GetState() override;

		static QDManagerImpl *GetInstance();

	private:
		QDPort *m_port;
		GDHandle m_gdHandle;

		static QDManagerImpl ms_instance;
	};

	QDManagerImpl::QDManagerImpl()
		: m_port(nullptr)
		, m_gdHandle(nullptr)
	{
	}

	void QDManagerImpl::Init()
	{
	}

	void QDManagerImpl::GetPort(QDPort **port, GDevice ***gdHandle)
	{
		if (port)
			*port = m_port;
		if (gdHandle)
			*gdHandle = m_gdHandle;
	}

	void QDManagerImpl::SetPort(QDPort *gw, GDevice **gdHandle)
	{
		m_port = gw;
		m_gdHandle = gdHandle;
	}

	int QDManagerImpl::NewGWorld(CGraf **gw, int depth, const Rect &bounds, ColorTable **colorTable, GDevice **device, int flags)
	{
		PixelFormat pixelFormat;

		switch (depth)
		{
		case 8:
			pixelFormat = (colorTable == nullptr) ? PixelFormat_8BitStandard : PixelFormat_8BitCustom;
			break;
		case 16:
			pixelFormat = PixelFormat_RGB555;
			break;
		case 32:
			pixelFormat = PixelFormat_RGB32;
			break;
		default:
			return genericErr;
		}

		if (depth != 8)
			return genericErr;

		void *grafStorage = MemoryManager::GetInstance()->Alloc(sizeof(CGraf));
		if (!grafStorage)
			return mFulErr;

		if (!bounds.IsValid())
			return genericErr;

		CGraf *graf = new (grafStorage) CGraf();
		int initError = graf->Init(bounds, pixelFormat);
		if (initError)
		{
			DisposeGWorld(graf);
			return initError;
		}

		*gw = graf;
		return noErr;
	}

	QDState *QDManagerImpl::GetState()
	{
		return m_port->GetState();
	}

	QDManagerImpl *QDManagerImpl::GetInstance()
	{
		return &ms_instance;
	}

	QDManagerImpl QDManagerImpl::ms_instance;

	QDManager *QDManager::GetInstance()
	{
		return QDManagerImpl::GetInstance();
	}
}