#pragma once

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "spdlog/sinks/rotating_file_sink.h"

#define LOGGER_FORMAT "[%^%l%$] %v"
#define __FILENAME__ (static_cast<const char *>(__FILE__))

#define LOGI(...) spdlog::info(__VA_ARGS__);
#define LOGW(...) spdlog::warn(__VA_ARGS__);
#define LOGE(...) spdlog::error(__VA_ARGS__);
#define LOGD(...) spdlog::debug(__VA_ARGS__);