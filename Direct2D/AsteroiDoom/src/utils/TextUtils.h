#pragma once

#include <d2d1.h>
#include <dwrite_3.h>

class TextHelper {
	static IDWriteFactory * write_factory;
	IDWriteTextFormat * text_format{};

	ID2D1HwndRenderTarget * target{};
	ID2D1SolidColorBrush * brush{};

	D2D1_RECT_F rect{};

public:
	TextHelper();

	TextHelper(
	    float size,
	    const wchar_t * fontFamily,
	    D2D1_RECT_F rect,
	    ID2D1HwndRenderTarget * target,
	    DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_BOLD,
	    DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL,
	    DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL
	);

	void reloadBrush(ID2D1HwndRenderTarget * newTarget);

	void draw(
	    const wchar_t * text,
	    unsigned int length,
	    D2D1_COLOR_F colour = D2D1::ColorF(D2D1::ColorF::White)
	);
};
