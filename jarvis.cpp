#include <iostream>
#include <string>
#include <cstdlib>
#include <cctype>
#include "httplib.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

std::string toLowerText(std::string s){
for(char& c : s){
	c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}
return s;
}

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

std::string askOllama(const std::string& prompt){
	httplib::Client cli("http://localhost:11434");

	std::string systemInstruction = 
            "Your name is Jarvis. You are a helpful, smart AI assistant created by Vahe. "
            "Always remember that your name is Jarvis. Keep your answers short, clear, "
            "and suitable for text-to-speech.\n"
	    "[CMD:google] - to open Google\n"
            "[CMD:youtube] - to open YouTube\n"
            "[CMD:terminal] - to open terminal\n"
            "[CMD:files] - to open file manager\n"
            "For normal questions, just answer normally in short text.";

	json rec_body = {
		{"model","llama3"},
		{"prompt",prompt},
		{"system",systemInstruction},
		{"stream",false}
	};

	auto res = cli.Post("/api/generate",rec_body.dump(),"application/json");
	
	if(res && res->status == 200){
		try{
			auto res_json = json::parse(res->body);
			if(res_json.contains("response")){
				return res_json["response"];
			}
		}
		catch(...){
			std::cout<<"AI parsing error!!!"<<std::endl;
		}
	}
	return "Ollama Error!!";

}

void openInWindows(const std::string& target) {
    std::string cmd =
        "/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe "
        "-NoProfile -NonInteractive -WindowStyle Hidden "
        "-Command \"Start-Process '" + target + "'\" >/dev/null 2>&1";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cout << "[Warning: Windows command returned code " << rc << "]\n";
	std::string expCmd = "/mnt/c/Windows/explorer.exe \"" + target + "\"";
        std::system(expCmd.c_str());
    }
}

bool openingApp(const std::string& aiReply) {
    std::string low = toLowerText(aiReply);
    if (low == "youtube" || low == "open youtube" || low.find("cmd:youtube") != std::string::npos) {
        std::cout << "Jarvis: Opening youtube...\n";
        speak("Opening youtube");
        openInWindows("https://www.youtube.com");
        return true;
    }
    if (low == "google" || low == "open google" || low.find("cmd:google") != std::string::npos) {
        std::cout << "Jarvis: Opening google...\n";
        speak("Opening google");
        openInWindows("https://www.google.com");
        return true;
    }
    return false;
}

int main(){
std::cout<<"===== JARVIS IS ACTIVATED =====\n";
std::cout<<"Hello I am Jarvis,ask me anything...\n";
speak("Hello I am Jarvis,ask me anything");

std::string input;

while(1){
	std::cout<<"Write something(exit for end): ";
	std::getline(std::cin,input);

	if(input.empty()){
		continue;
	}

	if(input == "exit" || input == "stop" || input == "Exit" || input == "Stop"){
		std::cout<<"End!!"<<std::endl;
		speak("Jarvis is end his work,goodbye");
		break;
	}
	
	if(openingApp(input)){
		continue;
	}

	std::cout<<"Thinking..."<<std::endl;
	std::string aiReply = askOllama(input);
	if(!openingApp(aiReply)){
		speak(aiReply);
		std::cout<<"Jarvis: "<<aiReply<<std::endl;
	}

}
return 0;
}
