/* The MIT License

   Copyright (C) 2011, 2012, 2013 Zilong Tan (eric.zltan@gmail.com)

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#include <stddef.h>
#include <string.h>
#include "util_algo.h"
#include "str_util.h"

char *nextline(char *buf, long size)
{
	char *end = buf + size;

	while (buf < end && *buf != '\n' && *buf != '\0')
		buf++;
	if (buf >= end)
		return NULL;
	*buf++ = '\0';
	return (buf >= end? NULL: buf);
}

const char *getfield(const char *from, const char *end,
		     int id, char *field, int flen, int delim)
{
	while (id-- > 0 && from < end) {
		from = memchr(from, delim, end - from);
		if (from == NULL)
			return end;
		++from;
	}
	if (from < end && field && flen > 0) {
		ptrdiff_t len = _min(end - from, (ptrdiff_t)(flen - 1));
		const char *next = (const char *)memchr(from, delim, len);
		if (next == NULL) {
			memcpy(field, from, len);
			field[len] = 0;
		} else {
			memcpy(field, from, next - from);
			field[next - from] = 0;
		}
	}
	return from;
}

static inline int __matchstar(char ch, const char *text, const char *regex);

static inline int __match(const char *text, const char *regex)
{
	if (*regex == '\0')
		return 1;
	if (*regex == '$' && regex[1] == '\0')
		return *text == '\0';
	if (regex[1] == '*')
		return __matchstar(*regex, text, regex + 2);
	if (*text && (*regex == '.' || *text == *regex))
		return __match(text + 1, regex + 1);
	return 0;
}

int __matchstar(char ch, const char *text, const char *regex)
{
	do {
		if (__match(text, regex))
			return 1;
	} while (*text && (*text++ == ch || ch == '.'));
	return 0;
}

int regex_match(const char *text, const char *regex)
{
	if (*regex == '^')
		return __match(text, regex + 1);

	do {
		if (__match(text, regex))
			return 1;
	} while (*text++ != '\0');

	return 0;
}
