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
#ifndef NETPW_ERROR_HANDLING_H
#define NETPW_ERROR_HANDLING_H

#include <stdio.h>
#include <errno.h>

#define CHECK_POINTER(_expr) \
{ \
    void* pointer = (_expr); \
    if (!pointer) \
    { \
        fprintf(stderr, "'%s' failed: %i\n", #_expr, errno); \
    } \
}

#define CHECK_ERRNO(_expr) \
result = (_expr); \
if (result < 0) \
{ \
    fprintf(stderr, "'%s' failed: %i\n", #_expr, errno); \
}

#define CHECK_ERROR(_expr) \
result = (_expr); \
if (result != 0) \
{ \
    fprintf(stderr, "'%s' failed: %i\n", #_expr, result); \
}

#define CHECK_POINTER_FATAL(_expr) \
{ \
    void* pointer = (_expr); \
    if (!pointer) \
    { \
        fprintf(stderr, "'%s' failed: %i\n", #_expr, errno); \
        exit(1); \
    } \
}

#define CHECK_ERRNO_FATAL(_expr) \
result = (_expr); \
if (result < 0) \
{ \
    fprintf(stderr, "'%s' failed: %i\n", #_expr, errno); \
    exit(1); \
}

#define CHECK_ERROR_FATAL(_expr) \
result = (_expr); \
if (result != 0) \
{ \
    fprintf(stderr, "'%s' failed: %i\n", #_expr, result); \
    exit(1); \
}

#endif
