#include "headers/jarvisSound.hpp"

void speak(const std::string& text){
    std::string safeText = text;
    for (char& c : safeText) {
        if (c == '"' || c == '\n' || c == '\r') c = ' ';
    }

    std::string cmd =
        "echo \"" + safeText + "\" | "
        "piper --model \"$HOME/piper-voices/en_US-lessac-medium.onnx\" "
        "--output_file /tmp/jarvis.wav 2>/dev/null && "
        "WIN_PATH=\"$(wslpath -w /tmp/jarvis.wav)\" && "
        "/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe "
        "-NoProfile -NonInteractive -WindowStyle Hidden "
        "-Command \"(New-Object Media.SoundPlayer '$WIN_PATH').PlaySync()\" "
        ">/dev/null 2>&1";

    std::system(cmd.c_str());
}
