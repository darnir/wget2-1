/*
 * Copyright(c) 2017 Free Software Foundation, Inc.
 *
 * This file is part of libwget.
 *
 * Libwget is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Libwget is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libwget.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "../config.h"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "wget.h"

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	char inbuf[2048];
	char *in;

	if (size < sizeof(inbuf))
		in = inbuf;
	else
		in = (char *) malloc(size + 1);

	assert(in != NULL);

	// 0 terminate
	memcpy(in, data, size);
	in[size] = 0;

	wget_metalink_t *metalink;
	metalink = wget_metalink_parse(in);
	wget_metalink_free(&metalink);

	if (in != inbuf)
		free(in);

	return 0;
}