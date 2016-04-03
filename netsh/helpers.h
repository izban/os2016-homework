#ifndef HELPERS_H
#define HELPERS_H

#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

ssize_t write_all(int fd, const void *buf, size_t count) {
	size_t processed = 0;
	while (processed < count) {
		ssize_t writed = write(fd, buf + processed, count - processed);
		if (writed < 0) return -1;
		processed += writed;
	}
	return processed;
}

ssize_t read_line(int fd, void *buf, size_t count) {
	size_t processed = 0;
	char was_delimeter = 0;
	while (!was_delimeter) {
		ssize_t readed = read(fd, buf + processed, count - processed);
		if (readed < 0) return -1; 
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

#endif

