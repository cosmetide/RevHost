#include "stdafx.h"
#include "BufferedImage.h"
#include "UIFontData.h"
#include "UIUnicodeBitmapFont.h"

UIUnicodeBitmapFont::UIUnicodeBitmapFont(const string &fontname, SFontData &referenceFontData)
	: UIAbstractBitmapFont(fontname)
{
	m_numGlyphs = 65536;
	m_referenceFontData = &referenceFontData;
	memset(m_glyphPages, 0, sizeof(m_glyphPages));
	memset(m_unicodeWidth, 0, sizeof(m_unicodeWidth));

	FILE *f = nullptr;
	fopen_s(&f, "resources/Common/res/1_2_2/font/glyph_sizes.bin", "rb");
	if (f)
	{
		fread(m_unicodeWidth, 1, 65536, f);
		fclose(f);
	}
}

UIUnicodeBitmapFont::~UIUnicodeBitmapFont()
{
	for (int i = 0; i < 256; i++)
		delete[] m_glyphPages[i];
}

void UIUnicodeBitmapFont::loadGlyphPage(int page)
{
	wchar_t fileName[64];
	swprintf(fileName, 64, L"/1_2_2/font/glyph_%02X.png", page);
	BufferedImage bimg(fileName);
	int *rawData = bimg.getData();
	if (!rawData) return;

	int size = 256 * 256;
	m_glyphPages[page] = new unsigned char[size];
	for (int i = 0; i < size; i++)
		m_glyphPages[page][i] = (rawData[i] & 0xFF000000) >> 24;
}

IggyFontMetrics *UIUnicodeBitmapFont::GetFontMetrics(IggyFontMetrics *metrics)
{
	metrics->ascent = m_referenceFontData->m_fAscent;
	metrics->descent = m_referenceFontData->m_fDescent;
	metrics->average_glyph_width_for_tab_stops = 8.0f;
	metrics->largest_glyph_bbox_y1 = metrics->descent;
	return metrics;
}

S32 UIUnicodeBitmapFont::GetCodepointGlyph(U32 codepoint)
{
	if (codepoint < 65536 && m_unicodeWidth[codepoint] != 0)
		return (S32)codepoint;
	return IGGY_GLYPH_INVALID;
}

IggyGlyphMetrics *UIUnicodeBitmapFont::GetGlyphMetrics(S32 glyph, IggyGlyphMetrics *metrics)
{
	if (glyph < 0 || glyph >= 65536) { metrics->x0 = metrics->x1 = metrics->advance = metrics->y0 = metrics->y1 = 0; return metrics; }
	int left = m_unicodeWidth[glyph] >> 4;
	int right = (m_unicodeWidth[glyph] & 0xF) + 1;
	float pixelWidth = (right - left) / 2.0f + 1.0f;
	float advance = pixelWidth * m_referenceFontData->m_fAdvPerPixel;

	metrics->x0 = 0.0f;
	metrics->x1 = advance;
	metrics->advance = advance;
	metrics->y0 = 0.0f;
	metrics->y1 = 1.0f;
	return metrics;
}

rrbool UIUnicodeBitmapFont::IsGlyphEmpty(S32 glyph)
{
	if (glyph < 0 || glyph >= 65536) return true;
	return m_unicodeWidth[glyph] == 0;
}

F32 UIUnicodeBitmapFont::GetKerningForGlyphPair(S32 first_glyph, S32 second_glyph)
{
	return 0.0f;
}

rrbool UIUnicodeBitmapFont::CanProvideBitmap(S32 glyph, F32 pixel_scale)
{
	return glyph >= 0 && glyph < 65536;
}

rrbool UIUnicodeBitmapFont::GetGlyphBitmap(S32 glyph, F32 pixel_scale, IggyBitmapCharacter *bitmap)
{
	if (glyph < 0 || glyph >= 65536) return false;
	int page = glyph / 256;
	if (!m_glyphPages[page])
	{
		loadGlyphPage(page);
		if (!m_glyphPages[page]) return false;
	}

	int cx = (glyph % 16) * 16;
	int cy = ((glyph & 0xFF) / 16) * 16;

	bitmap->pixels_one_per_byte = m_glyphPages[page] + (cy * 256) + cx;
	bitmap->width_in_pixels = 16;
	bitmap->height_in_pixels = 16;
	bitmap->stride_in_bytes = 256;

	bitmap->top_left_x = 0;
	bitmap->top_left_y = -static_cast<S32>(16) * m_referenceFontData->m_fAscent;

	bitmap->oversample = 0;

	// Scale parameters: match UIBitmapFont's approach.
	// truePixelScale = the pixel_scale at which 1 glyph pixel = 1 screen pixel.
	// For 16px glyphs displayed at the same visual size as Mojangles_7 (8px glyphs with advPerPixel 1/10):
	// The reference truePixelScale for Mojangles_7 is 1.0f/m_fAdvPerPixel = 10.0f
	// Since our glyphs are 16px (2x the Mojangles 8px), our truePixelScale is 20.0f
	float truePixelScale = 2.0f / m_referenceFontData->m_fAdvPerPixel;

#ifdef _WINDOWS64
	bitmap->pixel_scale_correct = truePixelScale;
	if (pixel_scale < truePixelScale)
	{
		bitmap->pixel_scale_min = 0.0f;
		bitmap->pixel_scale_max = truePixelScale;
		bitmap->point_sample = false;
	}
	else
	{
		bitmap->pixel_scale_min = truePixelScale;
		bitmap->pixel_scale_max = 99.0f;
		bitmap->point_sample = true;
	}
#else
	float glyphScale = 1.0f;
	while ((0.5f + glyphScale) * truePixelScale < pixel_scale)
		glyphScale++;

	if (glyphScale <= 1 && pixel_scale < truePixelScale)
	{
		bitmap->pixel_scale_correct = truePixelScale;
		bitmap->pixel_scale_min = 0.0f;
		bitmap->pixel_scale_max = truePixelScale * 1.001f;
		bitmap->point_sample = false;
	}
	else
	{
		float actualScale = pixel_scale / glyphScale;
		bitmap->pixel_scale_correct = actualScale;
		bitmap->pixel_scale_min = truePixelScale;
		bitmap->pixel_scale_max = 99.0f;
		bitmap->point_sample = true;
	}
#endif

	bitmap->user_context_for_free = nullptr;
	return true;
}

void UIUnicodeBitmapFont::FreeGlyphBitmap(S32 glyph, F32 pixel_scale, IggyBitmapCharacter *bitmap)
{
	// Pixel data lives in m_glyphPages -- nothing to free.
}
