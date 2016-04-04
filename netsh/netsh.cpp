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
#include <sys/epoll.h>
#include <map>
#include <signal.h>
#include <sys/wait.h>

using namespace std;

#define LISTEN_BACKLOG 50
#define MAX_CLIENTS 1000

struct epoll_event events[MAX_CLIENTS];

const int MAX_COMMAND_LEN = 1 << 21;
char buf[MAX_COMMAND_LEN];


int make_nonblocking(int sfd) {
	int flags, s;
	flags = fcntl(sfd, F_GETFL, 0);
	if (flags < 0) {
		error("can't make socket nonblocking");	
		return -1;
	}
	flags |= O_NONBLOCK;
	s = fcntl(sfd, F_SETFL, flags);
	if (s < 0) {
		error("can't make socket nonblocking");
		return -1;
	}
	return 0;
}

void make_self_daemon() {
	pid_t pid_1 = fork();
	if (pid_1 < 0) error("can't fork from initial process"); // throw error on failure
	if (pid_1 > 0) exit(0); // forget about first process
	// now we are not group leader
	pid_t pid_session = setsid();
	if (pid_session < 0) error("can't get new session"); // throw error on failure
	// now we are session leader

	pid_t pid_my = fork();
	if (pid_my < 0) error("can't fork in new session"); // throw error on failure 
	if (pid_my > 0) exit(0); // forget about parent process	
	pid_my = getpid(); // id of this process

	int f = open("/tmp/netsh.pid", O_RDWR | O_CREAT);
	if (f < 0) error("can't open file to write pid");

	char buf_pid[20];
	sprintf(buf_pid, "%d", pid_my);
	int pid_len = strlen(buf_pid);
	if (write_all(f, buf_pid, pid_len + 1) < 0) error("can't write to file");

	if (close(f) < 0) error("can't close file /tmp/netsh.pid");
}

int open_socket(int port) {
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) error("can't create socket");

	struct sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(struct sockaddr_in));
	my_addr.sin_family = AF_INET;

	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(port);

	if (bind(sfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_un)) < 0) error("can't bind");
	if (listen(sfd, LISTEN_BACKLOG) == -1) error("can't start listening");
	return sfd;
}

map<int, int> close_cfd;

void sigchld_handler(int signum, siginfo_t *info, void*) {
	if (signum != SIGCHLD) error("caught unexpected signal");
	int id = info->si_pid;
	waitpid(id, NULL, 0);
	if (close_cfd.count(id)) {
		close(close_cfd[id]);
		close_cfd.erase(id);
	}
}

