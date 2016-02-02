/*
 * big_pipe_program.cxx
 * 
 * Copyright 2016 olegartys <olegartys@olegartys-HP-Pavilion-15-Notebook-PC>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

using namespace std;
/**
 * Split function
 */
std::vector<std::string>& split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

/**
 * Trim functions 
 */
// trim from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

char *convert(const std::string & s) {
   char *pc = new char[s.size()+1];
   strcpy(pc, s.c_str());
   return pc; 
}

void run_source(int pfd[], const string& cmd, const vector<string>& args) {
	pid_t pid;
	
	switch (pid = fork()) {
	case 0: {
		//printf("%s: PID %d\n", cmd.c_str(), getpid());
		
		close(STDOUT_FILENO);
		dup2(pfd[1], STDOUT_FILENO);
		close(pfd[0]);
		//write(1, "kek", 3);
		//execlp(cmd.c_str(), cmd.c_str(), NULL);
		
		vector<char*> v;
		std::transform(args.begin(), args.end(), std::back_inserter(v), convert); 
		v.push_back(NULL);  	
		execvp(cmd.c_str(), v.data());
	}
	
	default:
		break;
	
	case -1:
		perror("fork");
		exit(1);
	}
}	

void run_dest(int pfd[], const string& cmd, const vector<string>& args) {
	pid_t pid;
	
	switch (pid = fork()) {
	case 0: {
		//printf("%s: PID %d\n", cmd.c_str(), getpid());
	
		close(STDIN_FILENO);
		dup2(pfd[0], STDIN_FILENO);
		close(pfd[1]);
		/*char buf[1024];
		while ((read(0, (void*)buf, 1024)) != -1) {
			//puts(buf);
		}*/
		//execlp(cmd.c_str(), cmd.c_str(), NULL);
		int fd = open("/home/box/result.out", O_WRONLY | O_CREAT | O_TRUNC, 0666);
		close(STDOUT_FILENO);
		dup2(fd, STDOUT_FILENO);
		close(fd);
		vector<char*> v;
		std::transform(args.begin(), args.end(), std::back_inserter(v), convert);   
		v.push_back(NULL); 	
		execvp(cmd.c_str(), v.data());
		//execlp("wc", "wc", NULL);
	}

	default:
		break;
	
	case -1:
		perror("fork");
		exit(1);
	}
}

void run_dest2(int pfd[], int pfd2[], const string& cmd, const vector<string>& args) {
	pid_t pid;
	
	switch (pid = fork()) {
	case 0: {
		//printf("%s: PID %d\n", cmd.c_str(), getpid());
	
		close(STDIN_FILENO);
		dup2(pfd[0], STDIN_FILENO);
		close(pfd[1]);
		
		/*char buf[3];
		read(0, buf, 3);*/
		
		close(STDOUT_FILENO);
		dup2(pfd2[1], STDOUT_FILENO);
		close(pfd2[0]);
		
		//write(1, "kek", 3);
		/*char buf[1024];
		while ((read(0, (void*)buf, 1024)) != -1) {
			write(1, buf, 1024);
		}
		exit(0);*/
		//execlp(cmd.c_str(), cmd.c_str(), NULL);
		vector<char*> v;
		std::transform(args.begin(), args.end(), std::back_inserter(v), convert);  
		v.push_back(NULL);  		
		execvp(cmd.c_str(), v.data());
	}
	
	default:
		break;
	
	case -1:
		perror("fork");
		exit(1);
	}
}

int main(int argc, char **argv)
{
	// Reading string from stdin
	string input;
	getline(cin, input);
	
	// Splitting input by commands
	vector<string> command_tokens;
	split(input, '|', command_tokens);
	size_t tokens_count = command_tokens.size();
	// Trimming command tokens
	for (auto& i: command_tokens) {
		trim(i);
	}
	
	// Getting names and flags from tokens
	vector<string> command_names;
	vector<vector<string>> command_flags;
	int i = 0;
	for (auto& token: command_tokens) {
		vector<string> _t;
		split(token, ' ', _t);
		command_names.push_back(trim(_t[0])); // getting command name
		command_flags.push_back(vector<string>());
		copy(_t.begin(), _t.end(), back_inserter(command_flags[i++])); //getting command flags

		/*cout << command_names[i-1] << ":";
		for (auto t: command_flags[i-1])
			cout << t << " ";
		cout << endl;*/
	}
		
	// Creating vector with fd[2] on each position
	vector<unique_ptr<int>> pipe_vector;
	for (size_t i = 0; i < tokens_count - 1; ++i) {
		auto fd = unique_ptr<int>(new int[2]);
		pipe(fd.get());
		pipe_vector.push_back(move(fd));
	}
	size_t pipe_count = pipe_vector.size();
	
	if (tokens_count == 1) {
		run_dest(new int[2], command_names[0], command_flags[0]);
	} else {


	// Running processes and connect them
	run_source(pipe_vector[0].get(), command_names[0], command_flags[0]);
	for (size_t i = 1; i < tokens_count - 1; ++i) {
		run_dest2(pipe_vector[i-1].get(), pipe_vector[i].get(), command_names[i], command_flags[i]);
		//close(pipe_vector[i-1].get()[0]);
		close(pipe_vector[i-1].get()[1]);
	}
	run_dest(pipe_vector[pipe_count-1].get(), command_names[tokens_count-1], command_flags[tokens_count-1]);
	
	for (auto& i: pipe_vector) {
		close(i.get()[0]);
		close(i.get()[1]);
	}
	
	}
	int pid, status;
	while ((pid = wait(&status)) != -1); // /* pick up all the dead children */ fprintf(stderr, "process %d exits with %d\n", pid, WEXITSTATUS(status));
	//waitpid(-1, NULL, 0);
	
	return 0;
}

