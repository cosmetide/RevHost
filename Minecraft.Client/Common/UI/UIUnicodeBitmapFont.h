#pragma once
#include "UIBitmapFont.h"

struct SFontData;

class UIUnicodeBitmapFont : public UIAbstractBitmapFont
{
private:
	unsigned char m_unicodeWidth[65536];
	unsigned char* m_glyphPages[256];
	SFontData* m_referenceFontData;

	void loadGlyphPage(int page);

public:
	UIUnicodeBitmapFont(const string &fontname, SFontData &referenceFontData);
	~UIUnicodeBitmapFont();

	virtual IggyFontMetrics *GetFontMetrics(IggyFontMetrics *metrics);
	virtual S32 GetCodepointGlyph(U32 codepoint);
	virtual IggyGlyphMetrics *GetGlyphMetrics(S32 glyph, IggyGlyphMetrics *metrics);
	virtual rrbool IsGlyphEmpty(S32 glyph);
	virtual F32 GetKerningForGlyphPair(S32 first_glyph, S32 second_glyph);
	virtual rrbool CanProvideBitmap(S32 glyph, F32 pixel_scale);
	virtual rrbool GetGlyphBitmap(S32 glyph, F32 pixel_scale, IggyBitmapCharacter *bitmap);
	virtual void FreeGlyphBitmap(S32 glyph, F32 pixel_scale, IggyBitmapCharacter *bitmap);
};
