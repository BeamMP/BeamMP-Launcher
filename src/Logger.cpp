///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Logger.h"
INITIALIZE_EASYLOGGINGPP
void Log::Init(int argc, char **argv) {
    el::Helpers::setArgs(argc, argv);
    el::Configurations Conf;
    Conf.setToDefault();
    std::string DFormat("%datetime{[%d/%M/%y %H:%m:%s]} %fbase:%line [%level] %msg");
    Conf.setGlobally(el::ConfigurationType::Format, "%datetime{[%d/%M/%y %H:%m:%s]} [%level] %msg");
    Conf.set(el::Level::Debug,el::ConfigurationType::Format, DFormat);
    Conf.set(el::Level::Trace,el::ConfigurationType::Format, DFormat);
    Conf.set(el::Level::Fatal,el::ConfigurationType::Format, DFormat);
    Conf.setGlobally(el::ConfigurationType::Filename, "Launcher.log");
    Conf.setGlobally(el::ConfigurationType::MaxLogFileSize, "2097152");
    el::Loggers::reconfigureAllLoggers(Conf);
    el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);
    el::Loggers::addFlag(el::LoggingFlag::HierarchicalLogging);
    el::Loggers::setLoggingLevel(el::Level::Global);
}
