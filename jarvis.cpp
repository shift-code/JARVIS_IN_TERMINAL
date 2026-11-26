#include "headers/jarvis.hpp"


std::string toLowerText(std::string s){
for(char& c : s){
	c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}
return s;
}

std::string normalizeText(const std::string& s){
	std::string tmp;
	for(unsigned char c : s){
		if(std::isalnum(c)) tmp += static_cast<char>(std::tolower(c));
		else tmp += ' ';
	}
	std::istringstream iss(tmp);
	std::string word, out;
	while(iss >> word){
		if(!out.empty()) out += ' ';
		out += word;
	}
	return out;
}

bool isExitCommand(const std::string& norm){
	std::istringstream iss(norm);
	std::string w;
	int count = 0;
	bool hit = false;
	while(iss >> w){
		count++;
		if(w == "exit" || w == "quit" || w == "stop" || w == "goodbye") hit = true;
	}
	return hit && count <= 3;
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

	std::cout<<">You: "<<input<<std::endl;

	std::string norm = normalizeText(input);
	if(norm.empty()){
		continue;
	}


	if(isExitCommand(norm)){
		std::cout<<">End!!"<<std::endl;
		speak("Jarvis is end his work,goodbye");
		break;
	}
	if(needsScreen(input)){
		std::cout<<">Looking at your screen..."<<std::endl;
		std::string answer = askAboutScreen(input);
		std::cout<<">Jarvis: "<<answer<<std::endl;
		speak(answer);
		continue;
	}


	std::cout<<">Thinking..."<<std::endl;
	json ans = askAI(input);

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
