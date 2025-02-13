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
#include "constants.h"
#include "error_handling.h"
#include "server.h"
#include "client.h"
#include "audio_input.h"
#include "audio_output.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>

static struct server* server = NULL;
static struct client* client = NULL;
static struct audio_input* audio_input = NULL;
static struct audio_output* audio_output = NULL;

static const char* host;
static unsigned short port = 8000;
static const char* ca = NULL;
static const char* privkey = NULL;
static const char* cert = NULL;
static int frequency = 48000;
static int channels = 2;
static int depth = 16;
static int buffer_size = 512;

static void on_audio_input(const unsigned char* data, int size)
{
    if (server)
    {
        server_send(server, data, size);
    }
    else if (client)
    {
        client_send(client, data, size);
    }
}

static void on_network_input(const unsigned char* data, int size)
{
    if (audio_output)
    {
        audio_output_send(audio_output, data, size);
    }
}

static void parse_arguments(int argc, char** argv)
{
    argc -= 2;
    argv += 2;

    static const struct option options[] = {
        { "host", required_argument, NULL, 'h' },
        { "port", required_argument, NULL, 'p' },
        { "ca", required_argument, NULL, 300 },
        { "privkey", required_argument, NULL, 301 },
        { "cert", required_argument, NULL, 302 },
        { "frequency", required_argument, NULL, 'f' },
        { "channels", required_argument, NULL, 'c' },
        { "depth", required_argument, NULL, 'd' },
        { "buffer", required_argument, NULL, 'b' },
        { NULL, 0, NULL, 0 }
    };

    int result;

    while (1)
    {
        result = getopt_long(argc, argv, "h:p:f:c:d:b:", options, NULL);

        if (result < 0)
        {
            break;
        }

        switch (result)
        {
        case 'h' :
            host = optarg;
            break;
        case 'p' :
            port = atoi(optarg);
            break;
        case 300 :
            ca = optarg;
            break;
        case 301 :
            privkey = optarg;
            break;
        case 302 :
            cert = optarg;
            break;
        case 'f' :
            frequency = atoi(optarg);
            break;
        case 'c' :
            channels = atoi(optarg);
            break;
        case 'd' :
            depth = atoi(optarg);
            break;
        case 'b' :
            buffer_size = atoi(optarg);
            break;
        }
    }
}

static void display_help()
{
    fprintf(stderr, "%s %s: PipeWire network socket virtual endpoint\n", NETPW_PROGRAM_NAME, NETPW_PROGRAM_VERSION);
    fprintf(stderr, "Copyright 2025 Amini Allight\n");
    fprintf(stderr, "This program comes with ABSOLUTELY NO WARRANTY; This is free software, and you are welcome to redistribute it under certain conditions. See the included license for further details.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: netpw server|client input|output [options]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "-h value\t--host value\t\tSpecify the network address to bind to or connect to.\n");
    fprintf(stderr, "-p value\t--port value\t\tSpecify the network port to bind to or connect to.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "\t\t--ca value\t\tSpecify the X.509 certificate authority certificate to use for TLS.\n");
    fprintf(stderr, "\t\t--privkey value\t\tSpecify the RSA private key to use for TLS.\n");
    fprintf(stderr, "\t\t--cert value\t\tSpecify the X.509 certificate to use for TLS.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "-f value\t--frequency value\tSpecify the audio sampling frequency.\n");
    fprintf(stderr, "-c value\t--channels value\tSpecify the audio channel count.\n");
    fprintf(stderr, "-d value\t--depth value\t\tSpecify the audio sample depth in bits.\n");
    fprintf(stderr, "-b value\t--buffer value\t\tSpecify the audio buffer in samples per channel.\n");
}

static void setup_server()
{
    server = server_init(host, port, ca, cert, privkey, on_network_input);
}

static void setup_client()
{
    client = client_init(host, port, ca, cert, privkey, on_network_input);
}

static void setup_audio_input()
{
    audio_input = audio_input_init(frequency, channels, depth, buffer_size, on_audio_input);
}

static void setup_audio_output()
{
    audio_output = audio_output_init(frequency, channels, depth, buffer_size);
}

int main(int argc, char** argv)
{
    if (argc == 1)
    {
        display_help();
        return 0;
    }
    else if (strcmp(argv[1], "server") == 0)
    {
        host = "0.0.0.0";
        parse_arguments(argc, argv);
        setup_server();

        if (strcmp(argv[2], "input") == 0)
        {
            setup_audio_input();

            audio_input_run(audio_input);

            audio_input_destroy(audio_input);
        }
        else if (strcmp(argv[2], "output") == 0)
        {
            setup_audio_output();

            audio_output_run(audio_output);

            audio_output_destroy(audio_output);
        }
        else
        {
            display_help();
            return 1;
        }

        server_destroy(server);
    }
    else if (strcmp(argv[1], "client") == 0)
    {
        host = "127.0.0.1";
        parse_arguments(argc, argv);
        setup_client();

        if (strcmp(argv[2], "input") == 0)
        {
            setup_audio_input();

            audio_input_run(audio_input);

            audio_input_destroy(audio_input);
        }
        else if (strcmp(argv[2], "output") == 0)
        {
            setup_audio_output();

            audio_output_run(audio_output);

            audio_output_destroy(audio_output);
        }
        else
        {
            display_help();
            return 1;
        }

        client_destroy(client);
    }
    else
    {
        display_help();
        return 1;
    }

    return 0;
}
