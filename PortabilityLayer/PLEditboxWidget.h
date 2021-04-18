#pragma once

#include "FontPresets.h"
#include "PascalStr.h"
#include "PLWidgets.h"
#include "Vec2i.h"

struct DrawSurface;

namespace PortabilityLayer
{
	class EditboxWidget final : public WidgetSpec<EditboxWidget, WidgetTypes::kEditbox>
	{
	public:
		typedef bool (*CharacterFilterCallback_t)(void *context, uint8_t character);

		EditboxWidget(const WidgetBasicState &state);
		~EditboxWidget();

		bool Init(const WidgetBasicState &state, const void *additionalData) override;

		void DrawControl(DrawSurface *surface) override;
		void SetString(const PLPasStr &str) override;
		PLPasStr GetString() const override;

		void GainFocus() override;
		void LoseFocus() override;

		WidgetHandleState_t ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt) GP_ASYNCIFY_PARANOID_OVERRIDE;
		int16_t Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback) GP_ASYNCIFY_PARANOID_OVERRIDE;

		Rect GetExpandedRect() const override;

		bool HandlesTickEvents() const override;

		void SetSelection(size_t startChar, size_t endChar);

		void SetMultiLine(bool isMultiLine);

		void SetCharacterFilter(void *context, CharacterFilterCallback_t callback);
		void SetCapacity(size_t capacity);

	private:
		static const unsigned int kCaratBlinkRate = 20;
		static const unsigned int kMouseScrollRate = 20;

		enum SpanScanDirection
		{
			SpanScanDirection_Left,
			SpanScanDirection_Right,
		};

		enum CaratSelectionAnchor
		{
			CaratSelectionAnchor_Start,
			CaratSelectionAnchor_End
		};

		enum CharacterCategory
		{
			CharacterCategory_AlphaNumeric,
			CharacterCategory_Whitespace,
			CharacterCategory_LineBreak,
			CharacterCategory_Punctuation,
		};

		struct CharacterCategorySpan
		{
			uint8_t m_lastCharacterPosInclusive;
			CharacterCategory m_category;
		};

		enum CaratCharacterAlignment
		{
			CaratCharacterAlignment_Start,		// Before the start of the text
			CaratCharacterAlignment_AfterChar,	// Carat is after the character that was clicked
			CaratCharacterAlignment_BeforeChar,	// Carat is before the character that was clicked
			CaratCharacterAlignment_EndOfLine,	// Carat is at the end of a line
		};

		void OnTick() override;
		void Redraw();

		void HandleCharacter(uint8_t keyChar, const uint32_t numRepeatsRequested);
		void HandleBackspace(const uint32_t numRepeatsRequested);
		void HandleForwardDelete(const uint32_t numRepeatsRequested);

		void HandleUpArrow(const uint32_t numRepeatsRequested, bool shiftHeld);
		void HandleDownArrow(const uint32_t numRepeatsRequested, bool shiftHeld);
		void HandleLeftArrow(const uint32_t numRepeatsRequested, bool shiftHeld, bool wholeWords);
		void HandleRightArrow(const uint32_t numRepeatsRequested, bool shiftHeld, bool wholeWords);

		void HandleHome(bool shiftHeld);
		void HandleEnd(bool shiftHeld);
		void HandleCopy();
		void HandleCut();
		void HandlePaste(size_t repeatCount);

		size_t FindVerticalMovementCaratPos(const Vec2i &desiredPos, bool &isOutOfRange, CaratCharacterAlignment *optOutAlignment) const;
		void ExpandSelectionToWords(size_t caratPos, size_t &outStartChar, size_t &outEndChar);
		void HandleKeyMoveCarat(size_t newPos, bool shiftHeld);

		WidgetHandleState_t HandleDragSelection(const TimeTaggedVOSEvent &evt);

		void DrawSelection(DrawSurface *surface, const Vec2i &basePoint, PortabilityLayer::RenderedFont *font) const;

		Vec2i ResolveCaratPos(PortabilityLayer::RenderedFont *rfont) const;
		Vec2i ResolveBasePoint() const;
		size_t ResolveCaratChar() const;
		void AdjustScrollToCarat();
		void AdjustScrollToTextBounds();
		size_t IdentifySpanLength(size_t startChar, SpanScanDirection scanDirection) const;
		static CharacterCategory CategorizeCharacter(uint8_t character);

		PortabilityLayer::FontPreset_t GetFontPreset() const;
		PortabilityLayer::RenderedFont *GetRenderedFont() const;

		uint8_t *m_chars;
		size_t m_capacity;
		size_t m_length;
		size_t m_selStartChar;
		size_t m_selEndChar;
		CaratSelectionAnchor m_caratSelectionAnchor;	// Where the carat is attached to the selection range

		Vec2i m_caratScrollPosition;	// Ideal position of the carat in the editbox, but not necessarily its actual location (i.e. may be in the middle of a glyph)
		bool m_caratScrollLocked;		// If true, the vertical position

		Vec2i m_scrollOffset;

		bool m_hasFocus;
		bool m_isMultiLine;
		bool m_isDraggingSelection;
		bool m_isDraggingWords;
		size_t m_dragSelectionStartChar;
		size_t m_dragSelectionEndChar;
		uint32_t m_doubleClickTime;
		Point m_doubleClickPoint;

		uint16_t m_caratTimer;

		CharacterFilterCallback_t m_characterFilter;
		void *m_characterFilterContext;

		static const CharacterCategorySpan gs_characterCategorySpans[];

		static const int kCopyShortcutKey = 'C';
		static const int kCutShortcutKey = 'X';
		static const int kPasteShortcutKey = 'V';
		static const int kSelectAllShortcutKey = 'A';
	};
}
