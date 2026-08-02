#include "stdafx.h"
#include "ArabicShaping.h"
#include <vector>
#include <unordered_map>

// Arabic text shaping - contextual form selection and RTL visual reordering.
// Ported from "Arabic Writer" JS reference by Omar Muhammad (GPL).

// Each entry: base char, isolated, initial, medial, final forms
struct ArabicCharEntry
{
	wchar_t base;
	wchar_t isolated;
	wchar_t initial;
	wchar_t medial;
	wchar_t final_;
	bool connectsLeft;
	bool connectsRight;
};

// Core Arabic + extended (Farsi/Urdu) characters with presentation forms
static const ArabicCharEntry arabicChars[] =
{
	// base    isolated  initial   medial    final     cL     cR
	{ 0x0621, 0xFE80, 0xFE80, 0xFE80, 0xFE80, false, false }, // hamza
	{ 0x0622, 0xFE81, 0xFE81, 0xFE82, 0xFE82, false, true  }, // alef madda
	{ 0x0623, 0xFE83, 0xFE83, 0xFE84, 0xFE84, false, true  }, // alef hamza above
	{ 0x0624, 0xFE85, 0xFE85, 0xFE86, 0xFE86, false, true  }, // waw hamza
	{ 0x0625, 0xFE87, 0xFE87, 0xFE88, 0xFE88, false, true  }, // alef hamza below
	{ 0x0626, 0xFE89, 0xFE8B, 0xFE8C, 0xFE8A, true,  true  }, // yeh hamza
	{ 0x0627, 0xFE8D, 0xFE8D, 0xFE8E, 0xFE8E, false, true  }, // alef
	{ 0x0628, 0xFE8F, 0xFE91, 0xFE92, 0xFE90, true,  true  }, // beh
	{ 0x0629, 0xFE93, 0xFE93, 0xFE94, 0xFE94, false, true  }, // teh marbuta
	{ 0x062A, 0xFE95, 0xFE97, 0xFE98, 0xFE96, true,  true  }, // teh
	{ 0x062B, 0xFE99, 0xFE9B, 0xFE9C, 0xFE9A, true,  true  }, // theh
	{ 0x062C, 0xFE9D, 0xFE9F, 0xFEA0, 0xFE9E, true,  true  }, // jeem
	{ 0x062D, 0xFEA1, 0xFEA3, 0xFEA4, 0xFEA2, true,  true  }, // hah
	{ 0x062E, 0xFEA5, 0xFEA7, 0xFEA8, 0xFEA6, true,  true  }, // khah
	{ 0x062F, 0xFEA9, 0xFEA9, 0xFEAA, 0xFEAA, false, true  }, // dal
	{ 0x0630, 0xFEAB, 0xFEAB, 0xFEAC, 0xFEAC, false, true  }, // thal
	{ 0x0631, 0xFEAD, 0xFEAD, 0xFEAE, 0xFEAE, false, true  }, // reh
	{ 0x0632, 0xFEAF, 0xFEAF, 0xFEB0, 0xFEB0, false, true  }, // zain
	{ 0x0633, 0xFEB1, 0xFEB3, 0xFEB4, 0xFEB2, true,  true  }, // seen
	{ 0x0634, 0xFEB5, 0xFEB7, 0xFEB8, 0xFEB6, true,  true  }, // sheen
	{ 0x0635, 0xFEB9, 0xFEBB, 0xFEBC, 0xFEBA, true,  true  }, // sad
	{ 0x0636, 0xFEBD, 0xFEBF, 0xFEC0, 0xFEBE, true,  true  }, // dad
	{ 0x0637, 0xFEC1, 0xFEC3, 0xFEC4, 0xFEC2, true,  true  }, // tah
	{ 0x0638, 0xFEC5, 0xFEC7, 0xFEC8, 0xFEC6, true,  true  }, // zah
	{ 0x0639, 0xFEC9, 0xFECB, 0xFECC, 0xFECA, true,  true  }, // ain
	{ 0x063A, 0xFECD, 0xFECF, 0xFED0, 0xFECE, true,  true  }, // ghain
	{ 0x0640, 0x0640, 0x0640, 0x0640, 0x0640, true,  true  }, // tatweel
	{ 0x0641, 0xFED1, 0xFED3, 0xFED4, 0xFED2, true,  true  }, // feh
	{ 0x0642, 0xFED5, 0xFED7, 0xFED8, 0xFED6, true,  true  }, // qaf
	{ 0x0643, 0xFED9, 0xFEDB, 0xFEDC, 0xFEDA, true,  true  }, // kaf
	{ 0x0644, 0xFEDD, 0xFEDF, 0xFEE0, 0xFEDE, true,  true  }, // lam
	{ 0x0645, 0xFEE1, 0xFEE3, 0xFEE4, 0xFEE2, true,  true  }, // meem
	{ 0x0646, 0xFEE5, 0xFEE7, 0xFEE8, 0xFEE6, true,  true  }, // noon
	{ 0x0647, 0xFEE9, 0xFEEB, 0xFEEC, 0xFEEA, true,  true  }, // heh
	{ 0x0648, 0xFEED, 0xFEED, 0xFEEE, 0xFEEE, false, true  }, // waw
	{ 0x0649, 0xFEEF, 0xFEEF, 0xFEF0, 0xFEF0, false, true  }, // alef maksura
	{ 0x064A, 0xFEF1, 0xFEF3, 0xFEF4, 0xFEF2, true,  true  }, // yeh
	// Extended - Farsi/Urdu
	{ 0x067E, 0xFB56, 0xFB58, 0xFB59, 0xFB57, true,  true  }, // peh
	{ 0x0686, 0xFB7A, 0xFB7C, 0xFB7D, 0xFB7B, true,  true  }, // tcheh
	{ 0x0698, 0xFB8A, 0xFB8A, 0xFB8B, 0xFB8B, false, true  }, // jeh
	{ 0x06A9, 0xFB8E, 0xFB90, 0xFB91, 0xFB8F, true,  true  }, // keheh (Farsi kaf)
	{ 0x06AF, 0xFB92, 0xFB94, 0xFB95, 0xFB93, true,  true  }, // gaf
	{ 0x06CC, 0xFBFC, 0xFBFE, 0xFBFF, 0xFBFD, true,  true  }, // Farsi yeh
	// Urdu
	{ 0x0679, 0xFB66, 0xFB68, 0xFB69, 0xFB67, true,  true  }, // tteh
	{ 0x0688, 0xFB88, 0xFB88, 0xFB89, 0xFB89, false, true  }, // ddal
	{ 0x0691, 0xFB8C, 0xFB8C, 0xFB8D, 0xFB8D, false, true  }, // rreh
	{ 0x06C1, 0xFBA6, 0xFBA8, 0xFBA9, 0xFBA7, true,  true  }, // heh goal
	{ 0x06D2, 0xFBAE, 0xFBAE, 0xFBAF, 0xFBAF, false, true  }, // yeh barree
};

