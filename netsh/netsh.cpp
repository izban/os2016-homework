#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "helpers.h"
#include <iostream>

using namespace std;

void error(string s) {
	cerr << s << endl;
	exit(1);
}

int main(int argc, char* argv[]) {
	cerr << "started" << endl;

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
	sprintf(buf_pid, "%d", f);
	int pid_len = strlen(buf_pid);
	if (write_all(f, buf_pid, pid_len) < 0) error("can't write to file");

	if (close(f) < 0) error("can't close file");

	cerr << "server was started" << endl;
	while (1) {}
}

