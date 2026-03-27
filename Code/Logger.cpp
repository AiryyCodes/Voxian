#include "Logger.h"

#include <spdlog/spdlog.h>

void Logger::Init()
{
    spdlog::set_pattern("[%T] [Thread: %t] [%l]: %v");
}
