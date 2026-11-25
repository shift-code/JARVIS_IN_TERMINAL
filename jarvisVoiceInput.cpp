#include "headers/jarvisVoiceInput.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

namespace {

std::string trimText(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string readTextFile(const std::string& filePath) {
    std::ifstream inFile(filePath);
    if (!inFile.is_open()) {
        return "";
    }

    std::string result;
    std::string line;
    while (std::getline(inFile, line)) {
        if (!result.empty()) {
            result += " ";
        }
        result += line;
    }
    inFile.close();
    return trimText(result);
}

} // namespace

std::string listenForVoiceInput(int maxSeconds) {
    if (maxSeconds <= 0) {
        maxSeconds = 5;
    }

    const std::string wavPath = "/tmp/jarvis_voice_input.wav";
    const std::string txtBasePath = "/tmp/jarvis_voice_input";
    const std::string txtPath = txtBasePath + ".txt";

    // Հեռացնում ենք նախորդ արդյունքի ֆայլը (եթե կար), որ հին տեքստը չկարդա
    std::remove(txtPath.c_str());

    // 1. Ձայնագրում ենք խոսափողից (16kHz, 16-bit mono PCM WAV)
    std::string recordCmd =
        "arecord -q -d " + std::to_string(maxSeconds) +
        " -f S16_LE -r 16000 -c 1 \"" + wavPath + "\" >/dev/null 2>&1";

    int recStatus = std::system(recordCmd.c_str());
    if (recStatus != 0) {
        return "";
    }

    // 2. Ձայնը վերածում ենք տեքստի (փոխարինիր քո whisper-cli-ի և մոդելի ուղիներով)
    std::string sttCmd =
        "whisper-cli -m /opt/whisper/models/ggml-base.en.bin -f \"" + wavPath +
        "\" -nt -otxt -of \"" + txtBasePath + "\" >/dev/null 2>&1";

    int sttStatus = std::system(sttCmd.c_str());
    if (sttStatus != 0) {
        return "";
    }

    // 3. Կարդում և վերադարձնում ենք տեքստը
    return readTextFile(txtPath);
}
