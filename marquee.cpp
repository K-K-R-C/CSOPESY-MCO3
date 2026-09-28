#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <fstream>
#include <map>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#endif

// ---- Shared state between main thread and marquee thread ----
std::mutex stateMutex;   // protects savedText / marqueeSpeedMs / windowWidth
std::mutex coutMutex;    // protects console output from interleaving
std::atomic<bool> marqueeRunning(false);

std::string savedText;
int marqueeSpeedMs = 300;   // refresh rate (config: speed_ms)
int windowWidth = 40;       // marquee width (config: width)
int pollingMs = 10;         // keyboard polling interval (config: polling_ms)
bool autoStart = false;     // config: autostart

std::thread marqueeThread;

// ------------------------------------------------------------------
// Config loading
// ------------------------------------------------------------------
static std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) a++;
    while (b > a && std::isspace((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
}

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

// Parses a whole-string integer. Returns false on garbage like "10abc".
static bool parseInt(const std::string& s, int& out) {
    try {
        size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size()) return false;
        out = v;
        return true;
    } catch (...) {
        return false;
    }
}

std::map<std::string, std::string> loadConfig(const std::string& path) {
    std::map<std::string, std::string> cfg;
    std::ifstream f(path);
    if (!f.is_open()) return cfg;
    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);                       // also strips '\r'
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = toLower(trim(line.substr(0, eq)));
        std::string val = trim(line.substr(eq + 1));
        cfg[key] = val;
    }
    return cfg;
}

// Applies config values with validation; invalid values keep the defaults.
void applyConfig(const std::string& path) {
    auto cfg = loadConfig(path);
    if (cfg.empty()) {
        std::cout << "[config] \"" << path << "\" not found or empty. Using defaults." << std::endl;
        return;
    }

    int v;
    if (cfg.count("text")) savedText = cfg["text"];

    if (cfg.count("speed_ms")) {
        if (parseInt(cfg["speed_ms"], v) && v > 0) marqueeSpeedMs = v;
        else std::cout << "[config] invalid speed_ms, using " << marqueeSpeedMs << std::endl;
    }
    if (cfg.count("width")) {
        if (parseInt(cfg["width"], v) && v > 0 && v <= 500) windowWidth = v;
        else std::cout << "[config] invalid width, using " << windowWidth << std::endl;
    }
    if (cfg.count("polling_ms")) {
        if (parseInt(cfg["polling_ms"], v) && v > 0) pollingMs = v;
        else std::cout << "[config] invalid polling_ms, using " << pollingMs << std::endl;
    }
    if (cfg.count("autostart")) {
        std::string a = toLower(cfg["autostart"]);
        autoStart = (a == "true" || a == "1" || a == "yes");
    }

    std::cout << "[config] text=\"" << savedText << "\" speed_ms=" << marqueeSpeedMs
              << " width=" << windowWidth << " polling_ms=" << pollingMs
              << " autostart=" << (autoStart ? "true" : "false") << std::endl << std::endl;
}

// Console helpers
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
    std::cout << "Bendol, Trisha Mae" << std::endl;
    std::cout << "Camato, Karl Kristoffer" << std::endl;
    std::cout << "Malapitan, Ryan James" << std::endl;
    std::cout << "Marcos, Alain Zuriel" << std::endl << std::endl;

    std::cout << "Version date: 2026-09-28" << std::endl << std::endl;
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

// Marquee thread
void marqueeLoop() {
    const char spinnerFrames[4] = { '|', '/', '-', '\\' };
    const int colorCodes[6] = { 31, 33, 32, 36, 34, 35 };
    int frameCounter = 0;
    size_t i = 0;

    while (marqueeRunning) {
        std::string text;
        int speed, width;
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            text = savedText;
            speed = marqueeSpeedMs;
            width = windowWidth;
        }

        if (text.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(speed));
            continue;
        }

        std::string padded = text + std::string(width, ' ');
        size_t len = padded.size();
        size_t position = i % len;

        std::string frame = padded.substr(position) + padded.substr(0, position);
        if (frame.size() > (size_t)width) frame = frame.substr(0, width);

        char spinner = spinnerFrames[frameCounter % 4];
        int color = colorCodes[frameCounter % 6];
        frameCounter++;

        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "\x1b[s"
                      << "\x1b[1A"
                      << "\x1b[2K"
                      << "\r" << spinner << " \x1b[" << color << "m[" << frame << "]\x1b[0m " << spinner
                      << "\x1b[u"
                      << std::flush;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(speed));
        i++;
    }

    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "\x1b[s" << "\x1b[1A" << "\x1b[2K" << "\x1b[u" << std::flush;
    }
}

