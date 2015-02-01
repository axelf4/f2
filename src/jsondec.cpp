#include "jsondec.h"
#include <assert.h>

struct DecoderState {
	struct JSONObjectDecoder *dec;
	char *start;
};

void skip_whitespace(struct DecoderState *ds) {
	char *start = ds->start;
	for (; start == ' ' || start == '\t' || start == '\r' || start == '\n'; start++);
	ds->start = start;
}

JSOBJ decode_numeric(struct DecoderState *ds) {}
JSOBJ decode_string(struct DecoderState *ds) {}
JSOBJ decode_array(struct DecoderState *ds) {}
JSOBJ decode_object(struct DecoderState *ds) {}
JSOBJ decode_true(struct DecoderState *ds) {
}
JSOBJ decode_false(struct DecoderState *ds) {}
JSOBJ decode_null(struct DecoderState *ds) {}


JSOBJ decode_any(struct DecoderState *ds) {
	for (;;) {
		switch (*ds->start) {
		case '"': return decode_string(ds);
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '-':
			return decode_numeric(ds);
		case '{': return decode_object(ds);
		case '[': return decode_array(ds);
		case 't': return decode_true(ds);
		case 'f': return decode_false(ds);
		case 'n': return decode_null(ds);
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			// Whitespace character
			ds->start++;
			break;
		default:
			// TODO ERROR
			return 0;
		}
	}
}

JSOBJ JSON_DecodeObject(const char *string) {
	struct DecoderState ds;
	ds.start = string;

	decode_any(&ds);
}