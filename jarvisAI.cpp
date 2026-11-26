#include "headers/jarvis.hpp"

json askAI(const std::string& prompt){
	httplib::Client cli("http://localhost:11434");

    std::string sys =
    	"You are Jarvis, your creator is Vahe. Always reply ONLY in JSON format:\n"
	"- Always reply in the same language the user speaks, but keep 'type' and JSON keys in English.\n"
    	"{\"type\": \"chat or open or list_files or write_file or read_file or delete_file\", "
    	"\"target\": \"app name, url, directory, or file name\", "
    	"\"content\": \"text to write into file (only for write_file, otherwise empty)\", "
    	"\"reply\": \"short answer\"}\n"
    	"Rules:\n"
    	"- If user wants to open a site or search, set type='open' and put full URL in target.\n"
    	"- If user wants to open a Windows app, set type='open' and put its executable in target (like notepad.exe, calc.exe).\n"
    	"- If user wants to list files or folders, set type='list_files' and put directory path in target (default '.').\n"
    	"- If user wants to create or write a file, set type='write_file', put file name in target, and put the text to write in content.\n"
    	"- If user wants to read a file, set type='read_file' and put file name in target.\n"
    	"- If user wants to delete a file, set type='delete_file' and put file name in target.\n"
    	"- For normal questions, set type='chat', target='', content='', and answer in reply.";	
	
	json body{
		{"system",sys},
		{"prompt",prompt},
		{"model","qwen3-vl:4b-instruct"},
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
