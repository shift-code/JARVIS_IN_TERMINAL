#include "headers/jarvisVoiceInput.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int    SAMPLE_RATE       = 16000;
constexpr int    CHUNK_MS          = 100;
constexpr int    CHUNK_SAMPLES     = SAMPLE_RATE * CHUNK_MS / 1000; // 1600
constexpr int    PREROLL_CHUNKS    = 3;     // 300 мс до начала речи, чтобы не резать первое слово
constexpr int    MIN_SPEECH_CHUNKS = 3;     // меньше 300 мс "речи" считаем щелчком и игнорируем
constexpr int    TRAILING_KEEP_MS  = 500;   // сколько тишины оставить в конце записи
constexpr double MIN_THRESHOLD     = 250.0; // минимальный порог громкости (шкала int16)
constexpr double NOISE_MULT        = 3.0;   // порог = шум * 3
constexpr bool   DEBUG_LEVELS      = false; // true, чтобы видеть громкость и подбирать порог

std::string trimText(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string readTextFile(const std::string& filePath) {
    std::ifstream inFile(filePath);
    if (!inFile.is_open()) return "";
    std::string result, line;
    while (std::getline(inFile, line)) {
        if (!result.empty()) result += " ";
        result += line;
    }
    return trimText(result);
}

double rms(const std::vector<int16_t>& v) {
    double sum = 0;
    for (int16_t s : v) sum += static_cast<double>(s) * s;
    return std::sqrt(sum / v.size());
}

void writeWav(const std::string& path, const std::vector<int16_t>& s) {
    std::ofstream f(path, std::ios::binary);
    uint32_t dataSize  = static_cast<uint32_t>(s.size() * 2);
    uint32_t chunkSize = 36 + dataSize;
    uint32_t fmtSize   = 16;
    uint16_t pcm = 1, ch = 1, bits = 16, block = 2;
    uint32_t rate = SAMPLE_RATE, byteRate = SAMPLE_RATE * 2;

    f.write("RIFF", 4); f.write((char*)&chunkSize, 4); f.write("WAVE", 4);
    f.write("fmt ", 4); f.write((char*)&fmtSize, 4);
    f.write((char*)&pcm, 2);  f.write((char*)&ch, 2);
    f.write((char*)&rate, 4); f.write((char*)&byteRate, 4);
    f.write((char*)&block, 2); f.write((char*)&bits, 2);
    f.write("data", 4); f.write((char*)&dataSize, 4);
    f.write((const char*)s.data(), dataSize);
}

// Записывает одну реплику: ждёт начала речи, останавливается после silenceMs тишины.
bool recordUtterance(std::vector<int16_t>& out, int maxSpeechSeconds, int silenceMs) {
    FILE* pipe = popen(
        "exec parec --channels=1 --rate=16000 --format=s16le --latency-msec=50 2>/dev/null",
        "r");
    if (!pipe) return false;

    std::vector<int16_t> chunk(CHUNK_SAMPLES);
    std::deque<std::vector<int16_t>> preroll;

    const int maxChunks = maxSpeechSeconds * 1000 / CHUNK_MS;
    double threshold = MIN_THRESHOLD, noiseSum = 0;
    int calib = 0, silent = 0, speechChunks = 0, totalChunks = 0;
    bool started = false, ok = false;

    while (true) {
        size_t n = fread(chunk.data(), sizeof(int16_t), CHUNK_SAMPLES, pipe);
        if (n < static_cast<size_t>(CHUNK_SAMPLES)) break; // parec умер

        double level = rms(chunk);
        if (DEBUG_LEVELS) std::cerr << "[level] " << (int)level << " / thr " << (int)threshold << "\n";

        // Калибровка шума по первым 300 мс
        if (calib < 3) {
            noiseSum += level;
            if (++calib == 3) threshold = std::max(MIN_THRESHOLD, noiseSum / 3 * NOISE_MULT);
            preroll.push_back(chunk);
            continue;
        }

        bool loud = level > threshold;

        if (!started) {
            if (loud) {
                started = true;
                out.clear();
                for (auto& c : preroll) out.insert(out.end(), c.begin(), c.end());
                preroll.clear();
                out.insert(out.end(), chunk.begin(), chunk.end());
                speechChunks = 1; silent = 0; totalChunks = 0;
            } else {
                preroll.push_back(chunk);
                if (preroll.size() > PREROLL_CHUNKS) preroll.pop_front();
            }
        } else {
            out.insert(out.end(), chunk.begin(), chunk.end());
            totalChunks++;
            if (loud) { speechChunks++; silent = 0; }
            else      { silent += CHUNK_MS; }

            if (silent >= silenceMs || totalChunks >= maxChunks) {
                int trimSamples = std::max(0, silent - TRAILING_KEEP_MS) / CHUNK_MS * CHUNK_SAMPLES;
                if (trimSamples > 0 && static_cast<size_t>(trimSamples) < out.size())
                    out.resize(out.size() - trimSamples);

                if (speechChunks >= MIN_SPEECH_CHUNKS) { ok = true; break; }

                // Это был шум или щелчок, сбрасываем и слушаем дальше
                started = false; out.clear();
                speechChunks = 0; silent = 0; totalChunks = 0;
            }
        }
    }

    pclose(pipe);
    return ok;
}

} // namespace

std::string listenForVoiceInput(int maxSpeechSeconds, int silenceMs) {
    const std::string wavPath = "/tmp/jarvis_voice_input.wav";
    const std::string txtBase = "/tmp/jarvis_voice_input";
    const std::string txtPath = txtBase + ".txt";
    std::remove(txtPath.c_str());

    std::cout << ">Listening..." << std::endl;

    std::vector<int16_t> samples;
    if (!recordUtterance(samples, maxSpeechSeconds, silenceMs)) {
        std::cerr << "[ERROR] audio capture stopped (parec)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1)); // чтобы не крутиться в цикле
        return "";
    }

    writeWav(wavPath, samples);

    std::cout << ">Recognizing..." << std::endl;
    std::string sttCmd =
        "whisper-cli -m \"$HOME/whisper-models/ggml-small.bin\" -f \"" + wavPath +
        "\" -nt -l auto -t 8 -otxt -of \"" + txtBase + "\" >/dev/null 2>&1";
    if (std::system(sttCmd.c_str()) != 0) {
        std::cerr << "[ERROR] whisper-cli failed" << std::endl;
        return "";
    }

    std::string text = readTextFile(txtPath);
    if (text == "[BLANK_AUDIO]" || text == "(silence)" || text == "[silence]") return "";
    return text;
}
