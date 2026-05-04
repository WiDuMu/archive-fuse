#pragma once
#ifndef LOGGING_H
#define LOGGING_H

#include <print>

enum logging_level {
    NONE,
    ERROR,
    INFO,
    VERBOSE,
};

enum logging_level logging_level = INFO;

#define log(level, ...) if (level <= logging_level) { std::println(__VA_ARGS__); }

#endif