static const int ARABIC_CHAR_COUNT = sizeof(arabicChars) / sizeof(arabicChars[0]);

// Laam-Alef ligatures: when lam (0x0644) is followed by certain alef forms
struct LaamAlefEntry
{
	wchar_t alef;       // the alef variant
	wchar_t isolated;   // ligature isolated form
	wchar_t final_;     // ligature final form
};

static const LaamAlefEntry laamAlefTable[] =
{
	{ 0x0622, 0xFEF5, 0xFEF6 }, // lam + alef madda
	{ 0x0623, 0xFEF7, 0xFEF8 }, // lam + alef hamza above
	{ 0x0625, 0xFEF9, 0xFEFA }, // lam + alef hamza below
	{ 0x0627, 0xFEFB, 0xFEFC }, // lam + alef
};

// Build lookup map on first use
static std::unordered_map<wchar_t, const ArabicCharEntry*> charMap;
static bool tablesInitialized = false;

static void initTables()
{
	if (tablesInitialized) return;
	for (int i = 0; i < ARABIC_CHAR_COUNT; i++)
	{
		charMap[arabicChars[i].base] = &arabicChars[i];
	}
	tablesInitialized = true;
}

static bool isArabicChar(wchar_t c)
{
	return (c >= 0x0600 && c <= 0x06FF) || (c >= 0xFB50 && c <= 0xFDFF) || (c >= 0xFE70 && c <= 0xFEFF);
}

static bool isHaraka(wchar_t c)
{
	// Arabic diacritics (tashkeel/harakat): U+0610-U+061A, U+064B-U+065F, U+0670
	return (c >= 0x064B && c <= 0x065F) || (c >= 0x0610 && c <= 0x061A) || c == 0x0670;
}

static bool isLaamAlefLigature(wchar_t c)
{
	// Laam-Alef presentation forms: U+FEF5-U+FEFC
	return c >= 0xFEF5 && c <= 0xFEFC;
}