int main(int argc, char* argv[]) {
	make_self_daemon();

	int port;
	if (argc != 2 || sscanf(argv[1], "%d", &port) != 1) {
		error("usage: ./netsh port");
	}

	int sfd = open_socket(port);

	int epolfd = make_epoll(MAX_CLIENTS);	

	struct sigaction saction;
	saction.sa_sigaction = &sigchld_handler;
	saction.sa_flags = SA_SIGINFO;
	if (sigaction(SIGCHLD, &saction, NULL) < 0) error("can't make SIGCHLD handler");
	

	string started = "Server has been started\n";
	write_all(STDERR_FILENO, started.c_str(), started.length());

	map<int, int> ttype; // map from fd to its type (1 -- sfd, 2 -- socket, 3 -- end of pipe)
	map<int, string> bufs; // buffer with readed info for each fd
	map<int, bool> already_execed; // if socket was execed already
	map<int, int> pipe_write_end; // if socket was execed already, its pipe write end

	function<void(int)> epoll_accept = [&](int sfd) {
		socklen_t peer_addr_size = sizeof(struct sockaddr_in);
		struct sockaddr_in peer_addr;
		int cfd;
		while (1) {
			cfd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addr_size);
			if (!(cfd < 0 && errno == EINTR)) break;
		}

		if (cfd < 0) {
			error("can't accept", 0); 
			return;
		}

		if (make_nonblocking(cfd) < 0) {
			error("can't make nonblocking", 0);
			return;
		}

		function<void(int)> epoll_read = [&](int cfd){
			if (already_execed[cfd]) {
				ssize_t readed;
				int pd = pipe_write_end[cfd];
				while (1) {
					readed = read_eintr(cfd, buf, MAX_COMMAND_LEN);
					if (readed <= 0) break;
					write_all(pd, buf, readed); // TODO: maybe should use epoll
				}
				bool done = 0;
				if (readed < 0) {
					if (errno != EAGAIN) {
						error("some error while reading from socket " + inttostr(errno), 0);
						done = 1;
					}
				}
				if (readed == 0) {
					done = 1;
				}
				if (done) {
					close(pipe_write_end[cfd]);
					my_ctl_delete(epolfd, cfd);
					close(cfd);
					bufs.erase(cfd);
					ttype.erase(cfd);
					already_execed.erase(cfd);
					pipe_write_end.erase(cfd);
				}
			} else {
				ssize_t readed;
				int endlinePos = -1;
				int lastreaded = -1;
				while ((readed = read_eintr(cfd, buf, MAX_COMMAND_LEN)) > 0) {
					lastreaded = readed;
					buf[readed] = 0;
					bufs[cfd] += (string)(buf);
					for (int i = readed - 1; i >= 0; i--) if (buf[i] == '\n') endlinePos = i;
				}
				if (endlinePos == -1) {
					return;
				}

				string command = split(bufs[cfd], '\n')[0];
				write_all(STDOUT_FILENO, (command + "\n").c_str(), (command + "\n").length());
		
				vector<string> commands = split(command, '|');
				int cpipe[2];
				if (pipe(cpipe) < 0) error("can't make first pipe", 0);
		
				pipe_write_end[cfd] = cpipe[1];
				already_execed[cfd] = 1;
				bufs[cfd] = "";
				int cur_stdin = cpipe[0];
				make_nonblocking(cpipe[1]);
				for (int i = 0; i < (int)commands.size(); i++) {
					int pipefd[2] = {0};
					bool first = i == 0;
					bool last = i + 1 == (int)commands.size();
					if (!last && pipe(pipefd) < 0) error("can't make pipe", 0);

					int cpid = fork(); // child write to pipe, right brother reads from pipe
					if (cpid < 0) error("can't fork");
					if (cpid == 0) {
						if (!last && close(pipefd[0]) < 0) error("can't close pipe read end");
						if (dup2(cur_stdin, STDIN_FILENO) < 0) error("error at trying to dup2 changing STDIN");
						close(cur_stdin);
						int fd_stdout = pipefd[1];
						if (last) fd_stdout = cfd;
						if (dup2(fd_stdout, STDOUT_FILENO) < 0) error("error at trying to dup2 changing STDOUT");
						close(fd_stdout);

						vector<string> vct = split(commands[i], ' ');
						string name = vct[0];
						char ** arguments = new char*[(int)vct.size() + 1];
						for (int i = 0; i < (int)vct.size(); i++) {
							arguments[i] = new char[(int)vct[i].length() + 1];
							strcpy(arguments[i], vct[i].c_str());
							arguments[i][vct[i].length()] = 0;
						}
						arguments[vct.size()] = 0;
						if (execvp(name.c_str(), arguments) < 0) error("can't run exec");
					} else {
						close(cur_stdin);
						if (!last) close(pipefd[1]) < 0;
						if (last) close_cfd[cpid] = cfd;
						cur_stdin = pipefd[0];
					}	
				}
				if (endlinePos + 1 < lastreaded) {
					write_all(pipe_write_end[cfd], buf + endlinePos, lastreaded - endlinePos);
				}
			}	
		};
		my_ctl(epolfd, cfd, EPOLL_CTL_ADD, EPOLLIN, epoll_read);
		
		bufs[cfd] = "";
		ttype[cfd] = 2;
		already_execed[cfd] = false;
	};
	my_ctl(epolfd, sfd, EPOLL_CTL_ADD, EPOLLIN, epoll_accept);

	ttype[sfd] = 1;
	while (1) {
		int nfds = epoll_wait(epolfd, events, MAX_CLIENTS, -1);
		if (nfds < 0) {
			if (errno == EINTR) continue;
			error("error after epoll_wait", 1); // can drop now
			continue;
		}

		for (int n = 0; n < nfds; n++) {
			int curfd = ((my_epoll_data*)events[n].data.ptr)->fd;
			int ctype = ttype[curfd];
			if (ctype == 0) error("invalid epoll type");
			((my_epoll_data*)events[n].data.ptr)->f(curfd);
			if (!ttype.count(curfd)) delete (my_epoll_data*)events[n].data.ptr; // if process is ended then it is deleted from all maps
		}
	}	
}

