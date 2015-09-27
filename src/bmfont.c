#include "bmfont.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

struct bmfont *create_bmfont(const char *data) {
	if (strncmp(data, "BMF\003", 4) != 0) return 0; // Check magic, only binary is supported
	data += 4;

	struct bmfont *font = (struct bmfont *)malloc(sizeof(struct bmfont));
	if (font == 0) return 0;

	for (unsigned int p = 0; p < PAGES; p++) {
		font->glyphs[p] = malloc(sizeof(struct glyph) * PAGE_SIZE);
	}

	while (*data != '\0') {
		char blockType = *data++;
		unsigned int blockSize = ((unsigned) data[3] << 24) | ((unsigned) data[2] << 16) | ((unsigned) data[1] << 8) | (((unsigned char *) data)[0]);
		data += 4; // Skip size

		switch (blockType) {
		case 1:
			data += blockSize; // Skip the information on how the font was generated
			break;
		case 2:
		{
			// Read common block
			unsigned int lineHeight = font->lineHeight = ((int) data[1] << 8) | data[0];
			data += 2;
			unsigned int base = font->base = data[1] << 8 | data[0] & 0xFF;
			data += 6; // Skip base, scaleW and scaleH
			unsigned int pages = font->pages = data[1] << 8 | data[0] & 0xFF;
			data += 7;

			// font->glyphs = malloc(sizeof(struct glyph *) * pages);
			// for (unsigned int p = 0; p < font->pages; p++) font->glyphs[p] = malloc(sizeof(struct glyph) * PAGE_SIZE);
			break;
		}
		case 3:
		{
			// Page block
			char **pageNames = font->pageNames = malloc(sizeof(char *) * font->pages);
			for (unsigned int p = 0, n = strlen(data); p < font->pages; p++) {
				strcpy(pageNames[p] = malloc(n + 1), data);
				data += n + 1;
			}
			break;
		}
		case 4:
		{
			// Character block
			int numChars = blockSize / 20;
			for (int c = 0; c < numChars; c++, data += 20) {
				struct glyph glyph;
				unsigned int id = glyph.id = ((data[3] & 0xFF) << 24) | ((data[2] & 0xFF) << 16) | ((data[1] & 0xFF) << 8) | (data[0] & 0xFF);
				// unsigned int id = glyph.id = (unsigned char) data[3] << 24 | (unsigned char) data[2] << 16 | (unsigned char) data[1] << 8 | (unsigned char) data[0];
				glyph.x = data[5] << 8 | data[4] & 0xFF;
				glyph.y = data[7] << 8 | data[6] & 0xFF;
				glyph.width = data[9] << 8 | data[8] & 0xFF;
				glyph.height = data[11] << 8 | data[10] & 0xFF;
				glyph.xoffset = data[13] << 8 | data[12] & 0xFF;
				glyph.yoffset = data[15] << 8 | data[14] & 0xFF;
				glyph.xadvance = data[17] << 8 | data[16] & 0xFF;
				glyph.page = data[18];
				glyph.kerning = 0;
				unsigned int chnl = data[19];

				if (id <= CHAR_MAX) {
					font->glyphs[id / PAGE_SIZE][id & PAGE_SIZE - 1] = glyph;
				}
			}
			break;
		}
		case 5:
		{
			// Kerning pair block
			for (unsigned int i = 0; i < blockSize / 10; i++, data += 10) {
				unsigned int first = (unsigned char) data[3] << 24 | (unsigned char) data[2] << 16 | (unsigned char) data[1] << 8 | (unsigned char) data[0],
					second = (unsigned char) data[7] << 24 | (unsigned char) data[6] << 16 | (unsigned char) data[5] << 8 | (unsigned char) data[4];
				int amount = data[9] << 8 | data[8];

				struct glyph *glyph = font->glyphs[first / PAGE_SIZE] + (first & PAGE_SIZE - 1);
				if (glyph->kerning == 0) glyph->kerning = calloc(PAGES, sizeof(char *));

				char *page = glyph->kerning[second >> LOG2_PAGE_SIZE];
				if (page == 0) page = glyph->kerning[second >> LOG2_PAGE_SIZE] = calloc(PAGE_SIZE, 1);
				page[second & PAGE_SIZE - 1] = amount;
			}
			int i = bmfont_get_glyph(font, 'A').kerning['l' >> LOG2_PAGE_SIZE]['l' & PAGE_SIZE - 1];
			break;
		}
		default:
			printf("blockType: %d.\n", blockType);
		}
	}

	return font;
}

void destroy_bmfont(struct bmfont *font) {
	for (unsigned int p = 0; p < PAGES; p++) {
		free(font->glyphs[p]);
	}
	for (unsigned int p = 0; p < font->pages; p++) {
		free(font->pageNames[p]);
	}
	free(font->pageNames);
	free(font);
}

struct glyph bmfont_get_glyph(struct bmfont *font, char c) {
	struct glyph glyph = font->glyphs[c / PAGE_SIZE][c & PAGE_SIZE - 1];
	return glyph;
}

int glyph_get_kerning(struct glyph glyph, char c) {
	if (glyph.kerning == 0) return 0;
	int i = glyph.kerning[c >> LOG2_PAGE_SIZE][c & PAGE_SIZE - 1];
	return i;
}
