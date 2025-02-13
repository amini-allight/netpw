/*
Copyright 2025 Amini Allight

This file is part of netpw.

netpw is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

netpw is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with netpw. If not, see <https://www.gnu.org/licenses/>.
*/
#include "tools.h"
#include "error_handling.h"

#include <stdio.h>
#include <stdlib.h>
#include <spa/param/audio/raw.h>

char* get_file(const char* path)
{
    int result;
    FILE* file;

    CHECK_POINTER(file = fopen(path, "rb"));

    int size;

    CHECK_ERRNO(fseek(file, 0, SEEK_END));
    CHECK_ERRNO(size = ftell(file));
    CHECK_ERRNO(fseek(file, 0, SEEK_SET));

    char* data = malloc(size * sizeof(char));

    fread(data, size, sizeof(char), file);

    CHECK_ERRNO(fclose(file));

    return data;
}

int min(int a, int b)
{
    return a < b ? a : b;
}

int max(int a, int b)
{
    return a > b ? a : b;
}

int identify_format(int bit_depth)
{
    switch (bit_depth)
    {
    default :
        fprintf(stderr, "unsupported bit depth: %i\n", bit_depth);
        exit(1);
    case 8 :
        return SPA_AUDIO_FORMAT_S8;
    case 16 :
        return SPA_AUDIO_FORMAT_S16;
    case 24 :
        return SPA_AUDIO_FORMAT_S24;
    case 32 :
        return SPA_AUDIO_FORMAT_S32;
    }
}
