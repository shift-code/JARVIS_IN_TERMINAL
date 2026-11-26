#include "headers/jarvisScreen.hpp"

#include "httplib.h"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>

using json = nlohmann::json;

namespace {

// Только ASCII! Скрипт отдаёт JPEG в stdout как base64, на диск ничего не пишет.
const char* PS_EXE = "/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe";
const char* PS_SCRIPT = R"PS(
$ProgressPreference = 'SilentlyContinue'
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type -MemberDefinition '[DllImport("user32.dll")] public static extern bool SetProcessDPIAware();' -Name Dpi -Namespace Win32
[Win32.Dpi]::SetProcessDPIAware() | Out-Null
$b = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
$bmp = New-Object System.Drawing.Bitmap($b.Width, $b.Height)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($b.Location, [System.Drawing.Point]::Empty, $b.Size)
$g.Dispose()
$w = [Math]::Min(__WIDTH__, $b.Width)
$h = [int]($b.Height * $w / $b.Width)
$small = New-Object System.Drawing.Bitmap($bmp, $w, $h)
$codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq 'image/jpeg' }
$ep = New-Object System.Drawing.Imaging.EncoderParameters(1)
$ep.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality, 70L)
$ms = New-Object System.IO.MemoryStream
$small.Save($ms, $codec, $ep)
[Console]::Out.Write([Convert]::ToBase64String($ms.ToArray()))
)PS";

std::string replaceAll(std::string s, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

std::string base64Encode(const std::string& in) {
    static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((in.size() + 2) / 3 * 4);
    size_t i = 0;
    while (i + 2 < in.size()) {
        unsigned v = ((unsigned char)in[i] << 16) | ((unsigned char)in[i + 1] << 8) | (unsigned char)in[i + 2];
        out += T[(v >> 18) & 63]; out += T[(v >> 12) & 63]; out += T[(v >> 6) & 63]; out += T[v & 63];
        i += 3;
    }
    if (i + 1 == in.size()) {
        unsigned v = (unsigned char)in[i] << 16;
        out += T[(v >> 18) & 63]; out += T[(v >> 12) & 63]; out += "==";
    } else if (i + 2 == in.size()) {
        unsigned v = ((unsigned char)in[i] << 16) | ((unsigned char)in[i + 1] << 8);
        out += T[(v >> 18) & 63]; out += T[(v >> 12) & 63]; out += T[(v >> 6) & 63]; out += '=';
    }
    return out;
}

// PowerShell -EncodedCommand ждёт base64 от UTF-16LE (для ASCII: байт + нулевой байт)
std::string psEncoded(const std::string& script) {
    std::string u16;
    u16.reserve(script.size() * 2);
    for (unsigned char c : script) {
        u16 += static_cast<char>(c);
        u16 += '\0';
    }
    return base64Encode(u16);
}

// Скриншот -> base64 JPEG, всё через pipe, без файлов. Пустая строка = ошибка.
std::string captureScreenBase64(int width) {
    std::string script = replaceAll(PS_SCRIPT, "__WIDTH__", std::to_string(width));
    std::string cmd = std::string("\"") + PS_EXE + "\" -NoProfile -NonInteractive -EncodedCommand " +
                      psEncoded(script) + " 2>&1";

    FILE* p = popen(cmd.c_str(), "r");
    if (!p) {
        std::cerr << "[screen] popen failed" << std::endl;
        return "";
    }

    std::string out;
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), p)) > 0) out.append(buf, n);
    pclose(p);

    size_t start = out.find("/9j/");
    if (start == std::string::npos) {
        if (out.empty())
            std::cerr << "[screen] powershell returned nothing (not found? check PS_EXE path)" << std::endl;
        else
            std::cerr << "[screen] powershell output: " << out.substr(0, 400) << std::endl;
        return "";
    }

    size_t end = start;
    while (end < out.size() &&
           (std::isalnum(static_cast<unsigned char>(out[end])) ||
            out[end] == '+' || out[end] == '/' || out[end] == '=')) {
        end++;
    }
    return out.substr(start, end - start);
}

// Убираем markdown и переводы строк, чтобы TTS не читал "звёздочка звёздочка"
std::string cleanForSpeech(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '*' || c == '#' || c == '`') continue;
        out += (c == '\n' || c == '\r') ? ' ' : c;
    }
    return out;
}

bool contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

long msBetween(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b) {
    return static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count());
}

} // namespace

bool needsScreen(const std::string& input) {
    std::string t;
    for (unsigned char c : input) t += static_cast<char>(std::tolower(c));

    static const char* keys[] = {
        "my screen", "the screen", "on screen", "on the screen", "screenshot",
        "look at", "what am i looking", "do you see", "can you see", "what do you see",
        "what is this", "what's this", "what is that", "this error", "read this",
        "what's on", "what is on", "describe this", "what window", "what page",
        "экран", "Экран", "смотри", "идишь"
    };
    for (const char* k : keys)
        if (contains(t, k) || contains(input, k)) return true;
    return false;
}

std::string askAboutScreen(const std::string& question, const std::string& model, int width) {
    auto t0 = std::chrono::steady_clock::now();

    std::string b64 = captureScreenBase64(width);
    if (b64.empty()) return "I could not take a screenshot.";

    auto t1 = std::chrono::steady_clock::now();

    json sys = {
        {"role", "system"},
        {"content",
         "You are Jarvis, a voice assistant. The image is the user's current screen. "
         "Answer the question about it in the same language as the question. "
         "Reply in one or two very short sentences, no markdown, no lists: the answer is read aloud."}
    };
    json usr = {
        {"role", "user"},
        {"content", question},
        {"images", json::array({b64})}
    };
    json body = {
        {"model", model},
        {"stream", false},
        {"keep_alive", "15m"},
        {"messages", json::array({sys, usr})},
        {"options", {{"num_predict", 80}, {"num_ctx", 3072}, {"temperature", 0.2}}}
    };

    httplib::Client cli("http://localhost:11434");
    cli.set_connection_timeout(5, 0);
    cli.set_write_timeout(30, 0);
    cli.set_read_timeout(300, 0);

    auto res = cli.Post("/api/chat", body.dump(), "application/json");

    // Картинка больше не нужна: освобождаем сразу
    b64.clear();
    b64.shrink_to_fit();

    auto t2 = std::chrono::steady_clock::now();
    std::cerr << "[screen] capture " << msBetween(t0, t1) << " ms, model "
              << msBetween(t1, t2) << " ms" << std::endl;

    if (!res) return "I could not reach Ollama.";
    if (res->status != 200) {
        std::cerr << "[screen] Ollama error: " << res->body << std::endl;
        return "The vision model returned an error. Is it installed?";
    }

    json r = json::parse(res->body, nullptr, false);
    if (r.is_discarded() || !r.contains("message")) return "I got a strange answer from the vision model.";

    std::string ans = cleanForSpeech(r["message"].value("content", ""));
    return ans.empty() ? "I could not understand the screen." : ans;
}
