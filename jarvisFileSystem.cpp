#include "headers/jarvis.hpp"

namespace fs = std::filesystem;

void listFiles(const std::string& path){
	if(!fs::exists(path) || !fs::is_directory(path)){
		std::cout<<"Error: No such file or directory!!"<<std::endl;
		return;
	}
	std::cout<<"---- Files in ["<<path<<"] directory ----"<<std::endl;
	for(const auto& f : fs::directory_iterator(path)){
		std::string type = fs::is_directory(path) ? "[DIR]" : "[FILE]";
		std::cout<<type<<" "<<f.path().filename().string()<<std::endl;
	}
	std::cout<<"---------------------\n";
}

void writeFile(const std::string& fileName,const std::string& content){
	std::ofstream file(fileName);
	if(!file.is_open()){
		std::cout<<"Error: Could not create file!!\n";
		return;
	}
	file<<content;
	file.close();
	std::cout<<"File saved: ["<<fileName<<"]\n";
}

void readFile(const std::string& fileName){
	std::ifstream file(fileName);
	if(!file.is_open()){
		std::cout<<"Error: Could not read file!!\n";
		return;
	}
	std::cout<<"---- Content of ["<<fileName<<"] \n";
	std::string line;
	while(getline(file,line)){
		std::cout<<line<<"\n";
	}
	file.close();
	std::cout<<"---------------------\n";
}

void deleteFile(const std::string& fileName){
	if(!fs::exists(fileName)){
		std::cout<<"Error: File does not exist: ["<<fileName<<"]\n";
	}
	fs::remove(fileName);
	std::cout<<"File deleted: "<<fileName<<"\n";
}
