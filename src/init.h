#ifndef EPIC_INIT_H
#define EPIC_INIT_H

#include "spdlog/sinks/basic_file_sink.h"
#include "utils/cpptoml.h"
#include "utils/cxxopts.hpp"

#include "config.h"
#include "utils/file_utils.h"

enum {
    COMMANDLINE_INIT_FAILURE = 1,
    LOG_INIT_FAILURE         = 2,
};

void Init(int argc, char* argv[]);

void LoadConfigFile();

void SetupCommandline(cxxopts::Options& options);
void ParseCommandLine(int argc, char* argv[], cxxopts::Options& options);

void UseFileLogger(const std::string& path, const std::string& filename);
void InitLogger();


#endif // EPIC_INIT_H
