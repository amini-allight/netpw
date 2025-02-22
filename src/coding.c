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
#include "coding.h"
#include "error_handling.h"
#include "tools.h"
#include "constants.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>

struct coding_context
{
    int child_in[2];
    int child_out[2];
    unsigned char buffer[NETPW_IO_BUFFER_SIZE];
    pid_t child;
    on_data_callback callback;
    pthread_t thread;
};

static void* coding_receive(void* arg)
{
    struct coding_context* ctx = arg;

    int result;

    while (1)
    {
        result = read(ctx->child_out[READ_PIPE_INDEX], ctx->buffer, NETPW_IO_BUFFER_SIZE);

        if (result < 0)
        {
            break;
        }

        ctx->callback(ctx->buffer, result);
    }

    return NULL;
}

static struct coding_context* coding_init(int argc, char** argv, on_data_callback callback)
{
    int result;

    struct coding_context* ctx = malloc(sizeof(struct coding_context));

    CHECK_ERRNO_FATAL(pipe(ctx->child_in));
    CHECK_ERRNO_FATAL(pipe(ctx->child_out));

    ctx->child = fork();

    if (ctx->child == 0)
    {
        CHECK_ERRNO(close(ctx->child_in[WRITE_PIPE_INDEX]));
        CHECK_ERRNO_FATAL(dup2(ctx->child_in[READ_PIPE_INDEX], STDIN_FILENO));
        CHECK_ERRNO(close(ctx->child_in[READ_PIPE_INDEX]));

        CHECK_ERRNO(close(ctx->child_out[READ_PIPE_INDEX]));
        CHECK_ERRNO_FATAL(dup2(ctx->child_out[WRITE_PIPE_INDEX], STDOUT_FILENO));
        CHECK_ERRNO(close(ctx->child_out[WRITE_PIPE_INDEX]));

        execvp("ffmpeg", argv);
    }
    else
    {
        CHECK_ERRNO(close(ctx->child_in[READ_PIPE_INDEX]));
        CHECK_ERRNO(close(ctx->child_out[WRITE_PIPE_INDEX]));

        ctx->callback = callback;
        CHECK_ERROR_FATAL(pthread_create(&ctx->thread, NULL, coding_receive, ctx));
    }

    return ctx;
}

struct coding_context* coding_init_audio_encoder(
    int frequency,
    int channels,
    int depth,
    int encoder_argc,
    char** encoder_argv,
    on_data_callback callback
)
{
    int argc = 9 + encoder_argc + 2;
    char** argv = malloc(sizeof(char*) * argc);

    argv[0] = "ffmpeg";
    argv[1] = "-f";
    argv[2] = strdup(identify_ffmpeg_format(depth));
    argv[3] = "-ar";
    argv[4] = int_to_string(frequency);
    argv[5] = "-ac";
    argv[6] = int_to_string(channels);
    argv[7] = "-i";
    argv[8] = "-";

    int i;
    for (i = 0; i < encoder_argc; i++)
    {
        argv[9 + i] = encoder_argv[i];
    }

    argv[argc - 2] = "-";
    argv[argc - 1] = NULL;

    return coding_init(argc, argv, callback);
}

struct coding_context* coding_init_audio_decoder(
    int frequency,
    int channels,
    int depth,
    int encoder_argc,
    char** encoder_argv,
    on_data_callback callback
)
{
    int argc = 1 + encoder_argc + 12;
    char** argv = malloc(sizeof(char*) * argc);

    argv[0] = "ffmpeg";

    int i;
    for (i = 0; i < encoder_argc; i++)
    {
        argv[1 + i] = encoder_argv[i];
    }

    argv[encoder_argc + 1] = "-i";
    argv[encoder_argc + 2] = "-";
    argv[encoder_argc + 3] = "-c:a";
    argv[encoder_argc + 4] = concat_strings("pcm_", strdup(identify_ffmpeg_format(depth)));
    argv[encoder_argc + 5] = "-f";
    argv[encoder_argc + 6] = strdup(identify_ffmpeg_format(depth));
    argv[encoder_argc + 7] = "-ar";
    argv[encoder_argc + 8] = int_to_string(frequency);
    argv[encoder_argc + 9] = "-ac";
    argv[encoder_argc + 10] = int_to_string(channels);
    argv[encoder_argc + 11] = "-";
    argv[encoder_argc + 12] = NULL;

    return coding_init(argc, argv, callback);
}

void coding_destroy(struct coding_context* ctx)
{
    int result;

    CHECK_ERRNO(kill(ctx->child, SIGINT));

    waitpid(ctx->child, NULL, 0);

    CHECK_ERROR(pthread_join(ctx->thread, NULL));

    CHECK_ERRNO(close(ctx->child_in[WRITE_PIPE_INDEX]));
    CHECK_ERRNO(close(ctx->child_out[READ_PIPE_INDEX]));

    free(ctx);
}

void coding_send(struct coding_context* ctx, const unsigned char* data, int size)
{
    int result;

    CHECK_ERRNO(write(ctx->child_in[WRITE_PIPE_INDEX], data, size));
}
