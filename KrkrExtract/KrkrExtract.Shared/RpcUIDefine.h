#pragma once

#include <stdint.h>

enum class LogLevel : uint32_t
{
	LOG_DEBUG = 0,
	LOG_INFO = (LOG_DEBUG + 1),
	LOG_WARN = (LOG_INFO + 1),
	LOG_ERROR = (LOG_WARN + 1),
	LOG_OK    = (LOG_ERROR + 1)
};


