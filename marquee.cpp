#include <iostream>
#include <string>

void printHeader() {
    std::cout << "Welcome to CSOPESY!" << std::endl << std::endl;

    std::cout << "Group developer:" << std::endl;
    std::cout << "Marcos, Alain Zuriel" << std::endl;
    std::cout << "Camato, Karl Kristoffer" << std::endl;
    std::cout << "Malapitan, Ryan James" << std::endl;
    std::cout << "Bendol, Trisha Mae" << std::endl << std::endl;

    std::cout << "Version date: 2026-09-22" << std::endl << std::endl;
}

void printHelp() {
    std::cout << "help - displays the commands and its description" << std::endl;
    std::cout << "start_marquee - starts the marquee \"animation\"" << std::endl;
    std::cout << "stop_marquee - stops the marquee \"animation\"" << std::endl;
    std::cout << "set_text - accepts a text input and displays it as a marquee" << std::endl;
    std::cout << "set_speed - sets the marquee animation refresh in milliseconds" << std::endl;
    std::cout << "exit - terminates the console" << std::endl;
}

int main() {
    printHeader();

    std::string savedText;
    bool marqueeRunning = false;
    int marqueeSpeedMs = 0;

    bool running = true;
    std::string line;

    while (running) {
        std::cout << "Command> ";
        std::getline(std::cin, line);

        // Stop if input stream fails
        if (!std::cin) {
            break;
        }

        // Split the line into the command keyword and the remaining argument text.
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
            if (argument.empty()) {
                std::cout << "Error: set_text requires a text argument." << std::endl << std::endl;
            } 
            else {
                savedText = argument;
                std::cout << "Text saved for marquee: " << savedText << std::endl << std::endl;
            }
        }
        else if (command == "set_speed") {
            if (argument.empty()) {
                std::cout << "Error: set_speed requires a numeric value in milliseconds." << std::endl << std::endl;
            } 
            else {
                try {
                    marqueeSpeedMs = std::stoi(argument);
                    std::cout << "Marquee speed set to " << marqueeSpeedMs << " ms." << std::endl << std::endl;
                } 
                catch (const std::exception&) {
                    std::cout << "Error: invalid speed value." << std::endl << std::endl;
                }
            }
        }
        else if (command == "start_marquee") {

        }
        else if (command == "stop_marquee") {

        }
        else if (command == "exit") {

        }
        else if (command.empty()) {
            // Empty input, just reprompt without an error.
            continue;
        }
        else {
            std::cout << "Error: \"" << command << "\" is not recognized as a command." << std::endl << std::endl;
        }
    }

    return 0;
}