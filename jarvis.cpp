#include "headers/jarvis.hpp"


std::string toLowerText(std::string s){
for(char& c : s){
	c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}
return s;
}


int main(){
std::cout<<"===== JARVIS IS ACTIVATED =====\n";
std::cout<<"Hello I am Jarvis,ask me anything...\n";
speak("Hello I am Jarvis,ask me anything");

std::string input;

while(1){
	std::cout<<">Write something(exit for end): ";
	std::getline(std::cin,input);
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

	if(!reply.empty()){
		std::cout<<">Jarvis: "<<reply<<std::endl;
		speak(reply);
	}

	if(type == "open" && !target.empty()){
		openInWindows(target);
	}

}
return 0;
}
