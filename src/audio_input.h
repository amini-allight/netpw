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
#ifndef NETPW_AUDIO_INPUT_H
#define NETPW_AUDIO_INPUT_H

#include "callback.h"

struct audio_input;

struct audio_input* audio_input_init(int frequency, int channels, int depth, int buffer_size, on_data_callback callback);
void audio_input_run(struct audio_input* audio_input);
void audio_input_destroy(struct audio_input* audio_input);

#endif
