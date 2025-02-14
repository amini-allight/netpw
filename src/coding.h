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
#ifndef NETPW_COMPRESSION_H
#define NETPW_COMPRESSION_H

#include "callback.h"

struct coding_context;

struct coding_context* coding_init_audio_encoder(
    int frequency,
    int channels,
    int depth,
    int encoder_argc,
    char** encoder_argv,
    on_data_callback callback
);
struct coding_context* coding_init_audio_decoder(
    int frequency,
    int channels,
    int depth,
    int encoder_argc,
    char** encoder_argv,
    on_data_callback callback
);
void coding_destroy(struct coding_context* ctx);

void coding_send(struct coding_context* ctx, const unsigned char* data, int size);

#endif
