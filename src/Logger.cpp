///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Logger.h"
INITIALIZE_EASYLOGGINGPP
using namespace el;
void Log::Init() {
    Configurations Conf;
    Conf.setToDefault();
    std::string DFormat("%datetime{[%d/%M/%y %H:%m:%s]} %fbase:%line [%level] %msg");
    Conf.setGlobally(ConfigurationType::Format, "%datetime{[%d/%M/%y %H:%m:%s]} [%level] %msg");
    Conf.setGlobally(ConfigurationType::LogFlushThreshold, "2");
    Conf.set(Level::Verbose,ConfigurationType::Format, DFormat);
    Conf.set(Level::Debug,ConfigurationType::Format, DFormat);
    Conf.set(Level::Trace,ConfigurationType::Format, DFormat);
    Conf.set(Level::Fatal,ConfigurationType::Format, DFormat);
    Conf.setGlobally(ConfigurationType::Filename, "Launcher.log");
    Conf.setGlobally(ConfigurationType::MaxLogFileSize, "7340032");
    Loggers::reconfigureAllLoggers(Conf);
    Loggers::addFlag(LoggingFlag::DisableApplicationAbortOnFatalLog);
    Loggers::addFlag(LoggingFlag::HierarchicalLogging);
    Loggers::setLoggingLevel(Level::Global);
}