void clearMarqueeRowIfRunning() {
    if (!marqueeRunning) return;
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "\x1b[s" << "\x1b[1A" << "\x1b[2K" << "\x1b[u" << std::flush;
}

void startMarquee() {
    marqueeRunning = true;
    marqueeThread = std::thread(marqueeLoop);
}

void stopMarquee() {
    marqueeRunning = false;
    if (marqueeThread.joinable()) marqueeThread.join();
}

// Input: polled keyboard on Windows (polling_ms), getline elsewhere
// Returns false on EOF / Ctrl+C. Does NOT print the final newline;
// main does that after clearing the marquee row.
bool readCommand(std::string& out) {
    out.clear();
#ifdef _WIN32
    while (true) {
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 0 || ch == 224) { _getch(); continue; }   // arrow/function keys
            if (ch == 3) return false;                          // Ctrl+C
            if (ch == '\r') return true;
            if (ch == 8) {                                      // backspace
                if (!out.empty()) {
                    out.pop_back();
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "\b \b" << std::flush;
                }
            } else if (ch >= 32 && ch < 127) {
                out.push_back((char)ch);
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << (char)ch << std::flush;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(pollingMs));
        }
    }
#else
    std::getline(std::cin, out);
    if (!std::cin) return false;
    return true;
#endif
}

// main
int main() {
    enableAnsiSupport();

    printLogo();
    printHeader();
    applyConfig("config.txt");

    if (autoStart && !savedText.empty()) {
        std::cout << "Marquee started (autostart)." << std::endl << std::endl;
        startMarquee();
    }

    bool running = true;
    std::string line;

    while (running) {
        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Command> " << std::flush;
        }

        if (!readCommand(line)) break;

#ifdef _WIN32
        // Clear the marquee row while the cursor is still on the prompt line,
        // then move to the next line for the command's output.
        clearMarqueeRowIfRunning();
        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << std::endl;
        }
#else
        clearMarqueeRowIfRunning();
#endif

        line = trim(line);
        std::string command, argument;
        size_t spacePos = line.find(' ');
        if (spacePos == std::string::npos) {
            command = line;
        } else {
            command = line.substr(0, spacePos);
            argument = trim(line.substr(spacePos + 1));
        }

        if (command.empty()) {
            continue;
        }
        else if (command == "help") {
            printHelp();
            std::cout << std::endl;
        }
        else if (command == "set_text") {
            if (argument.empty()) {
                std::cout << "Error: set_text requires a text argument." << std::endl << std::endl;
            } else {
                {
                    std::lock_guard<std::mutex> lock(stateMutex);
                    savedText = argument;
                }
                std::cout << "Text saved for marquee: " << argument << std::endl << std::endl;
            }
        }
        else if (command == "set_speed") {
            int newSpeed;
            if (argument.empty()) {
                std::cout << "Error: set_speed requires a numeric value in milliseconds." << std::endl << std::endl;
            } else if (!parseInt(argument, newSpeed)) {
                std::cout << "Error: invalid speed value." << std::endl << std::endl;
            } else if (newSpeed <= 0) {
                std::cout << "Error: speed must be greater than 0 ms." << std::endl << std::endl;
            } else {
                {
                    std::lock_guard<std::mutex> lock(stateMutex);
                    marqueeSpeedMs = newSpeed;
                }
                std::cout << "Marquee speed set to " << newSpeed << " ms." << std::endl << std::endl;
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
            } else if (marqueeRunning) {
                std::cout << "Error: Marquee is already running." << std::endl << std::endl;
            } else {
                std::cout << "Marquee started." << std::endl << std::endl;
                startMarquee();
            }
        }
        else if (command == "stop_marquee") {
            if (!marqueeRunning) {
                std::cout << "Error: Marquee is already stopped." << std::endl << std::endl;
            } else {
                stopMarquee();
                std::cout << "Marquee stopped." << std::endl << std::endl;
            }
        }
        else if (command == "exit") {
            if (marqueeRunning) stopMarquee();
            std::cout << "Terminating console..." << std::endl << std::endl;
            running = false;
        }
        else {
            std::cout << "Error: \"" << command << "\" is not recognized as a command." << std::endl << std::endl;
        }
    }

    if (marqueeRunning) stopMarquee();
    return 0;
}