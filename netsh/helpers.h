#ifndef HELPERS_H
#define HELPERS_H

#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <sys/epoll.h>
#include <functional>
#include <map>

using namespace std;

struct my_epoll_data {
	int fd;
	function<void(int)> f;

	my_epoll_data(int fd, function<void(int)>f) : fd(fd), f(f) {}
};

ssize_t write_all(int, const void*, size_t);

void error(string s, bool drop = true) {
	s += "\n";
	write_all(STDOUT_FILENO, s.c_str(), s.length());
	if (drop) exit(1);
}

ssize_t write_all(int fd, const void *buf, size_t count) {
	size_t processed = 0;
	while (processed < count) {
		ssize_t writed = write(fd, buf + processed, count - processed);
		if (writed < 0) {
			if (errno == EINTR) continue;
			return -1;
		}
		processed += writed;
	}
	return processed;
}

ssize_t read_eintr(int fd, void *buf, size_t count) {
	ssize_t readed;
	while (1) {
		readed = read(fd, buf, count);
		if (readed == -1 && errno == EINTR) continue;
		break;
	}
	return readed;
}

ssize_t read_line(int fd, void *buf, size_t count) {
	size_t processed = 0;
	char was_delimeter = 0;
	while (!was_delimeter) {
		ssize_t readed = read_eintr(fd, buf + processed, count - processed);
		if (readed < 0) {
			return -1;
		}
		if (readed == 0) break;
		int i;
		for (i = 0; i < readed; i++)  was_delimeter |= ((char*)buf)[processed + i] == '\n';
		processed += readed;
	}
	if (!was_delimeter) return -1;
	return processed;
}

string inttostr(int x) {
	stringstream ss;
	string s;
	ss << x;
	ss >> s;
	return s;
}

bool isBadSymbol(char c) {
	return c == '\"' || c == '\'' || c == '`';
}

vector<string> split(string s, char c) {
	s += c;
	vector<string> res;
	string cur;
	char lookForEnd = 0;
	for (int i = 0; i < (int)s.length(); i++) {
		if (lookForEnd != 0) {
			if (s[i] == lookForEnd) lookForEnd = 0;
			cur += s[i];
			continue;
		}
		if (isBadSymbol(s[i])) {
			lookForEnd = s[i];
			cur += s[i];
			continue;
		}
		if (s[i] == c) {
			if (cur != "") {
				if (cur.length() > 2 && isBadSymbol(cur[0]) && isBadSymbol(cur[cur.length() - 1])) cur = cur.substr(1, (int)cur.length() - 2);
				res.push_back(cur);
			}
			cur = "";
		} else cur += s[i];
	}
	return res;
}
int epolfd;
int make_epoll(int MAX_CLIENTS) {
	epolfd = epoll_create(MAX_CLIENTS);
	if (epolfd < 0) error("error at creating epoll");
	return epolfd;
}

map<int, epoll_event> cur_events;

void my_ctl_add(int efd, int fd, uint32_t events, function<void(int)> f) {
	cur_events[fd].events = events;
	cur_events[fd].data.ptr = new my_epoll_data(fd, f);
	//cerr << "add " << efd << " " << fd << endl;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &cur_events[fd]) < 0) error("error at my_ctl_add");
}

void my_ctl_del(int efd, int fd) {
	//cerr << "del " << efd << " " << fd << endl;
	if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL) < 0) {
		//cerr << errno << endl;
		error("can't close epoll " + inttostr(fd), 0);
	}
	delete (my_epoll_data*)cur_events[fd].data.ptr;
	cur_events.erase(fd);
}

void my_ctl_mod(int efd, int fd, uint32_t nw) {
	cur_events[fd].events = nw;
	//cerr << "mod " << efd << " " << fd << endl;
	int res = epoll_ctl(efd, EPOLL_CTL_MOD, fd, &cur_events[fd]);
	if (res < 0) {
		//cerr << errno << endl;
		error("can't mod ctl");
	}
}

#endif

