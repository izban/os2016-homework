#include "helpers.h"

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
