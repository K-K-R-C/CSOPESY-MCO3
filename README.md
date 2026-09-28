# CSOPESY Semi-Major Output 1 - OS Emulator

This program is a command-line OS emulator with a command interpreter, along with an animated text marquee.


## Group Developers
 - Bendol, Trisha Mae
 - Camato, Karl Kristoffer
 - Malapitan, Ryan James
 - Marcos, Alain Zuriel


## Entry Point
Main source file: **`marquee.cpp`** - contains the 'main()' function as the program's entry point.


## Repository Contents
 - `marquee.cpp` (source code)
 - `marquee.exe` (prebuilt Windows binary)
 - `marquee`     (prebuilt macOS binary)

## Requirements
 - C++11-compatible compiler (or later)
 - C++ Standard Library threading support
 - A terminal supporting ANSI/VT100 escape sequences


## Building & Running 
**Windows (MinGW / g++)**
```
g++ marquee.cpp -o marquee.exe -std=c++17 -pthread
marquee.exe
```

**macOS**
```
g++ -std=c++17 marquee.cpp -o marquee
./marquee
```


## Available Commands
| Command | Description |
|---|---|
| `help` | Displays the available commands and their descriptions |
| `start_marquee` | Starts the marquee animation |
| `stop_marquee` | Stops the marquee animation |
| `set_text <text>` | Sets the text displayed by the marquee |
| `set_speed <ms>` | Sets the marquee refresh interval (in milliseconds) |
| `exit` | Terminates the console |

## Notes
 - Default marquee refresh speed is 300 ms
 - Commands are case-sensitive (`help` works, but 'HELP' does not)
 - Any input with a leading space, or a blank input, is silently ignored rather than treated as an unrecognized command
 - `set_text` must be called with non-empty text before `start_marquee` will run
