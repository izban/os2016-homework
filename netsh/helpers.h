#ifndef HELPERS_H
#define HELPERS_H

#include <unistd.h>
#include <sstream>
#include <string>

using namespace std;

ssize_t write_all(int fd, const void *buf, size_t count);
ssize_t read_line(int fd, void *buf, size_t count);
string inttostr(int x);

#include "helpers.cpp"

#endif
