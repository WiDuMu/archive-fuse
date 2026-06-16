// SPDX-License-Identifier: MPL-2.0

#pragma once
#ifndef LOGGING_H
#define LOGGING_H

#include <print>

enum logging_level {
	NONE, // No logging
	ERROR, // Errors only
	INFO, // Default information
	VERBOSE, // All logging
};

// Logging level of the process
inline enum logging_level logging_level = INFO;

/// Log with a level, specify a level:
/// * NONE (Printed even when no output is requested)
/// * ERROR
/// * INFO (Standard information)
/// * VERBOSE (All information should be printed)
template <typename... Args>
void log_level(enum logging_level level, std::format_string<Args...> fmt, Args&&... args) {
    // Check if the message level meets or exceeds the current system log level
    if (level <= logging_level) {
        // Pass the format string and forwarded arguments to std::println
        std::println(fmt, std::forward<Args>(args)...);
    }
}

template <typename... Args>
inline void log_err(std::format_string<Args...> fmt, Args&&... args) {
    log(ERROR, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_info(std::format_string<Args...> fmt, Args&&... args) {
    log(INFO, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_verbose(std::format_string<Args...> fmt, Args&&... args) {
    log(VERBOSE, fmt, std::forward<Args>(args)...);
}

#endif
