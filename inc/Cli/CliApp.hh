#pragma once

namespace cli {

/**
 * @brief
 * Parses command-line arguments, extracts global options, and dispatches to the appropriate subcommand handler.
 */
class CliApp {
public:
    static int Run(int argc, char *pArgv[]);
};

} // namespace cli
