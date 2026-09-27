#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

// ---- Shared state between main thread and marquee thread ----
std::mutex stateMutex;   // protects savedText / marqueeSpeedMs
std::mutex coutMutex;    // protects console output from interleaving
std::atomic<bool> marqueeRunning(false);

std::string savedText;
int marqueeSpeedMs = 300; // default refresh rate

std::thread marqueeThread;

// Enables ANSI/VT100 escape sequence support on Windows consoles
// (needed for cursor save/restore and color codes). No-op elsewhere.
void enableAnsiSupport() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return;
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

void printHeader() {
    std::cout << "Group developer:" << std::endl;
    std::cout << "Marcos, Alain Zuriel" << std::endl;
    std::cout << "Camato, Karl Kristoffer" << std::endl;
    std::cout << "Malapitan, Ryan James" << std::endl;
    std::cout << "Bendol, Trisha Mae" << std::endl << std::endl;

    std::cout << "Version date: 2026-09-27" << std::endl << std::endl;
}

void printLogo() {
    std::cout << R"(
    __        _______ _     ____ ___  __  __ _____   _____ ___     ___  ____  _____ ______   __  _ 
    \ \      / / ____| |   / ___/ _ \|  \/  | ____| |_   _/ _ \   / _ \|  _ \| ____/ ___\ \ / / | |
     \ \ /\ / /|  _| | |  | |  | | | | |\/| |  _|     | || | | | | | | | |_) |  _| \___ \\ V /  | |
      \ V  V / | |___| |__| |__| |_| | |  | | |___    | || |_| | | |_| |  __/| |___ ___) || |   |_|
       \_/\_/  |_____|_____\____\___/|_|  |_|_____|   |_| \___/   \___/|_|   |_____|____/ |_|   (_))" << std::endl << std::endl;
}

void printHelp() {
    std::cout << "help - displays the commands and its description" << std::endl;
    std::cout << "start_marquee - starts the marquee \"animation\"" << std::endl;
    std::cout << "stop_marquee - stops the marquee \"animation\"" << std::endl;
    std::cout << "set_text - accepts a text input and displays it as a marquee" << std::endl;
    std::cout << "set_speed - sets the marquee animation refresh in milliseconds" << std::endl;
    std::cout << "exit - terminates the console" << std::endl;
}

// Runs on its own thread once start_marquee is called.
// Draws on a RESERVED ROW ABOVE the current cursor position (where the
// user is typing), using ANSI "save cursor / move up / clear / restore
// cursor" so the animation never collides with the Command> prompt line.
void marqueeLoop() {
    const int windowWidth = 40;
    const char spinnerFrames[4] = { '|', '/', '-', '\\' };
    // Foreground color codes to cycle through: red, yellow, green, cyan, blue, magenta
    const int colorCodes[6] = { 31, 33, 32, 36, 34, 35 };
    int frameCounter = 0;

    while (marqueeRunning) {
        std::string text;
        int speed;
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            text = savedText;
            speed = marqueeSpeedMs;
        }

        std::string padded = text + std::string(windowWidth, ' ');
        size_t len = padded.size();

        for (size_t i = 0; i < len && marqueeRunning; ++i) {
            std::string frame = padded.substr(i) + padded.substr(0, i);
            if (frame.size() > (size_t)windowWidth) {
                frame = frame.substr(0, windowWidth);
            }

            char spinner = spinnerFrames[frameCounter % 4];
            int color = colorCodes[frameCounter % 6];
            frameCounter++;

            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "\x1b[s"                      // save cursor (user's current typing spot)
                           << "\x1b[1A"                      // move up one line to the animation row
                           << "\x1b[2K"                       // clear that row completely
                           << "\r" << spinner << " \x1b[" << color << "m[" << frame << "]\x1b[0m " << spinner
                           << "\x1b[u"                        // restore cursor back to the prompt row
                           << std::flush;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(speed));

            {
                std::lock_guard<std::mutex> lock(stateMutex);
                speed = marqueeSpeedMs;
            }
        }
    }

    // Clear the reserved animation row once stopped.
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "\x1b[s" << "\x1b[1A" << "\x1b[2K" << "\x1b[u" << std::flush;
    }
}

