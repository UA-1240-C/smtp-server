#include "SignalHandler.h"

void SignalHandler::handleSignal(int signal) {
    switch (signal) {
        case SIGINT:
            std::cerr << "\nProgram interrupted by user (SIGINT) with ^C. "
                         "Exiting...\n";
            std::exit(EXIT_SUCCESS);
        default:
            break;
    }
}

void SignalHandler::setupSignalHandlers() { std::signal(SIGINT, handleSignal); }
