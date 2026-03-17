#pragma once

#include <memory>
#include <windows.h>
#include <gdiplus.h>
#include <string>

class Screenshot {
protected:
	RECT m_captureRect;
	std::shared_ptr<Gdiplus::Bitmap> m_image = nullptr;
	HWND m_window = nullptr;

	RECT createRect();
	void encoderClsid();
public:
	Screenshot();
	Screenshot(HWND window);
	void capture(HWND window);
	void save(const std::wstring& path);
	bool isCaptured();
	std::shared_ptr<Gdiplus::Bitmap> getBitmap() const;
	RECT getCaptureRect() const;
};