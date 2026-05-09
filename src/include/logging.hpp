#pragma once
#ifndef LOGGING_H
#define LOGGING_H

// Used in the log macro
#include <print> // IWYU pragma: keep

enum logging_level {
	NONE,
	ERROR,
	INFO,
	VERBOSE,
};

inline enum logging_level logging_level;

#define log(level, ...)            \
	if (level <= logging_level) {  \
		std::println(__VA_ARGS__); \
	}

#endif
