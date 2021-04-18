#include "WindowManager.h"

#include "DisplayDeviceManager.h"
#include "IGpDisplayDriver.h"
#include "IGpDisplayDriverSurface.h"
#include "IGpLogDriver.h"
#include "FontFamily.h"
#include "MemoryManager.h"
#include "MenuManager.h"
#include "QDGraf.h"
#include "QDManager.h"
#include "QDPixMap.h"
#include "PLTimeTaggedVOSEvent.h"
#include "Rect2i.h"
#include "RenderedFont.h"
#include "GpRenderedFontMetrics.h"
#include "ResolveCachingColor.h"
#include "Vec2i.h"
#include "WindowDef.h"

#include "PLCore.h"
#include "PLDrivers.h"
#include "PLEventQueue.h"
#include "PLStandardColors.h"
#include "PLSysCalls.h"

#include <algorithm>

struct GDevice;

namespace PortabilityLayer
{
	class CGrafImpl;
	class WindowImpl;

	struct WindowChromeTheme
	{
		virtual void GetChromePadding(const WindowImpl *window, uint16_t padding[WindowChromeSides::kCount]) const = 0;
		virtual void RenderChrome(WindowImpl *window, DrawSurface *surface, WindowChromeSide_t chromeSide) const = 0;
		virtual bool GetChromeInteractionZone(const WindowImpl *window, const Vec2i &point, RegionID_t &outRegion) const = 0;
		virtual bool GetChromeRegionRect(const WindowImpl *window, RegionID_t region, Rect2i &outRect) const = 0;
		virtual void UpdateRegionChromeState(const WindowImpl *window, RegionID_t region, const void *data) const = 0;
	};

	template<class T>
	struct WindowChromeThemeSingleton : public WindowChromeTheme
	{
	public:
		static T *GetInstance();

	private:
		static T ms_instance;
	};

	class SimpleBoxChromeTheme final : public WindowChromeThemeSingleton<SimpleBoxChromeTheme>
	{
	public:
		void GetChromePadding(const WindowImpl *window, uint16_t padding[WindowChromeSides::kCount]) const override;
		void RenderChrome(WindowImpl *window, DrawSurface *surface, WindowChromeSide_t chromeSide) const override;
		bool GetChromeInteractionZone(const WindowImpl *window, const Vec2i &point, RegionID_t &outRegion) const override;
		bool GetChromeRegionRect(const WindowImpl *window, RegionID_t region, Rect2i &outRect) const override;
		void UpdateRegionChromeState(const WindowImpl *window, RegionID_t region, const void *data) const override;
	};

	class GenericWindowChromeTheme final : public WindowChromeThemeSingleton<GenericWindowChromeTheme>
	{
	public:
		void GetChromePadding(const WindowImpl *window, uint16_t padding[WindowChromeSides::kCount]) const override;
		void RenderChrome(WindowImpl *window, DrawSurface *surface, WindowChromeSide_t chromeSide) const override;
		bool GetChromeInteractionZone(const WindowImpl *window, const Vec2i &point, RegionID_t &outRegion) const override;
		bool GetChromeRegionRect(const WindowImpl *window, RegionID_t region, Rect2i &outRect) const override;
		void UpdateRegionChromeState(const WindowImpl *window, RegionID_t region, const void *data) const override;

	private:
		void RenderChromeTop(WindowImpl *window, DrawSurface *surface) const;
		void RenderChromeLeft(WindowImpl *window, DrawSurface *surface) const;
		void RenderChromeBottom(WindowImpl *window, DrawSurface *surface) const;
		void RenderChromeRight(WindowImpl *window, DrawSurface *surface) const;

		void RenderChromeTopMini(WindowImpl *window, DrawSurface *surface) const;
		void RenderChromeLeftMini(WindowImpl *window, DrawSurface *surface) const;
		void RenderChromeBottomMini(WindowImpl *window, DrawSurface *surface) const;
		void RenderChromeRightMini(WindowImpl *window, DrawSurface *surface) const;

		static Rect GetCloseBoxRectInTopChrome();
		static Rect GetCloseBoxRectInTopChromeMini();

		void RenderChromeCloseBox(DrawSurface *surface, const Rect &windowRect, bool isClicked) const;

		static const int kDarkGray = 85;
		static const int kMidGray = 204;
		static const int kLightGray = 221;
	};

	class AlertWindowChromeTheme final : public WindowChromeThemeSingleton<AlertWindowChromeTheme>
	{
	public:
		void GetChromePadding(const WindowImpl *window, uint16_t padding[WindowChromeSides::kCount]) const override;
		void RenderChrome(WindowImpl *window, DrawSurface *surface, WindowChromeSide_t chromeSide) const override;
		bool GetChromeInteractionZone(const WindowImpl *window, const Vec2i &point, RegionID_t &outRegion) const override;
		bool GetChromeRegionRect(const WindowImpl *window, RegionID_t region, Rect2i &outRect) const override;
		void UpdateRegionChromeState(const WindowImpl *window, RegionID_t region, const void *data) const override;

	private:
		void RenderChromeTop(WindowImpl *window, DrawSurface *surface) const;
		void RenderChromeLeft(WindowImpl *window, DrawSurface *surface) const;
		void RenderChromeBottom(WindowImpl *window, DrawSurface *surface) const;
		void RenderChromeRight(WindowImpl *window, DrawSurface *surface) const;

		static const RGBAColor kDarkColor;
		static const RGBAColor kMidColor;
		static const RGBAColor kLightColor;
	};

	struct WindowEffects
	{
		WindowEffects();

		float m_desaturationLevel;
	};

	class WindowImpl final : public Window
	{
	public:
		WindowImpl();
		~WindowImpl();

		bool Init(const WindowDef &windowDef);
		bool Resize(int width, int height);

		WindowImpl *GetWindowAbove() const;
		WindowImpl *GetWindowBelow() const;

		void SetWindowAbove(WindowImpl *above);
		void SetWindowBelow(WindowImpl *below);

		bool IsVisible() const;
		void SetVisible(bool visible);

		void GetChromePadding(uint16_t padding[WindowChromeSides::kCount]) const;
		void GetChromeDimensions(int width, int height, Rect dimensions[WindowChromeSides::kCount]) const;
		bool GetChromeInteractionZone(const Vec2i &point, RegionID_t &outRegion) const;
		bool GetChromeRegionRect(RegionID_t region, Rect2i &outRect) const;
		void UpdateRegionChromeState(RegionID_t region, const void *data) const;

		bool IsBorderless() const;
		uint16_t GetStyleFlags() const;

		void SetTitle(const PLPasStr &str);
		const PascalStr<255> &GetTitle() const;

		WindowEffects &GetEffects();
		const WindowEffects &GetEffects() const;

	private:
		WindowEffects m_effects;

		WindowImpl *m_windowAbove;
		WindowImpl *m_windowBelow;

		WindowChromeTheme *m_chromeTheme;
		uint16_t m_styleFlags;
		bool m_visible;

		PascalStr<255> m_title;
	};

	class WindowManagerImpl final : public WindowManager
	{
	public:
		WindowManagerImpl();
		~WindowManagerImpl();

		void Init() override;

		Window *CreateWindow(const WindowDef &windowDef) override;
		void ResizeWindow(Window *window, int width, int height) override;
		void MoveWindow(Window *window, int x, int y) override;
		void PutWindowBehind(Window *window, Window *otherWindow) override;
		void ShowWindow(Window *window) override;
		void HideWindow(Window *window) override;
		void FindWindow(const Point &point, Window **outWindow, short *outRegion) const override;
		void DestroyWindow(Window *window) override;
		void DragWindow(Window *window, const Point &startPoint, const Rect &constraintRect) override;
		bool HandleCloseBoxClick(Window *window, const Point &startPoint) override;
		void SetWindowTitle(Window *window, const PLPasStr &title) override;
		Rect2i GetWindowFullRect(Window *window) const override;
		bool GetWindowChromeInteractionZone(Window *window, const Vec2i &point, RegionID_t &outRegion) const override;
		void SwapExclusiveWindow(Window *& windowRef) override;

		void FlickerWindowIn(Window *window, int32_t velocity) GP_ASYNCIFY_PARANOID_OVERRIDE;
		void FlickerWindowOut(Window *window, int32_t velocity) GP_ASYNCIFY_PARANOID_OVERRIDE;

		void SetWindowDesaturation(Window *window, float desaturationLevel) override;

		void SetResizeInProgress(Window *window, const PortabilityLayer::Vec2i &size) override;
		void ClearResizeInProgress() override;

		void RenderFrame(IGpDisplayDriver *displayDriver) override;

		void HandleScreenResolutionChange(uint32_t prevWidth, uint32_t prevHeight, uint32_t newWidth, uint32_t newHeight) override;

		Vec2i GetDisplayResolution() const override;

		void SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b) override;

		Window *GetPutInFrontSentinel() const override;

		static WindowManagerImpl *GetInstance();

	private:
		void RenderWindow(WindowImpl *window, IGpDisplayDriver *displayDriver);
		void DetachWindow(Window *window);

		void ResetResizeInProgressSurfaces();

