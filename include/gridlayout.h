/** Layout inspired by the CSS Grid Layout Module.
	@file gridlayout.h */

#ifndef GRIDLAYOUT_H
#define GRIDLAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <float.h>

#define MIN_CONTENT FLT_MIN
#define MAX_CONTENT FLT_MAX
#define AUTO { MIN_CONTENT, MAX_CONTENT }

#define FLEX(fr) ((fr) - 100)
#define GET_FLEX(fr) ((fr) + 100)
#define IS_FLEX(fr) ((fr) + 99 <= 0 && (fr) + 99 >= -1)

	enum {
		ALIGN_CENTER = 1 << 0, /**< Contents are centered. */
		ALIGN_TOP = 1 << 1, /**< The contents are aligned to the top edge of the box. */
		ALIGN_BOTTOM = 1 << 2, /**< The contents are aligned to the bottom edge of the box. */
		ALIGN_LEFT = 1 << 3, /**< The contents are aligned to the left edge of the box. */
		ALIGN_RIGHT = 1 << 4 /**< The contents are aligned to the right edge of the box. */
	};

	/** A 2D widget. */
	struct widget {
		float x, /**< The x-coordinate of the widget. */
			y, /**< The y-coordinate of the widget. */
			width, /**< The width of the widget. */
			height; /**< The height of the widget. */
	};

	struct size {
		float min, max;
	};

	struct item {
		int column,  /**< The column the item is in. */
			row,  /**< The row the cell is in. */
			colspan, /**< The number of columns spanned. */
			rowspan, /**< The number of rows spanned. */
			align; /**< Aligns the item within the grid area. */
		float fillX, fillY;
		struct widget *widget; /**< The widget. */
	};

	struct gridlayout {
		int columns, /**< The number of columns. */
			rows, /**< The number of rows. */
			itemCount, align;
		struct size *templateColumns, *templateRows;
		struct item *items;

		float gridMinWidth, gridMinHeight, gridMaxWidth, gridMaxHeight,
			(*minWidth)(struct widget *), (*minHeight)(struct widget *),
			(*maxWidth)(struct widget *), (*maxHeight)(struct widget *);
	};

	void layoutGrid(struct gridlayout *grid, float layoutX, float layoutY, float layoutWidth, float layoutHeight);

#ifdef __cplusplus
}
#endif

#endif