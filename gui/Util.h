// Util.h — вспомогательные функции интерфейса.
#pragma once
#include <wx/string.h>

#include <string>

/// UTF-8 строка (литерал или std::string) -> wxString.
inline wxString U8(const char* s) { return wxString::FromUTF8(s); }
inline wxString U8(const std::string& s) { return wxString::FromUTF8(s.c_str()); }

#include <wx/image.h>

#include <cstdlib>
#include <cstring>

#include "Image2D.h"

/// Image2D (RGB) -> wxImage. wxImage берёт буфер во владение, поэтому память выделяется malloc.
inline wxImage toWxImage(const fractal::Image2D& img) {
    if (img.width() <= 0 || img.height() <= 0) return wxImage();
    auto* buf = static_cast<unsigned char*>(std::malloc(img.byteSize()));
    std::memcpy(buf, img.data(), img.byteSize());
    return wxImage(img.width(), img.height(), buf, false);
}