		void ComputeFlickerEffects(const Vec2i &windowUpperLeft, int32_t offset, GpDisplayDriverSurfaceEffects &effects) const;

		WindowImpl *m_windowStackTop;
		WindowImpl *m_windowStackBottom;

		WindowImpl *m_exclusiveWindow;

		WindowImpl *m_flickerWindow;
		Vec2i m_flickerBasisCoordinate;
		Vec2i m_flickerAxis;
		int32_t m_flickerZoneSize;
		int32_t m_flickerBasisCoordinateDistance;
		int32_t m_flickerChromeDistanceOffset;

		Rect2i m_resizeInProgressRect;
		DrawSurface m_resizeInProgressHorizontalBar;
		DrawSurface m_resizeInProgressVerticalBar;
		bool m_isResizeInProgress;

		Vec2i m_displayResolution;

		static WindowManagerImpl ms_instance;

		static uint8_t ms_putInFrontSentinel;
	};


	template<class T>
	inline T *WindowChromeThemeSingleton<T>::GetInstance()
	{

		return &ms_instance;
	}

	template<class T>
	T WindowChromeThemeSingleton<T>::ms_instance;

	//---------------------------------------------------------------------------
	void SimpleBoxChromeTheme::GetChromePadding(const WindowImpl *window, uint16_t padding[WindowChromeSides::kCount]) const
	{
		padding[WindowChromeSides::kTop] = 1;
		padding[WindowChromeSides::kBottom] = 1;
		padding[WindowChromeSides::kLeft] = 1;
		padding[WindowChromeSides::kRight] = 1;
	}

	bool SimpleBoxChromeTheme::GetChromeInteractionZone(const WindowImpl *window, const Vec2i &point, RegionID_t &outRegion) const
	{
		return false;
	}

	bool SimpleBoxChromeTheme::GetChromeRegionRect(const WindowImpl *window, RegionID_t region, Rect2i &outRect) const
	{
		return false;
	}

	void SimpleBoxChromeTheme::UpdateRegionChromeState(const WindowImpl *window, RegionID_t region, const void *data) const
	{
	}

	void SimpleBoxChromeTheme::RenderChrome(WindowImpl *window, DrawSurface *surface, WindowChromeSide_t chromeSide) const
	{
		ResolveCachingColor blackColor = StdColors::Black();
		surface->FillRect((*surface->m_port.GetPixMap())->m_rect, blackColor);
	}

	//---------------------------------------------------------------------------
	void GenericWindowChromeTheme::GetChromePadding(const WindowImpl *window, uint16_t padding[WindowChromeSides::kCount]) const
	{
		if (window->GetStyleFlags() & WindowStyleFlags::kMiniBar)
		{
			padding[WindowChromeSides::kTop] = 13;
			padding[WindowChromeSides::kBottom] = 1;
			padding[WindowChromeSides::kLeft] = 1;
			padding[WindowChromeSides::kRight] = 1;

			//if (window->GetStyleFlags() & WindowStyleFlags::kResizable)
			//{
			//	padding[WindowChromeSides::kBottom] = 16;
			//	padding[WindowChromeSides::kRight] = 16;
			//}
		}
		else
		{
			padding[WindowChromeSides::kTop] = 22;
			padding[WindowChromeSides::kBottom] = 6;
			padding[WindowChromeSides::kLeft] = 6;
			padding[WindowChromeSides::kRight] = 6;

			//if (window->GetStyleFlags() & WindowStyleFlags::kResizable)
			//{
			//	padding[WindowChromeSides::kBottom] = 21;
			//	padding[WindowChromeSides::kRight] = 21;
			//}
		}
	}

	bool GenericWindowChromeTheme::GetChromeInteractionZone(const WindowImpl *window, const Vec2i &point, RegionID_t &outRegion) const
	{
		const DrawSurface *surface = window->GetDrawSurface();
		const Rect rect = (*surface->m_port.GetPixMap())->m_rect;

		const int32_t w = rect.Width();
		const int32_t h = rect.Height();

		if (window->GetStyleFlags() & WindowStyleFlags::kMiniBar)
		{
			if (point.m_x >= 0 && point.m_x < w && point.m_y < 0 && point.m_y >= -13)
			{
				if (window->GetStyleFlags() & WindowStyleFlags::kCloseBox)
				{
					uint16_t padding[WindowChromeSides::kCount];
					GetChromePadding(window, padding);

					const Rect closeBoxRect = GetCloseBoxRectInTopChromeMini();
					const Vec2i topChromePos = Vec2i(point.m_x + padding[WindowChromeSides::kLeft], point.m_y + padding[WindowChromeSides::kTop]);

					if (closeBoxRect.Contains(Point::Create(topChromePos.m_x, topChromePos.m_y)))
					{
						outRegion = RegionIDs::kClose;
						return true;
					}
				}

				outRegion = RegionIDs::kTitleBar;
				return true;
			}
		}
		else
		{
			if (point.m_x >= 0 && point.m_x < w && point.m_y < 0 && point.m_y >= -17)
			{
				outRegion = RegionIDs::kTitleBar;
				return true;
			}
		}

		if (window->GetStyleFlags() & WindowStyleFlags::kResizable)
		{
			if (point.m_x >= w - 14 && point.m_x < w && point.m_y >= h - 14 && point.m_y < h)
			{
				outRegion = RegionIDs::kResize;
				return true;
			}
		}

		return false;
	}

	bool GenericWindowChromeTheme::GetChromeRegionRect(const WindowImpl *window, RegionID_t region, Rect2i &outRect) const
	{
		if (region == RegionIDs::kClose)
		{
			if (window->GetStyleFlags() & WindowStyleFlags::kCloseBox)
			{
				if (window->GetStyleFlags() & WindowStyleFlags::kMiniBar)
				{
					uint16_t padding[WindowChromeSides::kCount];
					GetChromePadding(window, padding);

					const Rect closeBoxRect = GetCloseBoxRectInTopChromeMini();
					outRect = Rect2i(closeBoxRect.top - padding[WindowChromeSides::kTop], closeBoxRect.left - padding[WindowChromeSides::kLeft],
						closeBoxRect.bottom - padding[WindowChromeSides::kTop], closeBoxRect.right - padding[WindowChromeSides::kLeft]);
					return true;
				}
			}
		}

		return false;
	}

	void GenericWindowChromeTheme::UpdateRegionChromeState(const WindowImpl *window, RegionID_t region, const void *data) const
	{
		if (region == RegionIDs::kClose)
		{
			const bool state = *static_cast<const bool*>(data);

			if (window->GetStyleFlags() & WindowStyleFlags::kCloseBox)
			{
				if (window->GetStyleFlags() & WindowStyleFlags::kMiniBar)
				{
					DrawSurface *topChromeSurface = window->GetChromeSurface(WindowChromeSides::kTop);
					RenderChromeCloseBox(topChromeSurface, GetCloseBoxRectInTopChromeMini(), state);
				}
			}
		}
	}

	void GenericWindowChromeTheme::RenderChrome(WindowImpl *window, DrawSurface *surface, WindowChromeSide_t chromeSide) const
	{
		if (window->GetStyleFlags() & WindowStyleFlags::kMiniBar)
		{
			switch (chromeSide)
			{
			case WindowChromeSides::kTop:
				RenderChromeTopMini(window, surface);
				break;
			case WindowChromeSides::kLeft:
				RenderChromeLeftMini(window, surface);
				break;
			case WindowChromeSides::kBottom:
				RenderChromeBottomMini(window, surface);
				break;
			case WindowChromeSides::kRight:
				RenderChromeRightMini(window, surface);
				break;
			default:
				break;
			}
		}
		else
		{
			switch (chromeSide)
			{
			case WindowChromeSides::kTop:
				RenderChromeTop(window, surface);
				break;
			case WindowChromeSides::kLeft:
				RenderChromeLeft(window, surface);
				break;
			case WindowChromeSides::kBottom:
				RenderChromeBottom(window, surface);
				break;
			case WindowChromeSides::kRight:
				RenderChromeRight(window, surface);
				break;
			default:
				break;
			}
		}
	}

	void GenericWindowChromeTheme::RenderChromeTop(WindowImpl *window, DrawSurface *surface) const
	{
		ResolveCachingColor blackColor = StdColors::Black();
		ResolveCachingColor darkGrayColor = RGBAColor::Create(kDarkGray, kDarkGray, kDarkGray, 255);
		ResolveCachingColor midGrayColor = RGBAColor::Create(kMidGray, kMidGray, kMidGray, 255);
		ResolveCachingColor lightGrayColor = RGBAColor::Create(kLightGray, kLightGray, kLightGray, 255);
		ResolveCachingColor whiteColor = StdColors::White();

		const Rect rect = (*surface->m_port.GetPixMap())->m_rect;

		surface->FillRect(rect, midGrayColor);

		surface->FillRect(Rect::Create(rect.top, rect.left, rect.top + 1, rect.right), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.left, rect.bottom, 1), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 1, rect.bottom, rect.right), blackColor);
		surface->FillRect(Rect::Create(rect.bottom - 1, rect.left + 5, rect.bottom, rect.right - 5), blackColor);

		surface->FillRect(Rect::Create(rect.bottom - 2, rect.left + 4, rect.bottom - 1, rect.right - 5), darkGrayColor);
		surface->FillRect(Rect::Create(rect.bottom - 2, rect.left + 4, rect.bottom, rect.left + 5), darkGrayColor);
		surface->FillRect(Rect::Create(rect.top + 2, rect.right - 2, rect.bottom, rect.right - 1), darkGrayColor);

		surface->FillRect(Rect::Create(rect.top + 1, rect.left + 1, rect.bottom, rect.left + 2), whiteColor);
		surface->FillRect(Rect::Create(rect.top + 1, rect.left + 1, rect.top + 2, rect.right - 2), whiteColor);
		surface->FillRect(Rect::Create(rect.bottom - 1, rect.right - 5, rect.bottom, rect.right - 4), whiteColor);

		if (window->GetStyleFlags() & WindowStyleFlags::kCloseBox)
			RenderChromeCloseBox(surface, rect, false);

		PortabilityLayer::RenderedFont *sysFont = GetFont(FontPresets::kSystem12Bold);
		int32_t ascender = sysFont->GetMetrics().m_ascent;

		const PLPasStr titlePStr = window->GetTitle().ToShortStr();
		size_t titleWidth = sysFont->MeasureString(titlePStr.UChars(), titlePStr.Length());

		int32_t titleH = (rect.left + rect.right - static_cast<int32_t>(titleWidth) + 1) / 2;
		int32_t titleV = (rect.top + rect.bottom + ascender + 1) / 2;

		surface->DrawString(Point::Create(titleH, titleV), titlePStr, blackColor, sysFont);

		Rect leftStripesRect = Rect::Create(rect.top + 5, rect.left + 5, rect.bottom - 5, (rect.left + rect.right - static_cast<int16_t>(titleWidth) - 10) / 2);
		Rect rightStripesRect = Rect::Create(rect.top + 5, (rect.left + rect.right + static_cast<int16_t>(titleWidth) + 10) / 2, rect.bottom - 5, rect.right - 5);

		if (leftStripesRect.IsValid() && rightStripesRect.IsValid())
		{
			for (int i = leftStripesRect.top; i < leftStripesRect.bottom; i += 3)
			{
				surface->FillRect(Rect::Create(i, leftStripesRect.left, i + 2, leftStripesRect.right), lightGrayColor);
				surface->FillRect(Rect::Create(i, rightStripesRect.left, i + 2, rightStripesRect.right), lightGrayColor);

				surface->FillRect(Rect::Create(i + 1, leftStripesRect.left + 1, i + 2, leftStripesRect.right), darkGrayColor);
				surface->FillRect(Rect::Create(i + 1, rightStripesRect.left + 1, i + 2, rightStripesRect.right), darkGrayColor);
			}
		}
	}

	void GenericWindowChromeTheme::RenderChromeLeft(WindowImpl *window, DrawSurface *surface) const
	{
		ResolveCachingColor blackColor = StdColors::Black();
		ResolveCachingColor darkGrayColor = RGBAColor::Create(kDarkGray, kDarkGray, kDarkGray, 255);
		ResolveCachingColor midGrayColor = RGBAColor::Create(kMidGray, kMidGray, kMidGray, 255);
		ResolveCachingColor whiteColor = StdColors::White();

		const Rect rect = (*surface->m_port.GetPixMap())->m_rect;

		surface->FillRect(Rect::Create(rect.top, rect.right - 6, rect.bottom, rect.right - 5), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 1, rect.bottom, rect.right), blackColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 5, rect.bottom, rect.right - 4), whiteColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 2, rect.bottom, rect.right - 1), darkGrayColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 4, rect.bottom, rect.right - 2), midGrayColor);
	}

	void GenericWindowChromeTheme::RenderChromeBottom(WindowImpl *window, DrawSurface *surface) const
	{
		ResolveCachingColor blackColor = StdColors::Black();
		ResolveCachingColor darkGrayColor = RGBAColor::Create(kDarkGray, kDarkGray, kDarkGray, 255);
		ResolveCachingColor midGrayColor = RGBAColor::Create(kMidGray, kMidGray, kMidGray, 255);
		ResolveCachingColor whiteColor = StdColors::White();

		const Rect rect = (*surface->m_port.GetPixMap())->m_rect;

		surface->FillRect(Rect::Create(rect.top, rect.left + 1, rect.bottom - 1, rect.left + 5), midGrayColor);
		surface->FillRect(Rect::Create(rect.bottom - 4, rect.left + 5, rect.bottom - 2, rect.right - 4), midGrayColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 4, rect.bottom - 2, rect.right - 2), midGrayColor);

		surface->FillRect(Rect::Create(rect.top, rect.left, rect.bottom, rect.left + 1), blackColor);
		surface->FillRect(Rect::Create(rect.bottom - 1, rect.left, rect.bottom, rect.right), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 1, rect.bottom, rect.right), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.left + 5, rect.bottom - 5, rect.left + 6), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 6, rect.bottom - 5, rect.right - 5), blackColor);
		surface->FillRect(Rect::Create(rect.bottom - 6, rect.left + 6, rect.bottom - 5, rect.right - 6), blackColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 5, rect.bottom - 5, rect.right - 4), whiteColor);
		surface->FillRect(Rect::Create(rect.top, rect.left + 1, rect.bottom - 2, rect.left + 2), whiteColor);
		surface->FillRect(Rect::Create(rect.bottom - 5, rect.left + 5, rect.bottom - 4, rect.right - 4), whiteColor);

		surface->FillRect(Rect::Create(rect.bottom - 2, rect.left + 2, rect.bottom - 1, rect.right - 2), darkGrayColor);
		surface->FillRect(Rect::Create(rect.top, rect.left + 4, rect.bottom - 5, rect.left + 5), darkGrayColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 2, rect.bottom - 1, rect.right - 1), darkGrayColor);
	}

	void GenericWindowChromeTheme::RenderChromeRight(WindowImpl *window, DrawSurface *surface) const
	{
		ResolveCachingColor blackColor = StdColors::Black();
		ResolveCachingColor darkGrayColor = RGBAColor::Create(kDarkGray, kDarkGray, kDarkGray, 255);
		ResolveCachingColor midGrayColor = RGBAColor::Create(kMidGray, kMidGray, kMidGray, 255);
		ResolveCachingColor whiteColor = StdColors::White();

		const Rect rect = (*surface->m_port.GetPixMap())->m_rect;

		surface->FillRect(Rect::Create(rect.top, rect.right - 6, rect.bottom, rect.right - 5), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 1, rect.bottom, rect.right), blackColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 5, rect.bottom, rect.right - 4), whiteColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 2, rect.bottom, rect.right - 1), darkGrayColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 4, rect.bottom, rect.right - 2), midGrayColor);
	}

	void GenericWindowChromeTheme::RenderChromeTopMini(WindowImpl *window, DrawSurface *surface) const
	{
		ResolveCachingColor blackColor = StdColors::Black();
		ResolveCachingColor darkGrayColor = RGBAColor::Create(kDarkGray, kDarkGray, kDarkGray, 255);
		ResolveCachingColor lightGrayColor = RGBAColor::Create(kLightGray, kLightGray, kLightGray, 255);
		ResolveCachingColor whiteColor = StdColors::White();

		const Rect rect = (*surface->m_port.GetPixMap())->m_rect;

		surface->FillRect(rect, lightGrayColor);

		surface->FillRect(Rect::Create(rect.top, rect.left, rect.top + 1, rect.right), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.left, rect.bottom, 1), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 1, rect.bottom, rect.right), blackColor);
		surface->FillRect(Rect::Create(rect.bottom - 1, rect.left + 1, rect.bottom, rect.right - 1), blackColor);

		surface->FillRect(Rect::Create(rect.bottom - 2, rect.left + 2, rect.bottom - 1, rect.right - 2), darkGrayColor);
		surface->FillRect(Rect::Create(rect.top + 2, rect.right - 2, rect.bottom - 1, rect.right - 1), darkGrayColor);

		surface->FillRect(Rect::Create(rect.top + 1, rect.left + 1, rect.bottom - 2, rect.left + 2), whiteColor);
		surface->FillRect(Rect::Create(rect.top + 1, rect.left + 1, rect.top + 2, rect.right - 2), whiteColor);

		if (window->GetStyleFlags() & WindowStyleFlags::kCloseBox)
			RenderChromeCloseBox(surface, rect, false);

		PortabilityLayer::RenderedFont *appFont = GetFont(PortabilityLayer::FontPresets::kApplication10Bold);
		int32_t ascender = appFont->GetMetrics().m_ascent;

		const PLPasStr titlePStr = window->GetTitle().ToShortStr();
		size_t titleWidth = appFont->MeasureString(titlePStr.UChars(), titlePStr.Length());

		int32_t titleH = (rect.left + rect.right - static_cast<int32_t>(titleWidth) + 1) / 2;
		int32_t titleV = (rect.top + rect.bottom + ascender + 1) / 2;

		surface->DrawString(Point::Create(titleH, titleV), titlePStr, blackColor, appFont);
	}

	void GenericWindowChromeTheme::RenderChromeLeftMini(WindowImpl *window, DrawSurface *surface) const
	{
		const Rect rect = (*surface->m_port.GetPixMap())->m_rect;

		ResolveCachingColor blackColor = StdColors::Black();
		surface->FillRect(rect, blackColor);
	}

	void GenericWindowChromeTheme::RenderChromeBottomMini(WindowImpl *window, DrawSurface *surface) const
	{
		const Rect rect = (*surface->m_port.GetPixMap())->m_rect;

		ResolveCachingColor blackColor = StdColors::Black();
		surface->FillRect(rect, blackColor);
	}

	void GenericWindowChromeTheme::RenderChromeRightMini(WindowImpl *window, DrawSurface *surface) const
	{
		const Rect rect = (*surface->m_port.GetPixMap())->m_rect;

		ResolveCachingColor blackColor = StdColors::Black();
		surface->FillRect(rect, blackColor);
	}

	Rect GenericWindowChromeTheme::GetCloseBoxRectInTopChrome()
	{
		return GetCloseBoxRectInTopChromeMini();	// Temp, maybe... I don't think this is ever actually used
	}

	Rect GenericWindowChromeTheme::GetCloseBoxRectInTopChromeMini()
	{
		int boxDimensions = 8;
		return Rect::Create(2, 2, 2 + boxDimensions, 2 + boxDimensions);
	}

	void GenericWindowChromeTheme::RenderChromeCloseBox(DrawSurface *surface, const Rect &chromeRect, bool isClicked) const
	{
		const Rect closeBoxRect = GetCloseBoxRectInTopChromeMini();

		if (isClicked)
		{
			ResolveCachingColor blackColor = StdColors::Black();
			ResolveCachingColor darkGrayColor = RGBAColor::Create(kDarkGray, kDarkGray, kDarkGray, 255);

			surface->FillRect(closeBoxRect, blackColor);

			surface->FillRect(Rect::Create(closeBoxRect.top + 1, closeBoxRect.left + 1, closeBoxRect.bottom - 1, closeBoxRect.right - 1), darkGrayColor);
		}
		else
		{
			ResolveCachingColor darkGrayColor = RGBAColor::Create(kDarkGray, kDarkGray, kDarkGray, 255);
			ResolveCachingColor midGrayColor = RGBAColor::Create(kMidGray, kMidGray, kMidGray, 255);
			ResolveCachingColor lightGrayColor = RGBAColor::Create(kLightGray, kLightGray, kLightGray, 255);
			surface->FillRect(closeBoxRect, darkGrayColor);

			surface->FillRect(Rect::Create(closeBoxRect.top + 1, closeBoxRect.left + 1, closeBoxRect.bottom, closeBoxRect.right), midGrayColor);

			surface->FillRect(Rect::Create(closeBoxRect.top + 2, closeBoxRect.left + 2, closeBoxRect.bottom - 1, closeBoxRect.right - 1), darkGrayColor);

			surface->FillRect(Rect::Create(closeBoxRect.top + 2, closeBoxRect.left + 2, closeBoxRect.bottom - 2, closeBoxRect.right - 2), lightGrayColor);
		}
	}

	//---------------------------------------------------------------------------
	// Alert chrome theme
	const RGBAColor AlertWindowChromeTheme::kDarkColor = RGBAColor::Create(255, 51, 51, 255);
	const RGBAColor AlertWindowChromeTheme::kMidColor = RGBAColor::Create(255, 153, 51, 255);
	const RGBAColor AlertWindowChromeTheme::kLightColor = RGBAColor::Create(255, 204, 51, 255);


	void AlertWindowChromeTheme::GetChromePadding(const WindowImpl *window, uint16_t padding[WindowChromeSides::kCount]) const
	{
		padding[WindowChromeSides::kTop] = 6;
		padding[WindowChromeSides::kBottom] = 6;
		padding[WindowChromeSides::kLeft] = 6;
		padding[WindowChromeSides::kRight] = 6;
	}

	bool AlertWindowChromeTheme::GetChromeInteractionZone(const WindowImpl *window, const Vec2i &point, RegionID_t &outRegion) const
	{
		return false;
	}

	bool AlertWindowChromeTheme::GetChromeRegionRect(const WindowImpl *window, RegionID_t region, Rect2i &outRect) const
	{
		return false;
	}

	void AlertWindowChromeTheme::UpdateRegionChromeState(const WindowImpl *window, RegionID_t region, const void *data) const
	{
	}

	void AlertWindowChromeTheme::RenderChrome(WindowImpl *window, DrawSurface *surface, WindowChromeSide_t chromeSide) const
	{
		switch (chromeSide)
		{
		case WindowChromeSides::kTop:
			RenderChromeTop(window, surface);
			break;
		case WindowChromeSides::kLeft:
			RenderChromeLeft(window, surface);
			break;
		case WindowChromeSides::kBottom:
			RenderChromeBottom(window, surface);
			break;
		case WindowChromeSides::kRight:
			RenderChromeRight(window, surface);
			break;
		default:
			break;
		}
	}

	void AlertWindowChromeTheme::RenderChromeTop(WindowImpl *window, DrawSurface *surface) const
	{
		ResolveCachingColor darkColor = kDarkColor;
		ResolveCachingColor midColor = kMidColor;
		ResolveCachingColor lightColor = kLightColor;
		ResolveCachingColor blackColor = StdColors::Black();

		const Rect rect = (*surface->m_port.GetPixMap())->m_rect;

		surface->FillRect(rect, midColor);

		surface->FillRect(Rect::Create(rect.top, rect.left, rect.top + 1, rect.right), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.left, rect.bottom, 1), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 1, rect.bottom, rect.right), blackColor);
		surface->FillRect(Rect::Create(rect.bottom - 1, rect.left + 5, rect.bottom, rect.right - 5), blackColor);

		surface->FillRect(Rect::Create(rect.bottom - 2, rect.left + 4, rect.bottom - 1, rect.right - 5), darkColor);
		surface->FillRect(Rect::Create(rect.bottom - 2, rect.left + 4, rect.bottom, rect.left + 5), darkColor);
		surface->FillRect(Rect::Create(rect.top + 2, rect.right - 2, rect.bottom, rect.right - 1), darkColor);

		surface->FillRect(Rect::Create(rect.top + 1, rect.left + 1, rect.bottom, rect.left + 2), lightColor);
		surface->FillRect(Rect::Create(rect.top + 1, rect.left + 1, rect.top + 2, rect.right - 2), lightColor);
		surface->FillRect(Rect::Create(rect.bottom - 1, rect.right - 5, rect.bottom, rect.right - 4), lightColor);
	}

	void AlertWindowChromeTheme::RenderChromeLeft(WindowImpl *window, DrawSurface *surface) const
	{
		ResolveCachingColor darkColor = kDarkColor;
		ResolveCachingColor midColor = kMidColor;
		ResolveCachingColor lightColor = kLightColor;
		ResolveCachingColor blackColor = StdColors::Black();

		const Rect rect = (*surface->m_port.GetPixMap())->m_rect;

		surface->FillRect(Rect::Create(rect.top, rect.right - 6, rect.bottom, rect.right - 5), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 1, rect.bottom, rect.right), blackColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 5, rect.bottom, rect.right - 4), lightColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 2, rect.bottom, rect.right - 1), darkColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 4, rect.bottom, rect.right - 2), midColor);
	}

	void AlertWindowChromeTheme::RenderChromeBottom(WindowImpl *window, DrawSurface *surface) const
	{
		ResolveCachingColor darkColor = kDarkColor;
		ResolveCachingColor midColor = kMidColor;
		ResolveCachingColor lightColor = kLightColor;
		ResolveCachingColor blackColor = StdColors::Black();

		const Rect rect = (*surface->m_port.GetPixMap())->m_rect;

		surface->FillRect(Rect::Create(rect.top, rect.left + 1, rect.bottom - 1, rect.left + 5), midColor);
		surface->FillRect(Rect::Create(rect.bottom - 4, rect.left + 5, rect.bottom - 2, rect.right - 4), midColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 4, rect.bottom - 2, rect.right - 2), midColor);

		surface->FillRect(Rect::Create(rect.top, rect.left, rect.bottom, rect.left + 1), blackColor);
		surface->FillRect(Rect::Create(rect.bottom - 1, rect.left, rect.bottom, rect.right), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 1, rect.bottom, rect.right), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.left + 5, rect.bottom - 5, rect.left + 6), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 6, rect.bottom - 5, rect.right - 5), blackColor);
		surface->FillRect(Rect::Create(rect.bottom - 6, rect.left + 6, rect.bottom - 5, rect.right - 6), blackColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 5, rect.bottom - 5, rect.right - 4), lightColor);
		surface->FillRect(Rect::Create(rect.top, rect.left + 1, rect.bottom - 2, rect.left + 2), lightColor);
		surface->FillRect(Rect::Create(rect.bottom - 5, rect.left + 5, rect.bottom - 4, rect.right - 4), lightColor);

		surface->FillRect(Rect::Create(rect.bottom - 2, rect.left + 2, rect.bottom - 1, rect.right - 2), darkColor);
		surface->FillRect(Rect::Create(rect.top, rect.left + 4, rect.bottom - 5, rect.left + 5), darkColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 2, rect.bottom - 1, rect.right - 1), darkColor);
	}

	void AlertWindowChromeTheme::RenderChromeRight(WindowImpl *window, DrawSurface *surface) const
	{
		ResolveCachingColor darkColor = kDarkColor;
		ResolveCachingColor midColor = kMidColor;
		ResolveCachingColor lightColor = kLightColor;
		ResolveCachingColor blackColor = StdColors::Black();

		const Rect rect = (*surface->m_port.GetPixMap())->m_rect;

		surface->FillRect(Rect::Create(rect.top, rect.right - 6, rect.bottom, rect.right - 5), blackColor);
		surface->FillRect(Rect::Create(rect.top, rect.right - 1, rect.bottom, rect.right), blackColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 5, rect.bottom, rect.right - 4), lightColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 2, rect.bottom, rect.right - 1), darkColor);

		surface->FillRect(Rect::Create(rect.top, rect.right - 4, rect.bottom, rect.right - 2), midColor);
	}

	//---------------------------------------------------------------------------
	WindowEffects::WindowEffects()
		: m_desaturationLevel(0.0f)
	{
	}

	//---------------------------------------------------------------------------
	WindowImpl::WindowImpl()
		: m_windowAbove(nullptr)
		, m_windowBelow(nullptr)
		, m_chromeTheme(nullptr)
		, m_visible(true)
		, m_styleFlags(0)
	{
	}

	WindowImpl::~WindowImpl()
	{
		assert(m_windowAbove == nullptr && m_windowBelow == nullptr);
	}

	bool WindowImpl::Init(const WindowDef &windowDef)
	{
		const Rect bounds = windowDef.m_initialRect;

		if (!bounds.IsValid())
			return false;

		m_styleFlags = windowDef.m_styleFlags;

		const Rect adjustedBounds = Rect::Create(0, 0, bounds.Height(), bounds.Width());
		m_wmX = bounds.left;
		m_wmY = bounds.top;

		GpPixelFormat_t pixelFormat = PortabilityLayer::DisplayDeviceManager::GetInstance()->GetPixelFormat();

		if (int errorCode = m_surface.Init(adjustedBounds, pixelFormat))
			return false;

		PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();
		m_surface.FillRect(adjustedBounds, whiteColor);

		m_title.Set(windowDef.m_title[0], reinterpret_cast<const char*>(windowDef.m_title + 1));

		// Resolve chrome
		if (!IsBorderless())
		{
			m_chromeTheme = SimpleBoxChromeTheme::GetInstance();

			if (m_styleFlags & WindowStyleFlags::kTitleBar)
				m_chromeTheme = GenericWindowChromeTheme::GetInstance();
			else if (m_styleFlags & WindowStyleFlags::kAlert)
				m_chromeTheme = AlertWindowChromeTheme::GetInstance();

			Rect chromeBounds[WindowChromeSides::kCount];
			GetChromeDimensions(bounds.Width(), bounds.Height(), chromeBounds);

			for (int chromeSide = 0; chromeSide < WindowChromeSides::kCount; chromeSide++)
			{
				if (int errorCode = m_chromeSurfaces[chromeSide].Init(chromeBounds[chromeSide], pixelFormat))
					return false;

				m_chromeTheme->RenderChrome(this, m_chromeSurfaces + chromeSide, static_cast<WindowChromeSide_t>(chromeSide));
			}
		}

		return true;
	}

	bool WindowImpl::Resize(int width, int height)
	{
		Rect rect = m_surface.m_port.GetRect();
		rect.right = rect.left + width;
		rect.bottom = rect.top + height;

		if (!m_surface.Resize(rect))
			return false;

		if (!IsBorderless())
		{
			Rect chromeDimensions[WindowChromeSides::kCount];

			GetChromeDimensions(width, height, chromeDimensions);

			bool resized = true;
			for (int i = 0; i < WindowChromeSides::kCount; i++)
			{
				if (!m_chromeSurfaces[i].Resize(chromeDimensions[i]))
					return false;

				m_chromeTheme->RenderChrome(this, m_chromeSurfaces + i, static_cast<WindowChromeSide_t>(i));
			}
		}

		return true;
	}

	WindowImpl *WindowImpl::GetWindowAbove() const
	{
		return m_windowAbove;
	}

	WindowImpl *WindowImpl::GetWindowBelow() const
	{
		return m_windowBelow;
	}

	void WindowImpl::SetWindowAbove(WindowImpl *above)
	{
		m_windowAbove = above;
	}

	void WindowImpl::SetWindowBelow(WindowImpl *below)
	{
		m_windowBelow = below;
	}

	bool WindowImpl::IsVisible() const
	{
		return m_visible;
	}

	void WindowImpl::SetVisible(bool visible)
	{
		m_visible = visible;
	}

	void WindowImpl::GetChromePadding(uint16_t padding[WindowChromeSides::kCount]) const
	{
		if (m_chromeTheme)
			m_chromeTheme->GetChromePadding(this, padding);
		else
		{
			for (int i = 0; i < WindowChromeSides::kCount; i++)
				padding[i] = 0;
		}
	}

	void WindowImpl::GetChromeDimensions(int width, int height, Rect dimensions[WindowChromeSides::kCount]) const
	{
		uint16_t padding[WindowChromeSides::kCount];
		GetChromePadding(padding);

		int32_t topAndBottomWidth = width + padding[WindowChromeSides::kLeft] + padding[WindowChromeSides::kRight];
		int32_t leftAndRightHeight = height;

		dimensions[WindowChromeSides::kTop] = Rect::Create(0, 0, padding[WindowChromeSides::kTop], topAndBottomWidth);
		dimensions[WindowChromeSides::kBottom] = Rect::Create(0, 0, padding[WindowChromeSides::kBottom], topAndBottomWidth);
		dimensions[WindowChromeSides::kLeft] = Rect::Create(0, 0, leftAndRightHeight, padding[WindowChromeSides::kLeft]);
		dimensions[WindowChromeSides::kRight] = Rect::Create(0, 0, leftAndRightHeight, padding[WindowChromeSides::kRight]);
	}

	bool WindowImpl::GetChromeInteractionZone(const Vec2i &point, RegionID_t &outRegion) const
	{
		if (!m_chromeTheme)
			return false;

		return m_chromeTheme->GetChromeInteractionZone(this, point, outRegion);
	}

	bool WindowImpl::GetChromeRegionRect(RegionID_t region, Rect2i &outRect) const
	{
		if (!m_chromeTheme)
			return false;

		return m_chromeTheme->GetChromeRegionRect(this, region, outRect);
	}

	void WindowImpl::UpdateRegionChromeState(RegionID_t region, const void *data) const
	{
		if (!m_chromeTheme)
			return;

		m_chromeTheme->UpdateRegionChromeState(this, region, data);
	}

	bool WindowImpl::IsBorderless() const
	{
		return (m_styleFlags & WindowStyleFlags::kBorderless) != 0;
	}

	uint16_t WindowImpl::GetStyleFlags() const
	{
		return m_styleFlags;
	}

	void WindowImpl::SetTitle(const PLPasStr &str)
	{
		m_title.Set(str.Length(), str.Chars());
		m_chromeTheme->RenderChrome(this, m_chromeSurfaces + WindowChromeSides::kTop, WindowChromeSides::kTop);
	}

	const PascalStr<255> &WindowImpl::GetTitle() const
	{
		return m_title;
	}

	WindowEffects &WindowImpl::GetEffects()
	{
		return m_effects;
	}

	const WindowEffects &WindowImpl::GetEffects() const
	{
		return m_effects;
	}

	WindowManagerImpl::WindowManagerImpl()
		: m_windowStackTop(nullptr)
		, m_windowStackBottom(nullptr)
		, m_resizeInProgressRect(Rect2i(0, 0, 0, 0))
		, m_isResizeInProgress(false)
		, m_exclusiveWindow(nullptr)
		, m_flickerWindow(nullptr)
		, m_flickerBasisCoordinate(0, 0)
		, m_flickerAxis(0, 0)
		, m_flickerZoneSize(0)
		, m_flickerBasisCoordinateDistance(0)
		, m_flickerChromeDistanceOffset(0)
		, m_displayResolution(0, 0)
	{
	}

	WindowManagerImpl::~WindowManagerImpl()
	{
	}

	void WindowManagerImpl::Init()
	{
		unsigned int displayWidth, displayHeight;
		PLDrivers::GetDisplayDriver()->GetInitialDisplayResolution(&displayWidth, &displayHeight);

		m_displayResolution = Vec2i(displayWidth, displayHeight);

		IGpLogDriver *logger = PLDrivers::GetLogDriver();

		if (logger)
			logger->Printf(IGpLogDriver::Category_Information, "WindowManagerImpl: Initialized at resolution %ix%i", static_cast<int>(displayWidth), static_cast<int>(displayHeight));
	}

	Window *WindowManagerImpl::CreateWindow(const WindowDef &windowDef)
	{
		void *windowMem = MemoryManager::GetInstance()->Alloc(sizeof(WindowImpl));
		if (!windowMem)
			return nullptr;

		Rect portRect = windowDef.m_initialRect;
		if (!portRect.IsValid())
			return nullptr;

		WindowImpl *window = new (windowMem) WindowImpl();
		if (!window->Init(windowDef))
		{
			window->~WindowImpl();
			MemoryManager::GetInstance()->Release(windowMem);
			return nullptr;
		}

		return window;
	}

	void WindowManagerImpl::PutWindowBehind(Window *windowBase, Window *behind)
	{
		WindowImpl *window = static_cast<WindowImpl*>(windowBase);

		if (window->GetWindowAbove() != behind)
			DetachWindow(window);

		if (behind == GetPutInFrontSentinel())
		{
			if (m_windowStackTop)
			{
				m_windowStackTop->SetWindowAbove(window);
				window->SetWindowBelow(m_windowStackTop);
				m_windowStackTop = window;
			}
			else
			{
				m_windowStackTop = m_windowStackBottom = window;
			}
		}
		else
		{
			WindowImpl *windowAbove = static_cast<WindowImpl*>(behind);
			WindowImpl *windowBelow = windowAbove->GetWindowBelow();

			window->SetWindowAbove(windowAbove);
			window->SetWindowBelow(windowBelow);

			if (windowBelow)
				windowBelow->SetWindowAbove(window);
			else
				m_windowStackBottom = window;

			windowAbove->SetWindowBelow(window);
		}
	}

	void WindowManagerImpl::ShowWindow(Window *window)
	{
		static_cast<WindowImpl*>(window)->SetVisible(true);
	}

	void WindowManagerImpl::HideWindow(Window *window)
	{
		WindowImpl *impl = static_cast<WindowImpl*>(window);
		impl->SetVisible(false);

		// Per spec, hiding a window brings the window below it to the front
		if (m_windowStackTop == impl)
		{
			if (WindowImpl *below = impl->GetWindowBelow())
				PutWindowBehind(below, GetPutInFrontSentinel());
		}
	}

	void WindowManagerImpl::FindWindow(const Point &point, Window **outWindow, short *outRegion) const
	{
		PortabilityLayer::MenuManager *menuManager = PortabilityLayer::MenuManager::GetInstance();
		if (menuManager->IsPointInMenuBar(PortabilityLayer::Vec2i(point.h, point.v)))
		{
			if (outWindow)
				*outWindow = nullptr;

			if (outRegion)
				*outRegion = RegionIDs::kMenuBar;

			return;
		}

		WindowImpl *window = m_windowStackTop;
		while (window)
		{
			const Rect windowRect = window->GetSurfaceRect();
			const Vec2i windowPos = window->GetPosition();

			const int32_t localX = point.h - windowPos.m_x;
			const int32_t localY = point.v - windowPos.m_y;

			RegionID_t chromeInteractionZone = RegionIDs::kContent;
			if (window->GetChromeInteractionZone(Vec2i(localX, localY), chromeInteractionZone))
			{
				*outRegion = chromeInteractionZone;
				*outWindow = window;
				return;
			}

			if (localX >= 0 && localY >= 0 && localX < windowRect.right && localY < windowRect.bottom)
			{
				if (outWindow)
					*outWindow = window;

				if (outRegion)
					*outRegion = RegionIDs::kContent;

				return;
			}

			window = window->GetWindowBelow();
		}

		if (outWindow)
			*outWindow = nullptr;

		if (outRegion)
			*outRegion = 0;
	}

	void WindowManagerImpl::DestroyWindow(Window *window)
	{
		WindowImpl *windowImpl = static_cast<WindowImpl*>(window);

		assert(windowImpl != m_exclusiveWindow);

		DetachWindow(window);

		windowImpl->~WindowImpl();
		PortabilityLayer::MemoryManager::GetInstance()->Release(windowImpl);
	}

	void WindowManagerImpl::DragWindow(Window *window, const Point &startPointRef, const Rect &constraintRect)
	{
		int32_t baseX = startPointRef.h;
		int32_t baseY = startPointRef.v;

		for (;;)
		{
			TimeTaggedVOSEvent evt;
			if (WaitForEvent(&evt, 1))
			{
				if (evt.m_vosEvent.m_eventType == GpVOSEventTypes::kMouseInput)
				{
					const GpMouseInputEvent &mouseEvent = evt.m_vosEvent.m_event.m_mouseInputEvent;
					int32_t x = mouseEvent.m_x;
					int32_t y = mouseEvent.m_y;

					if (x < constraintRect.left)
						x = constraintRect.left;
					if (x >= constraintRect.right)
						x = constraintRect.right - 1;
					if (y < constraintRect.top)
						y = constraintRect.top;
					if (y >= constraintRect.bottom)
						y = constraintRect.bottom - 1;

					window->SetPosition(window->GetPosition() + Vec2i(x - baseX, y - baseY));

					baseX = x;
					baseY = y;

					if (mouseEvent.m_eventType == GpMouseEventTypes::kUp)
						break;
				}
			}
		}
	}

	bool WindowManagerImpl::HandleCloseBoxClick(Window *window, const Point &startPoint)
	{
		bool isInBounds = false;

		const Vec2i windowCoord = window->GetPosition();

		Rect2i closeBoxRect;
		if (!static_cast<WindowImpl*>(window)->GetChromeRegionRect(RegionIDs::kClose, closeBoxRect))
			return false;

		{
			const Vec2i relativeCoord = Vec2i(startPoint.h, startPoint.v) - windowCoord;
			const bool coordIsInBounds = closeBoxRect.Contains(relativeCoord);

			if (coordIsInBounds != isInBounds)
			{
				const bool state = coordIsInBounds;
				isInBounds = coordIsInBounds;

				static_cast<WindowImpl*>(window)->UpdateRegionChromeState(RegionIDs::kClose, &state);
			}
		}

		for (;;)
		{
			TimeTaggedVOSEvent evt;
			if (WaitForEvent(&evt, 1))
			{
				if (evt.m_vosEvent.m_eventType == GpVOSEventTypes::kMouseInput)
				{
					const GpMouseInputEvent &mouseEvent = evt.m_vosEvent.m_event.m_mouseInputEvent;

					const Vec2i relativeCoord = Vec2i(mouseEvent.m_x, mouseEvent.m_y) - windowCoord;
					const bool coordIsInBounds = closeBoxRect.Contains(relativeCoord);

					if (coordIsInBounds != isInBounds)
					{
						const bool state = coordIsInBounds;
						isInBounds = coordIsInBounds;

						static_cast<WindowImpl*>(window)->UpdateRegionChromeState(RegionIDs::kClose, &state);
					}

					if (mouseEvent.m_eventType == GpMouseEventTypes::kUp)
						break;
				}
			}
		}

		if (isInBounds)
		{
			const bool state = false;
			static_cast<WindowImpl*>(window)->UpdateRegionChromeState(RegionIDs::kClose, &state);
		}

		return isInBounds;
	}

	void WindowManagerImpl::SetWindowTitle(Window *window, const PLPasStr &title)
	{
		static_cast<WindowImpl*>(window)->SetTitle(title);
	}

	Rect2i WindowManagerImpl::GetWindowFullRect(Window *window) const
	{
		WindowImpl *windowImpl = static_cast<WindowImpl*>(window);

		uint16_t padding[WindowChromeSides::kCount];
		windowImpl->GetChromePadding(padding);

		const Rect portRect = windowImpl->GetSurfaceRect();
		const Vec2i windowPos = windowImpl->GetPosition();

		return Rect2i(windowPos.m_y - padding[WindowChromeSides::kTop], windowPos.m_x - padding[WindowChromeSides::kLeft], windowPos.m_y + portRect.Height() + padding[WindowChromeSides::kBottom], windowPos.m_x + portRect.Width() + padding[WindowChromeSides::kRight]);
	}

	bool WindowManagerImpl::GetWindowChromeInteractionZone(Window *window, const Vec2i &point, RegionID_t &outRegion) const
	{
		return static_cast<WindowImpl*>(window)->GetChromeInteractionZone(point, outRegion);
	}

	void WindowManagerImpl::SwapExclusiveWindow(Window *& windowRef)
	{
		const bool hadExclusiveWindow = (m_exclusiveWindow != nullptr);

		Window *temp = m_exclusiveWindow;
		m_exclusiveWindow = static_cast<WindowImpl*>(windowRef);
		windowRef = temp;

		const bool haveExclusiveWindow = (m_exclusiveWindow != nullptr);

		if (hadExclusiveWindow != haveExclusiveWindow)
			PLDrivers::GetDisplayDriver()->SetBackgroundDarkenEffect(haveExclusiveWindow);
	}

	void WindowManagerImpl::FlickerWindowIn(Window *window, int32_t velocity)
	{
		PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();
		m_flickerWindow = static_cast<WindowImpl*>(window);

		int32_t chromeLead = 64;
		int32_t flickerZoneSize = 128;

		const Rect windowRect = window->GetDrawSurface()->m_port.GetRect();

		Vec2i topLeft = m_flickerWindow->GetPosition();
		Vec2i dimensions = Vec2i(windowRect.Width(), windowRect.Height());

		m_flickerAxis = Vec2i(1, 1);
		m_flickerZoneSize = flickerZoneSize;
		m_flickerBasisCoordinate = topLeft + dimensions;
		m_flickerBasisCoordinateDistance = -chromeLead;
		m_flickerChromeDistanceOffset = chromeLead;

		int32_t axisLength = m_flickerAxis.m_x * dimensions.m_x + m_flickerAxis.m_y * dimensions.m_y;

		for (int32_t i = 0; i < axisLength + chromeLead + flickerZoneSize; i += velocity)
		{
			m_flickerBasisCoordinateDistance += velocity;
			PLSysCalls::Sleep(1);
		}

		m_flickerWindow = nullptr;
	}

	void WindowManagerImpl::FlickerWindowOut(Window *window, int32_t velocity)
	{
		PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();
		m_flickerWindow = static_cast<WindowImpl*>(window);

		int32_t chromeLead = 64;
		int32_t flickerZoneSize = 128;

		const Rect windowRect = window->GetSurfaceRect();

		Vec2i topLeft = m_flickerWindow->GetPosition();
		Vec2i dimensions = Vec2i(windowRect.Width(), windowRect.Height());

		m_flickerAxis = Vec2i(-1, -1);
		m_flickerZoneSize = 128;
		m_flickerBasisCoordinate = topLeft + dimensions;
		m_flickerBasisCoordinateDistance = flickerZoneSize;
		m_flickerChromeDistanceOffset = chromeLead;

		int32_t axisLength = -(m_flickerAxis.m_x * dimensions.m_x + m_flickerAxis.m_y * dimensions.m_y);

		for (int32_t i = 0; i < axisLength + chromeLead + flickerZoneSize; i += velocity)
		{
			m_flickerBasisCoordinateDistance -= velocity;
			PLSysCalls::Sleep(1);
		}

		m_flickerWindow = nullptr;
	}


	void WindowManagerImpl::SetWindowDesaturation(Window *window, float desaturationLevel)
	{
		static_cast<WindowImpl*>(window)->GetEffects().m_desaturationLevel = desaturationLevel;
	}

	void WindowManagerImpl::SetResizeInProgress(Window *window, const PortabilityLayer::Vec2i &size)
	{
		ResolveCachingColor blackColor = StdColors::Black();
		ResolveCachingColor whiteColor = StdColors::White();

		ResetResizeInProgressSurfaces();

		m_isResizeInProgress = true;
		if (!m_resizeInProgressHorizontalBar.Init(Rect::Create(0, 0, 3, size.m_x + 4), GpPixelFormats::k8BitStandard))
		{
			m_resizeInProgressHorizontalBar.FillRect(Rect::Create(0, 0, 3, size.m_x + 4), blackColor);
			m_resizeInProgressHorizontalBar.FillRect(Rect::Create(1, 1, 2, size.m_x + 3), whiteColor);
		}
		else
		{
			m_isResizeInProgress = false;
			return;
		}

		if (!m_resizeInProgressVerticalBar.Init(Rect::Create(0, 0, size.m_y, 3), GpPixelFormats::k8BitStandard))
		{
			m_resizeInProgressVerticalBar.FillRect(Rect::Create(0, 0, size.m_y, 1), blackColor);
			m_resizeInProgressVerticalBar.FillRect(Rect::Create(0, 2, size.m_y, 3), blackColor);
			m_resizeInProgressVerticalBar.FillRect(Rect::Create(0, 1, size.m_y, 2), whiteColor);
		}
		else
		{
			m_isResizeInProgress = false;
			return;
		}

		const PortabilityLayer::Vec2i topLeft = window->GetPosition();
		m_resizeInProgressRect = PortabilityLayer::Rect2i(topLeft, topLeft + size);
	}

	void WindowManagerImpl::ClearResizeInProgress()
	{
		ResetResizeInProgressSurfaces();
		m_isResizeInProgress = false;
	}

	void WindowManagerImpl::RenderFrame(IGpDisplayDriver *displayDriver)
	{
		PortabilityLayer::DisplayDeviceManager *dd = PortabilityLayer::DisplayDeviceManager::GetInstance();

		dd->SyncPalette(displayDriver);

		WindowImpl *window = m_windowStackBottom;
		while (window)
		{
			RenderWindow(window, displayDriver);
			window = window->GetWindowAbove();
		}

		if (m_isResizeInProgress)
		{
			m_resizeInProgressHorizontalBar.PushToDDSurface(displayDriver);
			m_resizeInProgressVerticalBar.PushToDDSurface(displayDriver);

			displayDriver->DrawSurface(m_resizeInProgressHorizontalBar.m_ddSurface, m_resizeInProgressRect.m_topLeft.m_x - 2, m_resizeInProgressRect.m_topLeft.m_y - 2, m_resizeInProgressRect.Right() - m_resizeInProgressRect.Left() + 4, 3, nullptr);
			displayDriver->DrawSurface(m_resizeInProgressHorizontalBar.m_ddSurface, m_resizeInProgressRect.m_topLeft.m_x - 2, m_resizeInProgressRect.m_bottomRight.m_y - 1, m_resizeInProgressRect.Right() - m_resizeInProgressRect.Left() + 4, 3, nullptr);
			displayDriver->DrawSurface(m_resizeInProgressVerticalBar.m_ddSurface, m_resizeInProgressRect.m_topLeft.m_x - 2, m_resizeInProgressRect.m_topLeft.m_y, 3, m_resizeInProgressRect.Bottom() - m_resizeInProgressRect.Top(), nullptr);
			displayDriver->DrawSurface(m_resizeInProgressVerticalBar.m_ddSurface, m_resizeInProgressRect.m_bottomRight.m_x - 1, m_resizeInProgressRect.m_topLeft.m_y, 3, m_resizeInProgressRect.Bottom() - m_resizeInProgressRect.Top(), nullptr);
		}
	}

	void WindowManagerImpl::HandleScreenResolutionChange(uint32_t prevWidth, uint32_t prevHeight, uint32_t newWidth, uint32_t newHeight)
	{
		IGpLogDriver *logger = PLDrivers::GetLogDriver();

		if (logger)
			logger->Printf(IGpLogDriver::Category_Information, "WindowManagerImpl: Resizing from %ix%i to %ix%i", static_cast<int>(prevWidth), static_cast<int>(prevHeight), static_cast<int>(newWidth), static_cast<int>(newHeight));

		const uint32_t menuBarHeight = PortabilityLayer::MenuManager::GetInstance()->GetMenuBarHeight();
		const bool menuIsTouchScreen = PortabilityLayer::MenuManager::GetInstance()->IsMenuTouchScreenStyle();

		for (PortabilityLayer::WindowImpl *window = m_windowStackTop; window != nullptr; window = window->GetWindowBelow())
		{
			uint16_t chromePadding[WindowChromeSides::kCount];
			window->GetChromePadding(chromePadding);

			const Rect surfaceRect = window->GetDrawSurface()->m_port.GetRect();

			if (logger)
				logger->Printf(IGpLogDriver::Category_Information, "Auto window reposition: Pos: %i,%i Size %i,%i", static_cast<int>(window->GetPosition().m_x), static_cast<int>(window->GetPosition().m_y), static_cast<int>(surfaceRect.Width()), static_cast<int>(surfaceRect.Height()));

			uint32_t paddedWidth = surfaceRect.Width() + chromePadding[WindowChromeSides::kLeft] + chromePadding[WindowChromeSides::kRight];
			uint32_t paddedHeight = surfaceRect.Height() + chromePadding[WindowChromeSides::kTop] + chromePadding[WindowChromeSides::kBottom];

			int64_t newX = 0;
			if (newWidth <= paddedWidth || prevWidth <= paddedWidth)
				newX = (static_cast<int64_t>(newWidth) - paddedWidth) / 2;
			else
			{
				uint32_t prevClearanceX = prevWidth - paddedWidth;
				uint32_t newClearanceX = newWidth - paddedWidth;
				newX = static_cast<int64_t>(window->GetPosition().m_x) * static_cast<int64_t>(newClearanceX) / static_cast<int64_t>(prevClearanceX);
			}

			int64_t newY = 0;
			int32_t currentY = window->GetPosition().m_y;
			if (!menuIsTouchScreen)
			{
				if (logger)
					logger->Printf(IGpLogDriver::Category_Information, "currentY: %i menuBarHeight: %i", static_cast<int>(currentY), static_cast<int>(menuBarHeight));

				if (currentY < static_cast<int32_t>(menuBarHeight))
					newY = currentY;
				else
				{
					if (newHeight <= (paddedHeight + menuBarHeight) || prevHeight <= paddedHeight + menuBarHeight)
						newY = (static_cast<int64_t>(newHeight) - paddedHeight - menuBarHeight) / 2 + menuBarHeight;
					else
					{
						uint32_t prevClearanceY = prevHeight - paddedHeight - menuBarHeight;
						uint32_t newClearanceY = newHeight - paddedHeight - menuBarHeight;

						if (logger)
							logger->Printf(IGpLogDriver::Category_Information, "prevClearanceY: %i newClearanceY: %i", static_cast<int>(prevClearanceY), static_cast<int>(newClearanceY));

						newY = (static_cast<int64_t>(currentY) - static_cast<int64_t>(menuBarHeight) - chromePadding[WindowChromeSides::kTop]) * static_cast<int64_t>(newClearanceY) / static_cast<int64_t>(prevClearanceY) + menuBarHeight + chromePadding[WindowChromeSides::kTop];
					}
				}
			}
			else
			{
				uint32_t heightWithoutMenu = std::max<uint32_t>(prevHeight, menuBarHeight) - menuBarHeight;

				if (logger)
					logger->Printf(IGpLogDriver::Category_Information, "currentY: %i paddedHeight: %i heightWithoutMenu: %i", static_cast<int>(currentY), static_cast<int>(paddedHeight), static_cast<int>(heightWithoutMenu));

				if (currentY + static_cast<int32_t>(paddedHeight) >= static_cast<int32_t>(heightWithoutMenu))
					newY = currentY;
				else
				{
					if (newHeight <= (paddedHeight + menuBarHeight) || prevHeight <= paddedHeight + menuBarHeight)
						newY = (static_cast<int64_t>(newHeight) - paddedHeight - menuBarHeight) / 2 + menuBarHeight;
					else
					{
						uint32_t prevClearanceY = prevHeight - paddedHeight - menuBarHeight;
						uint32_t newClearanceY = newHeight - paddedHeight - menuBarHeight;

						if (logger)
							logger->Printf(IGpLogDriver::Category_Information, "prevClearanceY: %i newClearanceY: %i", static_cast<int>(prevClearanceY), static_cast<int>(newClearanceY));

						newY = (static_cast<int64_t>(currentY) - chromePadding[WindowChromeSides::kTop]) * static_cast<int64_t>(newClearanceY) / static_cast<int64_t>(prevClearanceY) + chromePadding[WindowChromeSides::kTop];
					}
				}
			}

			newX = std::max<int64_t>(0, std::min<int64_t>(newX, newWidth - 1));
			newY = std::max<int64_t>(0, std::min<int64_t>(newY, newHeight - 1));

			if (logger)
				logger->Printf(IGpLogDriver::Category_Information, "New position: %i,%i", static_cast<int>(newX), static_cast<int>(newY));

			window->SetPosition(Vec2i(newX, newY));
		}

		m_displayResolution = Vec2i(newWidth, newHeight);
	}

	Vec2i WindowManagerImpl::GetDisplayResolution() const
	{
		return m_displayResolution;
	}

	void WindowManagerImpl::SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b)
	{
		PLDrivers::GetDisplayDriver()->SetBackgroundColor(r, g, b, 255);
	}

	void WindowManagerImpl::ResizeWindow(Window *window, int width, int height)
	{
		static_cast<WindowImpl*>(window)->Resize(width, height);
	}

	void WindowManagerImpl::MoveWindow(Window *window, int x, int y)
	{
		window->SetPosition(Vec2i(x, y));
	}

	void WindowManagerImpl::DetachWindow(Window *window)
	{
		if (m_windowStackBottom == window)
			m_windowStackBottom = m_windowStackBottom->GetWindowAbove();
		if (m_windowStackTop == window)
			m_windowStackTop = m_windowStackTop->GetWindowBelow();

		WindowImpl *impl = static_cast<WindowImpl*>(window);

		if (WindowImpl *below = impl->GetWindowBelow())
			below->SetWindowAbove(impl->GetWindowAbove());

		if (WindowImpl *above = impl->GetWindowAbove())
			above->SetWindowBelow(impl->GetWindowBelow());

		impl->SetWindowAbove(nullptr);
		impl->SetWindowBelow(nullptr);
	}

	void WindowManagerImpl::ResetResizeInProgressSurfaces()
	{
		m_resizeInProgressHorizontalBar.~DrawSurface();
		m_resizeInProgressVerticalBar.~DrawSurface();

		new (&m_resizeInProgressHorizontalBar) DrawSurface();
		new (&m_resizeInProgressVerticalBar) DrawSurface();
	}

	void WindowManagerImpl::ComputeFlickerEffects(const Vec2i &windowUpperLeft, int32_t offset, GpDisplayDriverSurfaceEffects &effects) const
	{
		int32_t upperLeftCoord = windowUpperLeft.m_x * m_flickerAxis.m_x + windowUpperLeft.m_y * m_flickerAxis.m_y;
		int32_t baselineDist = m_flickerBasisCoordinate.m_x * m_flickerAxis.m_x + m_flickerBasisCoordinate.m_y * m_flickerAxis.m_y;
		int32_t distanceFromBaseline = upperLeftCoord - baselineDist;

		int32_t startThreshold = -(m_flickerBasisCoordinateDistance + offset + distanceFromBaseline);
		int32_t endThreshold = startThreshold + m_flickerZoneSize;

		effects.m_flicker = true;
		effects.m_flickerAxisX = m_flickerAxis.m_x;
		effects.m_flickerAxisY = m_flickerAxis.m_y;
		effects.m_flickerStartThreshold = startThreshold;
		effects.m_flickerEndThreshold = endThreshold;
	}

	Window *WindowManagerImpl::GetPutInFrontSentinel() const
	{
		return reinterpret_cast<Window*>(&ms_putInFrontSentinel);
	}

	void WindowManagerImpl::RenderWindow(WindowImpl *window, IGpDisplayDriver *displayDriver)
	{
		if (!window->IsVisible())
			return;

		GpDisplayDriverSurfaceEffects effects;

		if (m_exclusiveWindow != nullptr && m_exclusiveWindow != window)
			effects.m_darken = true;

		effects.m_desaturation = window->GetEffects().m_desaturationLevel;

		bool hasFlicker = (m_flickerWindow == window);

		DrawSurface &graf = *window->GetDrawSurface();

		graf.PushToDDSurface(displayDriver);

		const PixMap *pixMap = *graf.m_port.GetPixMap();
		const uint16_t width = pixMap->m_rect.Width();
		const uint16_t height = pixMap->m_rect.Height();

		const Vec2i windowPos = window->GetPosition();

		if (hasFlicker)
			ComputeFlickerEffects(windowPos, 0, effects);

		displayDriver->DrawSurface(graf.m_ddSurface, windowPos.m_x, windowPos.m_y, width, height, &effects);

		if (!window->IsBorderless())
		{
			uint16_t chromePadding[WindowChromeSides::kCount];
			window->GetChromePadding(chromePadding);

			Vec2i chromeOrigins[WindowChromeSides::kCount];
			chromeOrigins[WindowChromeSides::kTop] = Vec2i(windowPos.m_x - chromePadding[WindowChromeSides::kLeft], windowPos.m_y - chromePadding[WindowChromeSides::kTop]);
			chromeOrigins[WindowChromeSides::kLeft] = Vec2i(windowPos.m_x - chromePadding[WindowChromeSides::kLeft], windowPos.m_y);
			chromeOrigins[WindowChromeSides::kRight] = Vec2i(windowPos.m_x + width, windowPos.m_y);
			chromeOrigins[WindowChromeSides::kBottom] = Vec2i(windowPos.m_x - chromePadding[WindowChromeSides::kLeft], windowPos.m_y + height);

			Vec2i chromeDimensions[WindowChromeSides::kCount];
			chromeDimensions[WindowChromeSides::kTop] = Vec2i(chromePadding[WindowChromeSides::kLeft] + chromePadding[WindowChromeSides::kRight] + width, chromePadding[WindowChromeSides::kTop]);
			chromeDimensions[WindowChromeSides::kLeft] = Vec2i(chromePadding[WindowChromeSides::kLeft], height);
			chromeDimensions[WindowChromeSides::kRight] = Vec2i(chromePadding[WindowChromeSides::kRight], height);
			chromeDimensions[WindowChromeSides::kBottom] = Vec2i(chromePadding[WindowChromeSides::kLeft] + chromePadding[WindowChromeSides::kRight] + width, chromePadding[WindowChromeSides::kBottom]);

			for (int i = 0; i < WindowChromeSides::kCount; i++)
			{
				DrawSurface *chromeSurface = window->GetChromeSurface(static_cast<WindowChromeSide_t>(i));

				chromeSurface->PushToDDSurface(displayDriver);

				if (hasFlicker)
					ComputeFlickerEffects(Vec2i(chromeOrigins[i].m_x, chromeOrigins[i].m_y), m_flickerChromeDistanceOffset, effects);

				displayDriver->DrawSurface(chromeSurface->m_ddSurface, chromeOrigins[i].m_x, chromeOrigins[i].m_y, chromeDimensions[i].m_x, chromeDimensions[i].m_y, &effects);
			}
		}
	}

	WindowManagerImpl *WindowManagerImpl::GetInstance()
	{
		return &ms_instance;
	}

	WindowManagerImpl WindowManagerImpl::ms_instance;
	uint8_t WindowManagerImpl::ms_putInFrontSentinel;

	WindowManager *WindowManager::GetInstance()
	{
		return WindowManagerImpl::GetInstance();
	}

#if GP_ASYNCIFY_PARANOID
	void WindowManager::FlickerWindowIn(Window *window, int32_t velocity)
	{
		static_cast<WindowManagerImpl*>(this)->FlickerWindowIn(window, velocity);
	}

	void WindowManager::FlickerWindowOut(Window *window, int32_t velocity)
	{
		static_cast<WindowManagerImpl*>(this)->FlickerWindowOut(window, velocity);
	}
#endif
}
