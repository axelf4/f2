#include "gridlayout.h"
#include <stdlib.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define IS_INTRINSIC(size) ((size) == MIN_CONTENT || (size) == MAX_CONTENT)

struct track {
	float base,
		growth; /** The growth limit. */
};

static void initializeTrackSizes(int count, struct track *tracks, struct size *template) {
	for (int i = 0; i < count; i++) {
		struct track *track = tracks + i;
		float min = template[i].min, max = template[i].max;

		track->base = IS_INTRINSIC(min) || IS_FLEX(min) ? 0 : min;
		if (IS_INTRINSIC(max) || IS_FLEX(max)) track->growth = track->base;
		else track->growth = max;
		if (track->growth < track->base) track->growth = track->base;
	}
}

static void maximizeTracks(float space, int count, struct track *tracks) {
	int unfrozen = 0;
	float growth = 0;
	for (int i = 0; i < count; i++) {
		struct track *track = tracks + i;
		if (track->base < track->growth) {
			unfrozen++;
			if (growth == 0 || track->growth - track->base < growth) growth = track->growth - track->base;
		}
	}
	if (unfrozen == 0) return;
	if (growth * unfrozen > space) growth = space / unfrozen;
	for (int i = 0; i < count; i++) {
		struct track *track = tracks + i;
		track->base += growth;
		if (track->base - track->growth < FLT_EPSILON) unfrozen--;
	}
	if (unfrozen > 0) maximizeTracks(space, count, tracks);
}

static void stretchFlexibleTracks(float space, int count, struct track *tracks, struct size *template) {
	float leftover = space, // Let leftover space be the space to fill minus the base sizes of the non - flexible grid tracks.
		flexSum = 0; // Let flex factor sum be the sum of the flex factors of the flexible tracks. If this value is less than 1, set it to 1 instead.
	for (int i = 0; i < count; i++) if (IS_FLEX(template[i].max)) flexSum += GET_FLEX(template[i].max);
	else leftover -= tracks[i].base;
	float hypothetical = leftover / MAX(flexSum, 1); // Let the hypothetical fr size be the leftover space divided by the flex factor sum.
	// If the product of the hypothetical fr size and a flexible track’s flex factor is less than the track’s base size, restart this algorithm treating all such tracks as inflexible.
	for (int i = 0; i < count; i++) {
		float flex = template[i].max;
		if (IS_FLEX(flex) && hypothetical * GET_FLEX(flex) < tracks[i].base) {
			template[i].max = 0;
			stretchFlexibleTracks(space, count, tracks, template);
			return;
		}
	}
	for (int i = 0; i < count; i++) if (IS_FLEX(template[i].max)) tracks[i].base = hypothetical * GET_FLEX(template[i].max);
}

