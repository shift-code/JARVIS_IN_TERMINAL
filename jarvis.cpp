#include "headers/jarvis.hpp"


std::string toLowerText(std::string s){
for(char& c : s){
	c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}
return s;
}


int main(){
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

std::cout<<"===== JARVIS IS ACTIVATED =====\n";
std::cout<<"Hello I am Jarvis,I am listening...\n";
speak("Hello I am Jarvis,I am listening");

while(1){
	std::string input = listenForVoiceInput();
	if(input.empty()){
		continue;
	}

	std::cout<<">You: ";

	std::string low = toLowerText(input);
	if(low.empty()){
		continue;
	}

	if(low == "exit" || low == "stop"){
		std::cout<<">End!!"<<std::endl;
		speak("Jarvis is end his work,goodbye");
		break;
	}


	std::cout<<">Thinking..."<<std::endl;
	json ans = askAI(low);

	std::string type = ans.value("type","chat");
	std::string target = ans.value("target","");
	std::string reply = ans.value("reply","");
	std::string content = ans.value("content","");

	if(!reply.empty()){
		std::cout<<">Jarvis: "<<reply<<std::endl;
		speak(reply);
	}

	if(type == "open" && !target.empty()){
		openInWindows(target);
	}
	else if(type == "list_files"){
		std::string dir = target.empty() ? "." : target;
		listFiles(dir);
	}
	else if(type == "write_file" && !target.empty()){
		writeFile(target,content);
	}
	else if(type == "read_file" && !target.empty()){
		readFile(target);
	}
	else if(type == "delete_file" && !target.empty()){
		deleteFile(target);
	}

}
return 0;
}
