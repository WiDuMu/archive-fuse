#pragma once
#ifndef LOGGING_H
#define LOGGING_H

// Used in the log macro
#include <print> // IWYU pragma: keep

enum logging_level {
	NONE, // No logging
	ERROR, // Errors only
	INFO, // Default information
	VERBOSE, // All logging
};

inline enum logging_level logging_level = INFO;

#define log(level, ...)            \
	if (level <= logging_level) {  \
		std::println(__VA_ARGS__); \
	}

#endif
