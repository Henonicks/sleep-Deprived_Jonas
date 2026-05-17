# sleep-Deprived Jonas (sDJ)

A Discord bot that continuously plays audio in a voice channel.

The content is pulled from the `resources` directory. Currently supported formats:

- WAV (.wav)
- MP3 (.mp3)
- OGG (.ogg)
- OPUS (.opus)
- FLAC (.flac)
- RAW PCM (.pcm/.raw) (the program always assumes its format is the one playable by D++!)

If the current file the program tries to open has a header, it's virtually converted (without overriding the file on disk)
into the format that's playable by D++: raw PCM data containing 16-bit signed stereo audio with a 48 kHz sample rate. The
program assumes the format of the current file is the one specified by its extension. So if a file is named `audio.wav`,
it will open it as a WAV file. So if the header is missing/corrupted, the program will fail and continue iterating.

## Prerequisites

- A C++-17-capable compiler
- [D++](https://github.com/brainboxdotcc/DPP) 10.1.4 (any 10.1 version should work but .4 is recommended)
- libsndfile (developed with 1.2.2-4)
- libsamplerate (developed with 0.2.2-3)

## Compilation

Create a build directory in the root directory of the project and change into it:

    mkdir build
    cd build

Now configure the CMake project and build it:

    cmake ..
    make -j

## Running

### Configuring

Simply run the program once, and it will create the default config files for you to edit. `config/behaviour.hfg` is always
required while `config/jonas.hfg` is only required if you're deploying the bot (e.g. not simply testing the files);

The default `config/behaviour.hfg` can be found here: [behaviour.hfg](default_configs/behaviour.hfg)
The default `config/jonas.hfg` can be found here: [jonas.hfg](default_configs/jonas.hfg)

### Executing

Simply make sure you're in the build directory and run the generated executable:

    cd build
    ./sleepless_jonas

## Extending the bot

You can add as many header files and .cpp files into the src and include folders as you wish. All .cpp files in the src directory will be linked together into the bot's executable.

## Renaming the bot

To rename the bot, search and replace "sleepless_jonas" in the `CMakeLists.txt` with your new bot's name and then rename the `jonas` folder in `include`. Rerun `cmake ..` from the `build` directory and rebuild. You might need to re-create the build directory.

## Credits

- `include/strnatcmp.hpp` is a single-header library taken from the fork @ https://github.com/Amerge/natsort
