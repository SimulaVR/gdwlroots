#include <fstream>
#include <iostream>

void log_str(std::string str) {
	std::ofstream log_file;
	log_file.open("log.txt");
	log_file << str << std::endl;
	log_file.close();
}
