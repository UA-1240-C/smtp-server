#pragma once

#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <csignal>
#include <cstdlib>
#include <iostream>

class SignalHandler {
public:
    static void setupSignalHandlers();
    static void handleSignal(int signal);
};

#endif  // SIGNAL_HANDLER_H
