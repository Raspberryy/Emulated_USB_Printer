#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/usb/g_printer.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#define PRINTER_FILE			"/dev/g_printer0"
#define PRINTJOB_FILE_EXTENSION	"pcl"

int main(int  argc, char *argv[])
{
	/* Open device file for printer gadget. */
	struct pollfd fd[1];
	fd[0].fd = open(PRINTER_FILE, O_RDWR);
	if (fd[0].fd < 0) {
		printf("[!] Error\t%d opening %s\n", fd[0].fd, PRINTER_FILE);
		close(fd[0].fd);
		return(-1);
	}

	fd[0].events = POLLIN | POLLRDNORM;
	int rec = -1;
	char filename[100];
	char file_path[256];
	FILE* cur_job;

	while (1) {
		static char buf[512];
		int bytes_read;
		int retval;

		/* Wait for up to 1 second for data. */
		retval = poll(fd, 1, 1000);

		if (retval && (fd[0].revents & POLLRDNORM)) {

			/* Data received - Create new file if needed */
			if(rec == -1) {
				time_t     now;
				struct tm  ts;

				// Build file name
				time(&now);
				ts = *localtime(&now);
				strftime(filename, sizeof(filename), "%Y-%m-%d-%H-%M-%S", &ts);
				snprintf(file_path, sizeof(file_path), "%s.pcl", filename);

				// Create PCL file
				cur_job = fopen(file_path, "w");
				if(cur_job == NULL){
						printf("[!] Error\tOpening %s. Exiting\n", file_path);
						return -1;
				}
				rec = 0;
			}

			/* Read data from printer gadget driver. */
			bytes_read = read(fd[0].fd, buf, 512);

			if (bytes_read < 0) {
				printf("[!] Error\t%d reading from %s\n",
						fd[0].fd, PRINTER_FILE);
				close(fd[0].fd);
				return(-1);
			} else if (bytes_read > 0) {
				rec = 1;
				fwrite(buf, 1, bytes_read, cur_job);
				fflush(stdout);
			}
		} else if(rec == 1) {
			/* Save printjob */
			rec = -1;
			fclose(cur_job);
			printf("[*] RECEIVED\tPrint job %s\n", file_path);
		}
	}

	/* Close the device file. */
	close(fd[0].fd);

	return 0;
}

