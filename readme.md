# netpw

A network socket acting as a source or sink for PipeWire streams.

<img src="doc/icon.png" width="256"/>

## Dependencies

- Boost (build only)
- FFmpeg (runtime only, must be present on system path if stream compression is used)
- OpenSSL
- PipeWire

## Contributions

Contributions are accepted.

## Platform Support

This program is intended to be run on Linux but should work in other POSIX environments that utilize PipeWire.

## Building

Run the following commands to build the project, replacing the version numbers with those appropriate for your system:

```sh
mkdir -p build
cd build
cmake -DNETPW_PIPEWIRE_VERSION=0.3 -DNETPW_SPA_VERSION=0.2 ..
make -j$(nproc)
```

## Usage

To run as a server:

```sh
netpw server input -h 0.0.0.0 -p 8000
```

To run as a client:

```sh
netpw client output -h 192.168.1.1 -p 8000
```

To run as a server with stream compression (the arguments following the double dash are passed to a child instance of FFmpeg):

```sh
netpw server input -h 0.0.0.0 -p 8000 -- -f mpegts
```

To run as a client with stream compression (the arguments following the double dash are passed to a child instance of FFmpeg):

```sh
netpw client input -h 0.0.0.0 -p 8000 -- -f mpegts
```

Running the program without any arguments will display the help information:

```sh
netpw
```

## State

The program does not read or write any configuration files or state files (e.g. in `~/.local`) but such files may be created on its behalf (e.g. by PipeWire).

## Known Issues

- Doesn't support video or MIDI streams, only audio.
- Currently relies on C++ & Boost for its lockless single-producer single-consumer queue.

## License

Developed by Amini Allight, licensed under the GPL 3.0.
