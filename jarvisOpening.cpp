#include "headers/jarvisOpening.hpp"

void openInWindows(const std::string& target) {
    std::string cmd =
        "/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe "
        "-NoProfile -NonInteractive -WindowStyle Hidden "
        "-Command \"Start-Process '" + target + "'\" >/dev/null 2>&1";

    std::system(cmd.c_str());
}
