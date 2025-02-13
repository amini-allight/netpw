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
#include "audio_input.h"
#include "tools.h"
#include "constants.h"
#include "error_handling.h"

#include <stdlib.h>
#define __LOCALE_C_ONLY
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

struct audio_input
{
    struct pw_main_loop* main_loop;
    struct pw_context* context;
    struct pw_core* core;
    struct pw_stream* stream;
    struct spa_hook hook;
    on_data_callback callback;
};

static void audio_input_process(void* userdata)
{
    struct audio_input* audio_input = userdata;

    struct pw_buffer* buffer = pw_stream_dequeue_buffer(audio_input->stream);

    if (!buffer)
    {
        return;
    }

    audio_input->callback(buffer->buffer->datas[0].data, buffer->buffer->datas[0].chunk->size);

    pw_stream_queue_buffer(audio_input->stream, buffer);
}

static struct pw_stream_events stream_listener = {
    .version = PW_VERSION_STREAM_EVENTS,
    .process = audio_input_process
};

static void on_quit(void* userdata, int signal)
{
    struct audio_input* audio_input = userdata;
    pw_main_loop_quit(audio_input->main_loop);
}

struct audio_input* audio_input_init(int frequency, int channels, int depth, int buffer_size, on_data_callback callback)
{
    int result;

    struct audio_input* audio_input = malloc(sizeof(struct audio_input));

    unsigned char buffer[1024];
    struct spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    struct spa_audio_info_raw info = {
        .format = identify_format(depth),
        .rate = frequency,
        .channels = channels
    };

    const struct spa_pod* format = spa_format_audio_raw_build(
        &builder,
        SPA_PARAM_EnumFormat,
        &info
    );

    pw_init(0, NULL);

    CHECK_POINTER_FATAL(audio_input->main_loop = pw_main_loop_new(NULL));
    pw_loop_add_signal(pw_main_loop_get_loop(audio_input->main_loop), SIGINT, on_quit, audio_input);
    pw_loop_add_signal(pw_main_loop_get_loop(audio_input->main_loop), SIGTERM, on_quit, audio_input);

    CHECK_POINTER_FATAL(audio_input->context = pw_context_new(pw_main_loop_get_loop(audio_input->main_loop), NULL, 0));

    CHECK_POINTER_FATAL(audio_input->core = pw_context_connect(audio_input->context, NULL, 0));

    struct pw_properties* properties = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Network",
        PW_KEY_APP_NAME, NETPW_PROGRAM_NAME,
        PW_KEY_APP_ID, NETPW_PROGRAM_NAME,
        PW_KEY_NODE_NAME, "Capture",
        PW_KEY_NODE_DESCRIPTION, "Capture",
        NULL
    );

    pw_properties_setf(properties, PW_KEY_NODE_LATENCY, "%u/%u", buffer_size, frequency);
    pw_properties_setf(properties, PW_KEY_NODE_RATE, "1/%u", frequency);

    CHECK_POINTER_FATAL(audio_input->stream = pw_stream_new(audio_input->core, NETPW_PROGRAM_NAME, properties));

    pw_stream_add_listener(audio_input->stream, &audio_input->hook, &stream_listener, audio_input);

    CHECK_ERROR_FATAL(pw_stream_connect(
        audio_input->stream,
        PW_DIRECTION_INPUT,
        PW_ID_ANY,
        PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS,
        &format,
        1
    ));

    audio_input->callback = callback;

    return audio_input;
}

void audio_input_run(struct audio_input* audio_input)
{
    int result;

    CHECK_ERROR(pw_main_loop_run(audio_input->main_loop));
}

void audio_input_destroy(struct audio_input* audio_input)
{
    pw_stream_disconnect(audio_input->stream);
    pw_stream_destroy(audio_input->stream);
    pw_core_disconnect(audio_input->core);
    pw_context_destroy(audio_input->context);
    pw_main_loop_destroy(audio_input->main_loop);
    pw_deinit();
    free(audio_input);
}