static bool isDigit(wchar_t c)
{
	// Western digits and Arabic-Indic digits
	return (c >= L'0' && c <= L'9') || (c >= 0x0660 && c <= 0x0669);
}

// Neutral characters that inherit direction from surrounding context
static bool isNeutralChar(wchar_t c)
{
	return c == L' '
		|| c == L'.'  || c == L','  || c == L'!'  || c == L'?'
		|| c == L':'  || c == L';'  || c == L'-'  || c == L'('
		|| c == L')'  || c == L'['  || c == L']'
		|| c == 0x060C  // Arabic comma
		|| c == 0x061B  // Arabic semicolon
		|| c == 0x061F; // Arabic question mark
}

static const ArabicCharEntry* findEntry(wchar_t c)
{
	auto it = charMap.find(c);
	if (it != charMap.end()) return it->second;
	return nullptr;
}

static const LaamAlefEntry* findLaamAlef(wchar_t alef)
{
	for (int i = 0; i < 4; i++)
	{
		if (laamAlefTable[i].alef == alef) return &laamAlefTable[i];
	}
	return nullptr;
}

// -------------------------------------------------------------------------
// Core shaping logic, shared by both public overloads.
//
// logicalToVisual: if non-null, maps input[i] -> position in output string.
//                  Must be pre-sized to input.size()+1.
// -------------------------------------------------------------------------
static std::wstring shapeArabicTextInternal(const std::wstring& input,
                                            std::vector<int>* logicalToVisual)
{
	if (input.empty()) return input;

	initTables();

	// Fast path: check if any base Arabic characters exist
	bool hasArabic = false;
	for (size_t i = 0; i < input.size(); i++)
	{
		if (input[i] >= 0x0600 && input[i] <= 0x06FF)
		{
			hasArabic = true;
			break;
		}
	}
	if (!hasArabic)
	{
		// Identity mapping
		if (logicalToVisual)
		{
			for (size_t i = 0; i <= input.size(); i++)
				(*logicalToVisual)[i] = (int)i;
		}
		return input;
	}

	// -----------------------------------------------------------------------
	// Split into runs: Arabic vs non-Arabic.
	// Track the starting logical index of each run.
	// -----------------------------------------------------------------------
	struct Run
	{
		std::wstring text;
		bool arabic;
		int logicalStart; // index into input[] where this run begins
	};
	std::vector<Run> runs;

	size_t i = 0;
	while (i < input.size())
	{
		bool curArabic = isArabicChar(input[i]) || isHaraka(input[i]);
		Run run;
		run.arabic = curArabic;
		run.logicalStart = (int)i;
		while (i < input.size())
		{
			bool charArabic = isArabicChar(input[i]) || isHaraka(input[i]);
			if (charArabic == curArabic)
			{
				run.text += input[i];
				i++;
			}
			else
			{
				break;
			}
		}
		runs.push_back(run);
	}

	// -----------------------------------------------------------------------
	// Merge neutral runs that sit between two Arabic runs into the preceding
	// Arabic run (with the following Arabic run appended too). This keeps
	// inter-word spaces inside the Arabic run so the whole phrase reverses
	// together, producing correct RTL word order.
	// -----------------------------------------------------------------------
	for (size_t r = 1; r + 1 < runs.size(); r++)
	{
		if (!runs[r].arabic && runs[r - 1].arabic && runs[r + 1].arabic)
		{
			bool allNeutral = true;
			for (wchar_t c : runs[r].text)
			{
				if (!isNeutralChar(c)) { allNeutral = false; break; }
			}
			if (allNeutral)
			{
				// Absorb runs[r] and runs[r+1] into runs[r-1]
				runs[r - 1].text += runs[r].text + runs[r + 1].text;
				runs.erase(runs.begin() + r, runs.begin() + r + 2);
				r--; // re-check from same position
			}
		}
	}

	// Recompute logical starts after merging (run text lengths may have grown)
	{
		int pos = 0;
		for (size_t r = 0; r < runs.size(); r++)
		{
			runs[r].logicalStart = pos;
			pos += (int)runs[r].text.size();
		}
	}

	// -----------------------------------------------------------------------
	// Shape each Arabic run.
	// For each run we also build a posMap: posMap[localLogical] = localVisual
	// (local means within the run's text, before run offsets are added).
	// -----------------------------------------------------------------------
	std::vector<std::vector<int>> runPosMap(runs.size()); // per-run local maps

	for (size_t r = 0; r < runs.size(); r++)
	{
		if (!runs[r].arabic)
		{
			// Non-Arabic: identity mapping
			runPosMap[r].resize(runs[r].text.size() + 1);
			for (size_t j = 0; j <= runs[r].text.size(); j++)
				runPosMap[r][j] = (int)j;
			continue;
		}

		std::wstring& text = runs[r].text;
		const int textLen = (int)text.size();

		// Collect base character indices (skip harakat)
		std::vector<size_t> baseIndices;
		for (size_t j = 0; j < text.size(); j++)
		{
			if (!isHaraka(text[j]))
				baseIndices.push_back(j);
		}

		// ------------------------------------------------------------------
		// Laam-Alef ligatures
		// ------------------------------------------------------------------
		std::vector<bool> consumed(text.size(), false);
		std::vector<std::pair<size_t, wchar_t>> ligatures; // lam idx -> ligature char

		for (size_t bi = 0; bi + 1 < baseIndices.size(); bi++)
		{
			size_t idx     = baseIndices[bi];
			size_t nextIdx = baseIndices[bi + 1];
			if (text[idx] == 0x0644) // lam
			{
				const LaamAlefEntry* la = findLaamAlef(text[nextIdx]);
				if (la)
				{
					bool connectsToPrev = false;
					if (bi > 0)
					{
						size_t prevIdx = baseIndices[bi - 1];
						const ArabicCharEntry* prevEntry = findEntry(text[prevIdx]);
						if (prevEntry && prevEntry->connectsLeft)
							connectsToPrev = true;
					}
					wchar_t ligChar = connectsToPrev ? la->final_ : la->isolated;
					ligatures.push_back({ idx, ligChar });
					consumed[nextIdx] = true;
					bi++; // skip the alef
				}
			}
		}

		// Apply ligature characters
		for (size_t li = 0; li < ligatures.size(); li++)
			text[ligatures[li].first] = ligatures[li].second;

		// Rebuild base indices after ligature consumption
		baseIndices.clear();
		for (size_t j = 0; j < text.size(); j++)
		{
			if (!isHaraka(text[j]) && !consumed[j])
				baseIndices.push_back(j);
		}

		// ------------------------------------------------------------------
		// Contextual form selection
		// ------------------------------------------------------------------
		for (size_t bi = 0; bi < baseIndices.size(); bi++)
		{
			size_t idx = baseIndices[bi];
			const ArabicCharEntry* entry = findEntry(text[idx]);
			if (!entry) continue;

			bool prevConnects = false;
			if (bi > 0)
			{
				size_t prevIdx = baseIndices[bi - 1];
				const ArabicCharEntry* prevEntry = findEntry(text[prevIdx]);
				if (prevEntry && prevEntry->connectsLeft)
					prevConnects = true;
				else if (!prevEntry && text[prevIdx] >= 0xFE70)
					prevConnects = false; // lam-alef ligature doesn't connect left
			}

			bool nextConnects = false;
			if (bi + 1 < baseIndices.size())
			{
				size_t nextIdx = baseIndices[bi + 1];
				const ArabicCharEntry* nextEntry = findEntry(text[nextIdx]);
				if (nextEntry && nextEntry->connectsRight)
					nextConnects = true;
				else if (!nextEntry && isLaamAlefLigature(text[nextIdx]))
					nextConnects = true; // lam-alef ligatures always connect to the right
			}

			bool canConnectPrev = prevConnects && entry->connectsRight;
			bool canConnectNext = nextConnects && entry->connectsLeft;

			if      (canConnectPrev && canConnectNext) text[idx] = entry->medial;
			else if (canConnectPrev)                   text[idx] = entry->final_;
			else if (canConnectNext)                   text[idx] = entry->initial;
			else                                       text[idx] = entry->isolated;
		}

		// ------------------------------------------------------------------
		// Build shaped string (drop consumed ligature partners)
		// and a forward mapping: shapedIdx[logicalPos] = position in shaped
		// (for consumed chars, map to the ligature position)
		// ------------------------------------------------------------------
		std::vector<int> logToShaped(textLen + 1, 0);
		std::wstring shaped;

		// Track where each position of text[] lands in shaped[]
		// consumed chars map to the position of their ligature replacement
		{
			int shapedPos = 0;
			// First record the ligature lam positions so consumed alefs can map there
			// We need to process in order
			for (int j = 0; j < textLen; j++)
			{
				logToShaped[j] = shapedPos;
				if (!consumed[j])
				{
					shaped += text[j];
					shapedPos++;
				}
				// consumed chars: logToShaped[j] stays pointing to the lam position
				// (shapedPos is not incremented, so it already equals lam's slot)
			}
			logToShaped[textLen] = shapedPos; // end-of-run cursor
		}

		// ------------------------------------------------------------------
		// Reverse for RTL visual order, preserving LTR digit sequences.
		// Also build reversedPos[posInShaped] -> posInReversed.
		// ------------------------------------------------------------------
		const int shapedLen = (int)shaped.size();
		std::wstring reversed;
		reversed.reserve(shapedLen);

		// reversedOf[i] = where shaped[i] ended up in reversed[]
		std::vector<int> reversedOf(shapedLen, 0);

		// We'll do a two-pass approach:
		// 1. Walk shaped backwards, collecting digit runs and individual chars
		// 2. For each element we emit, record reversedOf[]

		// Collect output segments: each segment is (isDigitSeq, startInShaped, len)
		struct Seg { int start; int len; bool isDigit; };
		std::vector<Seg> segs;
		{
			int j = shapedLen - 1;
			while (j >= 0)
			{
				if (isDigit(shaped[j]))
				{
					// Find the full digit run (going left from j)
					int end = j;
					while (j >= 0 && isDigit(shaped[j])) j--;
					int start = j + 1;
					segs.push_back({ start, end - start + 1, true });
				}
				else
				{
					segs.push_back({ j, 1, false });
					j--;
				}
			}
		}

		// Emit segments in order, recording reversedOf[]
		{
			int outPos = 0;
			for (size_t s = 0; s < segs.size(); s++)
			{
				const Seg& seg = segs[s];
				if (seg.isDigit)
				{
					// Digit sequence: output in LTR order (start..start+len-1)
					for (int k = seg.start; k < seg.start + seg.len; k++)
					{
						reversedOf[k] = outPos;
						reversed += shaped[k];
						outPos++;
					}
				}
				else
				{
					// Single non-digit char
					reversedOf[seg.start] = outPos;
					reversed += shaped[seg.start];
					outPos++;
				}
			}
		}

		runs[r].text = reversed;

		// ------------------------------------------------------------------
		// Build the local logical->visual position map for this run.
		// Cursor positions are between characters: for RTL text, cursor
		// at shaped position p maps to visual position (shapedLen - p).
		// ------------------------------------------------------------------
		if (logicalToVisual)
		{
			runPosMap[r].resize(textLen + 1);
			for (int j = 0; j <= textLen; j++)
			{
				int sp = logToShaped[j];
				runPosMap[r][j] = shapedLen - sp;
			}
		}
	}

	// -----------------------------------------------------------------------
	// Concatenate runs and compute absolute visual positions
	// -----------------------------------------------------------------------
	std::wstring result;
	result.reserve(input.size());

	// Compute the visual start offset for each run
	std::vector<int> runVisualStart(runs.size(), 0);
	{
		int voff = 0;
		for (size_t r = 0; r < runs.size(); r++)
		{
			runVisualStart[r] = voff;
			voff += (int)runs[r].text.size();
		}
	}

	for (size_t r = 0; r < runs.size(); r++)
		result += runs[r].text;

	// -----------------------------------------------------------------------
	// Fill the caller's logicalToVisual[] array
	// -----------------------------------------------------------------------
	if (logicalToVisual)
	{
		// For each input position, find which run it belongs to and map it
		for (size_t r = 0; r < runs.size(); r++)
		{
			int logStart = runs[r].logicalStart;

			if (r < runPosMap.size() && !runPosMap[r].empty())
			{
				int localLen = (int)runPosMap[r].size() - 1; // number of logical chars
				for (int j = 0; j <= localLen; j++)
				{
					int logIdx = logStart + j;
					if (logIdx <= (int)input.size())
						(*logicalToVisual)[logIdx] = runVisualStart[r] + runPosMap[r][j];
				}
			}
		}

	}

	return result;
}

// -------------------------------------------------------------------------
// Public API
// -------------------------------------------------------------------------

std::wstring shapeArabicText(const std::wstring& input)
{
	return shapeArabicTextInternal(input, nullptr);
}

std::wstring shapeArabicText(const std::wstring& input, int logicalCursorPos, int* visualCursorPos)
{
	std::vector<int> ltv(input.size() + 1, 0);
	std::wstring result = shapeArabicTextInternal(input, &ltv);

	if (visualCursorPos)
	{
		int clamped = logicalCursorPos;
		if (clamped < 0) clamped = 0;
		if (clamped > (int)input.size()) clamped = (int)input.size();
		*visualCursorPos = ltv[clamped];
	}

	return result;
}
