#pragma once
#include <string>

// Shape Arabic text for visual display. Returns the visually-ordered string.
std::wstring shapeArabicText(const std::wstring& input);

// Same as above, but also maps a logical cursor position to its visual position
// in the returned string. Pass the logical cursor index; visualCursorPos receives
// the index into the returned string where the cursor should be drawn.
std::wstring shapeArabicText(const std::wstring& input, int logicalCursorPos, int* visualCursorPos);
