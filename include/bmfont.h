/** Binary AngelCode BMFont file parser. Based on Matthias Mann's TWL BitmapFont class.
	@file bmfont.h */

#ifndef BMFONT_H
#define BMFONT_H

#ifdef __cplusplus
extern "C" {
#endif

#define LOG2_PAGE_SIZE 9
#define PAGE_SIZE (1 << LOG2_PAGE_SIZE)
#define	PAGES (0x10000 / PAGE_SIZE)

	/** A single character in a front page. */
	struct glyph {
		int id, /**< The character id. */
			x, /**< The left position of the character image in the texture. */
			y, /**< The top position of the character image in the texture. */
			width, /**< The width of the character image in the texture. */
			height, /**< The height of the character image in the texture. */
			xoffset, /**< How much the current position should be offset when copying the image from the texture to the screen. */
			yoffset, /**< How much the current position should be offset when copying the image from the texture to the screen. */
			xadvance, /**< How much the current position should be advanced after drawing the character. */
			page; /**< The texture page where the character image is found. */
		char **kerning; /**< The kerning information is used to adjust the distance between certain characters. */
	};

	/** An AngelCode BMFont. */
	struct bmfont {
		unsigned int lineHeight, /**< This is the distance in pixels between each line of text. */
			base, /**< The number of pixels from the absolute top of the line to the base of the characters. */
			pages; /**< The number of texture pages included in the font. */
		char **pageNames; /**< The texture file names. */
		struct glyph *glyphs[PAGES]; /**< The characters. */
	};

	/** Returns a new \c struct bmfont from the specified AngelCode BMFont file.
		@param data The textual representation of the font file
		@return A new font */
	struct bmfont *create_bmfont(const char *data);

	/** Destroys the font in order to free up resources.
		@param font The font to free */
	void destroy_bmfont(struct bmfont *font);

	/** Returns the character information from the struct bmfont.
		@param font The font
		@param c The character to lookup
		@return The font's information on the character */
	struct glyph bmfont_get_glyph(struct bmfont *font, char c);

	/** Returns the kerning between the glyph and the character.
		@param glyph The glyph with the kerning data
		@param c The other character to find kerning between
		@return The kerning */
	int glyph_get_kerning(struct glyph glyph, char c);

#ifdef __cplusplus
}
#endif

#endif
