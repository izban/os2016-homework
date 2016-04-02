#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "helpers.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <iostream>
#include <netinet/in.h>
#include <string>

using namespace std;

#define LISTEN_BACKLOG 50

void error(string s) {
	s += "\n";
	write_all(STDERR_FILENO, s.c_str(), s.length());
	exit(1);
}

int main(int argc, char* argv[]) {
	pid_t pid_1 = fork();
	if (pid_1 < 0) error("can't fork from initial process"); // throw erro on failure
	if (pid_1 > 0) return 0; // forget about first process
	// now we are not group leader
	pid_t pid_session = setsid();
	if (pid_session < 0) error("can't get new session"); // throw error on failure
	// now we are session leader

	pid_t pid_my = fork();
	if (pid_my < 0) error("can't fork in new session"); // throw error on failure 
	if (pid_my > 0) return 0; // forget about parent process	
	pid_my = getpid(); // id of this process

	int f = open("/tmp/netsh.pid", O_RDWR | O_CREAT);
	if (f < 0) error("can't open file to write pid");

	char buf_pid[20];
	sprintf(buf_pid, "%d", pid_my);
	int pid_len = strlen(buf_pid);
	if (write_all(f, buf_pid, pid_len) < 0) error("can't write to file");

	if (close(f) < 0) error("can't close file");

	int port;
	if (argc != 2 || sscanf(argv[1], "%d", &port) != 1) {
		error("usage: ./netsh port");
	}

	int sfd;
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) error("can't create socket");

	struct sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(struct sockaddr_un));
	my_addr.sin_family = AF_INET;
	//string addr = "localhost:" + inttostr(port);
	//strncpy(my_addr.sun_path, addr.c_str(), addr.length());	
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(port);

	if (bind(sfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_un)) < 0) error("can't bind");
	if (listen(sfd, LISTEN_BACKLOG) == -1) error("can't start listening");

	string started = "Server has been started\n";
	write_all(STDERR_FILENO, started.c_str(), started.length());
	while (1) {
		socklen_t peer_addr_size = sizeof(struct sockaddr_in);
		struct sockaddr_in peer_addr;
		int cfd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addr_size);
		if (cfd < 0) error("can't accept"); // TODO: don't die
		const int MAX_COMMAND_LEN = 1 << 21;
		char buf[MAX_COMMAND_LEN];
		ssize_t readed = read_line(cfd, buf, MAX_COMMAND_LEN);
		if (readed < 0) {
			string errmsg = "incorrect command syntax, no endline found\n";
			write_all(STDOUT_FILENO, errmsg.c_str(), errmsg.length());
			sendto(sfd, errmsg.c_str(), errmsg.length(), 0, (struct sockaddr *)&peer_addr, peer_addr_size);
		} else {
			write_all(STDOUT_FILENO, buf, readed);
			
			buf[readed] = 0;
			string s = split(buf, '\n')[0];
			vector<string> commands = split(s, '|'); // TODO: can be bad at <<echo "|">>
			
			int cur_stdin = -1;
			for (int i = 0; i < (int)commands.size(); i++) {
				int pipefd[2];
				bool first = i == 0;
				bool last = i + 1 == (int)commands.size();
				if (!last && pipe(pipefd) < 0) error("can't make pipe");

				int cpid = fork(); // child write to pipe, right brother reads from pipe
				if (cpid < 0) error("can't fork");
				if (cpid == 0) {
					if (!last && close(pipefd[0]) < 0) error("can't close pipe read end");
					if (!first && dup2(cur_stdin, STDIN_FILENO) < 0) error("error at trying to dup2 changing STDIN");
					int fd_stdout = pipefd[1];
					if (last) fd_stdout = cfd;
					if (dup2(fd_stdout, STDOUT_FILENO) < 0) error("error at trying to dup2 changing STDOUT");

					vector<string> vct = split(commands[i], ' ');
					string name = vct[0];
					char ** arguments = new char*[(int)vct.size() + 1];
					//cerr << commands[i] << endl;
					for (int i = 0; i < (int)vct.size(); i++) {
						//cerr << vct[i] << " ||| ";
						arguments[i] = new char[(int)vct[i].length() + 1];
						strcpy(arguments[i], vct[i].c_str());
						arguments[i][vct[i].length()] = 0;
					}
					//cerr << endl;
					arguments[vct.size()] = 0;
					if (execvp(name.c_str(), arguments) < 0) error("can't run exec");
				} else {
					if (!first && close(cur_stdin) < 0) error("can't close old stdin");
					if (!last && close(pipefd[1]) < 0) error("can't close pipe write end");
					cur_stdin = pipefd[0];
				}	
			}
			// send message	
		}
		close(cfd);
	}	
}