void layoutGrid(struct gridlayout *grid, float layoutX, float layoutY, float layoutWidth, float layoutHeight) {
	struct track *columnTracks = malloc(grid->columns * sizeof(struct track)),
		*rowTracks = malloc(grid->rows * sizeof(struct track));

	// Initialize each track’s base size and growth limit.
	initializeTrackSizes(grid->columns, columnTracks, grid->templateColumns);
	initializeTrackSizes(grid->rows, rowTracks, grid->templateRows);

	// Resolve intrinsic track sizing functions to absolute lengths.
	int span = 1, repeat = 0;
	do {
		repeat = 0;
		for (int i = 0; i < grid->itemCount; i++) {
			struct item item = grid->items[i];

			if (item.colspan == span) {
				for (int column = item.column, n = column + item.colspan; column < n; column++) {
					struct track *track = columnTracks + column;
					struct size template = grid->templateColumns[column];

					if (template.min == MIN_CONTENT || IS_FLEX(template.min)) track->base = MAX(track->base, grid->minWidth(item.widget));
					else if (template.min == MAX_CONTENT) track->base = MAX(track->base, grid->maxWidth(item.widget));

					if (template.max == MIN_CONTENT) track->growth = MAX(track->growth, grid->minWidth(item.widget));
					else if (template.max == MAX_CONTENT) track->growth = MAX(track->growth, grid->maxWidth(item.widget));

					if (track->growth < track->base) track->growth = track->base;
				}
			} else if (item.colspan > span) repeat = 1;

			if (item.rowspan == span) {
				for (int row = item.row, n = row + item.rowspan; row < n; row++) {
					struct track *track = rowTracks + row;
					struct size template = grid->templateRows[row];

					if (template.min == MIN_CONTENT || IS_FLEX(template.min)) track->base = MAX(track->base, grid->minHeight(item.widget));
					else if (template.min == MAX_CONTENT) track->base = MAX(track->base, grid->maxHeight(item.widget));

					if (template.max == MIN_CONTENT) track->growth = MAX(track->growth, grid->minHeight(item.widget));
					else if (template.max == MAX_CONTENT) track->growth = MAX(track->growth, grid->maxHeight(item.widget));

					if (track->growth < track->base) track->growth = track->base;
				}
			} else if (item.rowspan > span) repeat = 1;
		}
		span++;
	} while (repeat);
	// TODO If any track still has an infinite growth limit (because, for example, it had no items placed in it), set its growth limit to its base size.

	float gridMinWidth = 0, gridMinHeight = 0, gridMaxWidth = 0, gridMaxHeight = 0;
	for (int column = 0; column < grid->columns; column++) {
		gridMinWidth += columnTracks[column].base;
		gridMaxWidth += columnTracks[column].growth;
	}
	for (int row = 0; row < grid->rows; row++) {
		gridMinHeight += rowTracks[row].base;
		gridMaxHeight += rowTracks[row].growth;
	}
	grid->gridMinWidth = gridMinWidth;
	grid->gridMinHeight = gridMinHeight;
	grid->gridMaxWidth = gridMaxWidth;
	grid->gridMaxHeight = gridMaxHeight;

	float freeWidth = layoutWidth - gridMinWidth, freeHeight = layoutHeight - gridMinHeight;
	if (freeWidth > 0) maximizeTracks(freeWidth, grid->columns, columnTracks);
	if (freeHeight > 0) maximizeTracks(freeHeight, grid->rows, rowTracks);

	stretchFlexibleTracks(layoutWidth, grid->columns, columnTracks, grid->templateColumns);
	stretchFlexibleTracks(layoutHeight, grid->rows, rowTracks, grid->templateRows);

	// Position widgets within grid areas.
	float currentX = 0, currentY = 0;
	for (int column = 0; column < grid->columns; column++) {
		for (int row = 0; row < grid->rows; row++) {
			// Find the item in the grid area
			struct item *item = 0;
			for (int i = 0; i < grid->itemCount; i++) if (grid->items[i].column == column && grid->items[i].row == row) item = grid->items + i;

			if (item != 0) {
				float width = 0, height = 0;
				for (int i = column; i < column + item->colspan; i++) width += columnTracks[i].base;
				for (int i = row; i < row + item->rowspan; i++) height += rowTracks[i].base;

				if (item->fillX > 0) {
					item->widget->width = width * item->fillX;
					float maxWidth = grid->maxWidth(item->widget);
					if (maxWidth > 0) item->widget->width = MIN(item->widget->width, maxWidth);
				} else item->widget->width = grid->minWidth(item->widget);
				if (item->fillY > 0) {
					item->widget->height = height * item->fillY;
					float maxHeight = grid->maxHeight(item->widget);
					if (maxHeight > 0) item->widget->height = MIN(item->widget->height, maxHeight);
				} else item->widget->height = grid->minHeight(item->widget);

				if ((item->align & ALIGN_LEFT) != 0) item->widget->x = currentX;
				else if ((item->align & ALIGN_RIGHT) != 0) item->widget->x = currentX + columnTracks[item->column].base - item->widget->width;
				else item->widget->x = currentX + (width - item->widget->width) / 2;

				if ((item->align & ALIGN_TOP) != 0) item->widget->y = currentY;
				else if ((item->align & ALIGN_BOTTOM) != 0) item->widget->y = currentY + height - item->widget->height;
				else item->widget->y = currentY + (height - item->widget->height) / 2;
			}
			currentY += rowTracks[row].base;
		}
		currentX += columnTracks[column].base;
		currentY = 0;
	}
}