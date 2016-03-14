#include <unistd.h>

int main() {
	const int BUFFER_SIZE = 4096;
	char buf[BUFFER_SIZE];
	while (1) {
		ssize_t readed = read(STDIN_FILENO, buf, BUFFER_SIZE);
		if (readed == -1) {
			return 1; // some error happened
		}
		if (readed == 0) {
			return 0; // end of file, everything is fine
		}
		if (write(STDOUT_FILENO, buf, readed) < 0) {
			return 1; // some error happened
		}
	}
}
