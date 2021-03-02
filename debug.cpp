#include <fstream>
#include <iostream>

void log_str(std::string str) {
	std::ofstream log_file;
	log_file.open("log.txt", std::fstream::app);
	log_file << str << std::endl;
	log_file.close();
}
