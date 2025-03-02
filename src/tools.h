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
#ifndef NETPW_TOOLS_H
#define NETPW_TOOLS_H

char* get_file(const char* path);

int min(int a, int b);
int max(int a, int b);

int identify_spa_format(int bit_depth);

const char* identify_ffmpeg_format(int bit_depth);

/* allocs and returns ownership */
char* concat_strings(const char* a, const char* b);

/* allocs and returns ownership */
char* int_to_string(int x);

#endif
