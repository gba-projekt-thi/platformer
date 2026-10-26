#pragma once

#include "frame_time.h"

// -----------------------------------------------------------------------------
// TextBuffer
//
// Fixed-size text for menus and banners: appends text, numbers and frame
// times, always NUL-terminated, and cuts off at the capacity instead of
// writing past it. N includes the terminator. No Butano dependency, so it is
// unit-tested on the host (tests/host/test_text_buffer.cpp).
// -----------------------------------------------------------------------------
template <int N>
class TextBuffer {
    static_assert(N >= 1, "TextBuffer needs room for the terminator");

   public:
    auto append(char c) -> TextBuffer& {
        if (_size < N - 1) {
            _text[_size++] = c;
            _text[_size] = '\0';
        }
        return *this;
    }

    auto append(const char* text) -> TextBuffer& {
        for (const char* p = text; p != nullptr && *p != '\0'; ++p) {
            append(*p);
        }
        return *this;
    }

    // Decimal digits of `value`, padded with leading zeros to `width`.
    auto append_uint(unsigned value, int width = 1) -> TextBuffer& {
        char digits[10];
        int count = 0;
        do {
            digits[count++] = char('0' + value % 10);
            value /= 10;
        } while (value > 0);
        for (int i = count; i < width; ++i) {
            append('0');
        }
        while (count > 0) {
            append(digits[--count]);
        }
        return *this;
    }

    // "MM:SS.cc"; minutes wrap at 100 so the layout never shifts.
    auto append_time(const FrameTime& time) -> TextBuffer& {
        return append_uint(time.minutes % 100u, 2)
            .append(':')
            .append_uint(time.seconds, 2)
            .append('.')
            .append_uint(time.centis, 2);
    }

    void clear() {
        _size = 0;
        _text[0] = '\0';
    }

    [[nodiscard]] auto c_str() const -> const char* { return _text; }
    [[nodiscard]] auto size() const -> int { return _size; }

   private:
    char _text[N] = {};
    int _size = 0;
};
