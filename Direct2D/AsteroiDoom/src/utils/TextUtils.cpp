#include "TextUtils.h"

#include <stdexcept>

IDWriteFactory * TextHelper::write_factory = nullptr;

TextHelper::TextHelper() {
	if (!write_factory) {
		if (DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown **)(&write_factory)) !=
		    S_OK) {
			throw std::runtime_error("Failed to initialize DWrite factory.");
		}
	}
}

TextHelper::TextHelper(
    float size,
    const wchar_t * fontFamily,
    D2D1_RECT_F rect,
    ID2D1HwndRenderTarget * target,
    DWRITE_FONT_WEIGHT weight,
    DWRITE_FONT_STYLE style,
    DWRITE_FONT_STRETCH stretch
) :
    rect(rect),
    target(target) {
	if (!write_factory) {
		if (DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown **)(&write_factory)) !=
		    S_OK) {
			throw std::runtime_error("Failed to initialize DWrite factory.");
		}
	}

	if (write_factory->CreateTextFormat(fontFamily, nullptr, weight, style, stretch, size, L"en-us", &text_format) !=
	    S_OK) {
		throw std::runtime_error("Failed to create text format.");
	}

	reloadBrush(target);
}

void TextHelper::reloadBrush(ID2D1HwndRenderTarget * newTarget) {
	target = newTarget;
	if (target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush) != S_OK) {
		throw std::runtime_error("Failed to create solid colour brush.");
	}
}

void TextHelper::draw(const wchar_t * text, unsigned int length, D2D1_COLOR_F colour) {
	D2D1_COLOR_F oldColour = brush->GetColor();
	brush->SetColor(colour);
	target->DrawTextW(text, length, text_format, rect, brush);
	brush->SetColor(oldColour);
}
