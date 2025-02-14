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
#include "audio_output.h"
#include "tools.h"
#include "constants.h"
#include "error_handling.h"
#include "lockfree_spsc_queue.h"

#include <stdlib.h>
#define __LOCALE_C_ONLY
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

struct audio_output
{
    struct pw_main_loop* main_loop;
    struct pw_context* context;
    struct pw_core* core;
    struct pw_stream* stream;
    struct spa_hook hook;
    struct lockfree_spsc_queue* queue;
    int stride;
};

static void audio_output_process(void* userdata)
{
    struct audio_output* audio_output = userdata;

    struct pw_buffer* buffer = pw_stream_dequeue_buffer(audio_output->stream);

    if (!buffer)
    {
        return;
    }

    unsigned char* out_data = buffer->buffer->datas[0].data;
    uint32_t out_size = buffer->buffer->datas[0].maxsize;
    buffer->buffer->datas[0].chunk->offset = 0;
    buffer->buffer->datas[0].chunk->stride = audio_output->stride;
    buffer->buffer->datas[0].chunk->size = out_size;

    int actual_read_size = lockfree_spsc_queue_pull(audio_output->queue, out_data, out_size);

    memset(out_data + actual_read_size, 0, out_size - actual_read_size);

    pw_stream_queue_buffer(audio_output->stream, buffer);
}

static struct pw_stream_events stream_listener = {
    .version = PW_VERSION_STREAM_EVENTS,
    .process = audio_output_process
};

static void on_quit(void* userdata, int signal)
{
    struct audio_output* audio_output = userdata;
    pw_main_loop_quit(audio_output->main_loop);
}

struct audio_output* audio_output_init(int frequency, int channels, int depth, int buffer_size)
{
    int result;

    struct audio_output* audio_output = malloc(sizeof(struct audio_output));

    unsigned char queue[1024];
    struct spa_pod_builder builder = SPA_POD_BUILDER_INIT(queue, sizeof(queue));

    struct spa_audio_info_raw info = {
        .format = identify_spa_format(depth),
        .rate = frequency,
        .channels = channels
    };

    const struct spa_pod* format = spa_format_audio_raw_build(
        &builder,
        SPA_PARAM_EnumFormat,
        &info
    );

    pw_init(0, NULL);

    CHECK_POINTER_FATAL(audio_output->main_loop = pw_main_loop_new(NULL));
    pw_loop_add_signal(pw_main_loop_get_loop(audio_output->main_loop), SIGINT, on_quit, audio_output);
    pw_loop_add_signal(pw_main_loop_get_loop(audio_output->main_loop), SIGTERM, on_quit, audio_output);

    CHECK_POINTER_FATAL(audio_output->context = pw_context_new(pw_main_loop_get_loop(audio_output->main_loop), NULL, 0));

    CHECK_POINTER_FATAL(audio_output->core = pw_context_connect(audio_output->context, NULL, 0));

    struct pw_properties* properties = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Playback",
        PW_KEY_MEDIA_ROLE, "Network",
        PW_KEY_APP_NAME, NETPW_PROGRAM_NAME,
        PW_KEY_APP_ID, NETPW_PROGRAM_NAME,
        PW_KEY_NODE_NAME, "Playback",
        PW_KEY_NODE_DESCRIPTION, "Playback",
        NULL
    );

    pw_properties_setf(properties, PW_KEY_NODE_LATENCY, "%u/%u", buffer_size, frequency);
    pw_properties_setf(properties, PW_KEY_NODE_RATE, "1/%u", frequency);

    CHECK_POINTER_FATAL(audio_output->stream = pw_stream_new(audio_output->core, NETPW_PROGRAM_NAME, properties));

    pw_stream_add_listener(audio_output->stream, &audio_output->hook, &stream_listener, audio_output);

    CHECK_ERROR_FATAL(lockfree_spsc_queue_init(&audio_output->queue));
    audio_output->stride = channels * (depth / 8);

    CHECK_ERROR_FATAL(pw_stream_connect(
        audio_output->stream,
        PW_DIRECTION_OUTPUT,
        PW_ID_ANY,
        PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS,
        &format,
        1
    ));

    return audio_output;
}

void audio_output_send(struct audio_output* audio_output, const unsigned char* data, int size)
{
    lockfree_spsc_queue_push(audio_output->queue, data, size);
}

void audio_output_run(struct audio_output* audio_output)
{
    int result;

    CHECK_ERROR(pw_main_loop_run(audio_output->main_loop));
}

void audio_output_destroy(struct audio_output* audio_output)
{
    pw_stream_disconnect(audio_output->stream);
    lockfree_spsc_queue_destroy(audio_output->queue);
    pw_stream_destroy(audio_output->stream);
    pw_core_disconnect(audio_output->core);
    pw_context_destroy(audio_output->context);
    pw_main_loop_destroy(audio_output->main_loop);
    pw_deinit();
    free(audio_output);
}
