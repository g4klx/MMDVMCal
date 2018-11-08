/*
 *	Copyright (C) 2009,2014-2016 Jonathan Naylor, G4KLX
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 */

#include "Utils.h"

#include <cstdio>
#include <cassert>

void CUtils::dump(const std::string& title, const unsigned char* data, unsigned int length)
{
	assert(data != NULL);

#if defined(_WIN32) || defined(_WIN64)
	::fprintf(stdout, "%s\n", title.c_str());
#else
	::fprintf(stdout, "%s\r\n", title.c_str());
#endif

	unsigned int offset = 0U;

	while (length > 0U) {
		::fprintf(stdout, "%04X:  ", offset);

		unsigned int bytes = (length > 16U) ? 16U : length;

		for (unsigned i = 0U; i < bytes; i++)
			::fprintf(stdout, "%02X ", data[offset + i]);

		for (unsigned int i = bytes; i < 16U; i++)
			::fprintf(stdout, "   ");

		::fprintf(stdout, "   *");

		for (unsigned i = 0U; i < bytes; i++) {
			unsigned char c = data[offset + i];

			if (::isprint(c))
				::fprintf(stdout, "%c", c);
			else
				::fprintf(stdout, ".");
		}

#if defined(_WIN32) || defined(_WIN64)
		::fprintf(stdout, "*\n");
#else
		::fprintf(stdout, "*\r\n");
#endif

		offset += 16U;

		if (length >= 16U)
			length -= 16U;
		else
			length = 0U;
	}
}

void CUtils::bitsToByteBE(const bool* bits, unsigned char& byte)
{
	assert(bits != NULL);

	byte  = bits[0U] ? 0x80U : 0x00U;
	byte |= bits[1U] ? 0x40U : 0x00U;
	byte |= bits[2U] ? 0x20U : 0x00U;
	byte |= bits[3U] ? 0x10U : 0x00U;
	byte |= bits[4U] ? 0x08U : 0x00U;
	byte |= bits[5U] ? 0x04U : 0x00U;
	byte |= bits[6U] ? 0x02U : 0x00U;
	byte |= bits[7U] ? 0x01U : 0x00U;
}
