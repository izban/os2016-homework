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
			return 0; // end of input, everything is fine
		}
		ssize_t already_writed = 0;
		while (already_writed < readed) {
			ssize_t writed = write(STDOUT_FILENO, buf + already_writed, readed - already_writed);
			if (writed == -1) {
				return 1; // some error happened
			}
			already_writed += writed;
		}
	}
}
