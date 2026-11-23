#include "headers/jarvis.hpp"

json askAI(const std::string& prompt){
	httplib::Client cli("http://localhost:11434");

	std::string sys = 
	"You are Jarvis, your creator is Vahe. Always reply ONLY in JSON format:\n"
        "{\"type\": \"chat or open\", \"target\": \"app name or url\", \"reply\": \"short answer\"}\n"
        "Rules:\n"
        "- If user wants to open a site or search (e.g. 'open google and find cats'), "
        "set type='open' and put full URL in target (like https://www.google.com/search?q=cats).\n"
        "- If user wants to open a Windows app (like notepad, calc, chrome, mspaint, explorer), "
        "set type='open' and put its executable name in target (like notepad.exe, calc.exe, chrome.exe).\n"
        "- For normal questions, set type='chat', target='', and answer in reply.";
	
	json body{
		{"system",sys},
		{"prompt",prompt},
		{"model","llama3"},
		{"format","json"},
		{"stream",false}	
	};

	auto res = cli.Post("/api/generate",body.dump(),"application/json");

	if(res && res->status == 200){
		try{
			json outer = json::parse(res->body);
			std::string aiText = outer["response"];
			return json::parse(aiText);
		}
		catch(...){
			std::cout<<"JSON parsing error!!!"<<std::endl;
		}
	}
	return {
		{"type","chat"},
		{"target",""},
		{"reply","Sorry,I have an error!"}
	};
}