// Called from the MAIN thread right before it prints a command's response.
// If the marquee is running, this wipes the currently-reserved animation
// row first -- otherwise, as soon as this new output shifts the cursor
// down, the old frame line gets orphaned as permanent scrollback instead
// of being reused/cleared by the marquee thread.
void clearMarqueeRowIfRunning() {
    if (!marqueeRunning) return;
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "\x1b[s" << "\x1b[1A" << "\x1b[2K" << "\x1b[u" << std::flush;
}

int main() {
    enableAnsiSupport();

    printLogo();
    printHeader();

    bool running = true;
    std::string line;

    while (running) {
        std::cout << "Command> ";
        std::getline(std::cin, line);

        if (!std::cin) {
            break;
        }

        // Wipe any stale animation frame BEFORE printing this command's
        // response, so we don't leave a frozen line behind in scrollback.
        clearMarqueeRowIfRunning();

        std::string command;
        std::string argument;

        size_t spacePos = line.find(' ');
        if (spacePos == std::string::npos) {
            command = line;
        } else {
            command = line.substr(0, spacePos);
            argument = line.substr(spacePos + 1);
        }

        if (command == "help") {
            printHelp();
            std::cout << std::endl;
        }
        else if (command == "set_text") {
            // NOTE: stub for now -- just saves the text.
            if (argument.empty()) {
                std::cout << "Error: set_text requires a text argument." << std::endl << std::endl;
            } else {
                savedText = argument;
                std::cout << "Text saved for marquee: " << savedText << std::endl << std::endl;
            }
        }
        else if (command == "set_speed") {
            if (argument.empty()) {
                std::cout << "Error: set_speed requires a numeric value in milliseconds." << std::endl << std::endl;
            } else {
                try {
                    int newSpeed = std::stoi(argument);
                    if (newSpeed <= 0) {
                        std::cout << "Error: speed must be greater than 0 ms." << std::endl << std::endl;
                    } else {
                        std::lock_guard<std::mutex> lock(stateMutex);
                        marqueeSpeedMs = newSpeed;
                        std::cout << "Marquee speed set to " << marqueeSpeedMs << " ms." << std::endl << std::endl;
                    }
                } catch (const std::exception&) {
                    std::cout << "Error: invalid speed value." << std::endl << std::endl;
                }
            }
        }
        else if (command == "start_marquee") {
            bool hasText;
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                hasText = !savedText.empty();
            }

            if (!hasText) {
                std::cout << "Error: No text has been set." << std::endl << std::endl;
            }
            else if (marqueeRunning) {
                std::cout << "Error: Marquee is already running." << std::endl << std::endl;
            }
            else {
                marqueeRunning = true;
                // Print one blank line to reserve as the animation row, then
                // an extra newline so the cursor lands on the row BELOW it --
                // that row is where "Command> " and your typing will live.
                std::cout << "Marquee started." << std::endl << std::endl;
                marqueeThread = std::thread(marqueeLoop);
            }
        }
        else if (command == "stop_marquee") {
            if (!marqueeRunning) {
                std::cout << "Error: Marquee is already stopped." << std::endl << std::endl;
            }
            else {
                
            }
        }
        else if (command == "exit") {
            if (marqueeRunning) {
                marqueeRunning = false;
                if (marqueeThread.joinable()) {
                    marqueeThread.join();
                }
            }
            std::cout << "Terminating console..." << std::endl << std::endl;
            running = false;
        }
        else if (command.empty()) {
            continue;
        }
        else {
            std::cout << "Error: \"" << command << "\" is not recognized as a command." << std::endl << std::endl;
        }
    }

    return 0;
}