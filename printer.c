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
  #define BUF_SIZE			512
  #define GPDL_BIN_FILE			"/home/pi/Downloads/ghostpdl-9.53.3/bin/gpdl"
  #define GPDL_SDEVICE_METHOD		"pdfwrite"
  #define GPDL_FILE_EXTENSION		"pdf"

  const char* FILE_OUTPUT_PATH = "printjobs/";

  static int display_printer_status();

  // ****************************
  // *** Processing printouts ***
  // ****************************

  /*
   * 'gpdl_parse_printjob()' - Calls GPDL_BIN_FILE
   * with GPDL_SDEVICE_METHOD to convert printjob.
   * Saves output in FILE_OUTPUT_PATH.
   */

  static void
  gpdl_parse_printjob(const char* printjob, const char* outp_filename)
  {
	// Prepare gpdl
	char sys_cmd[1024];
	sprintf(sys_cmd,
		"%s -dNOPAUSE -sDEVICE=%s -sOutputFile=%s%s.%s %s",
		GPDL_BIN_FILE,
		GPDL_SDEVICE_METHOD,
		FILE_OUTPUT_PATH,
		outp_filename,
		GPDL_FILE_EXTENSION,
		printjob);

	// System call
	FILE* systemOutput;
 	char line[1024];
	int error = 0;
	systemOutput = popen(sys_cmd, "r");

	if(systemOutput == NULL){
		printf("[!] ERROR\tGPDL failed to process printjob\n");
		_exit(-1);
	}

	while (fgets(line, sizeof(line), systemOutput) != NULL) {
   		if(error || strstr(line, "error") != NULL || strstr(line, "failed") != NULL){
			printf("[!] ERROR\tGPDL error: %s\n", line);
			error = 1;
		}
  	}

	if(!error)
		printf("[*] FINSIEHD\tNew printout %s.%s\n", 
			outp_filename, GPDL_FILE_EXTENSION);

	// Kill child process
	pclose(systemOutput);
	_exit(0);
  }

  /*
   * 'usage()' - Show program usage.
   */

  static void
  usage(const char *option)		/* I - Option string or NULL */
  {
	if (option) {
		fprintf(stderr,"emulatedprinter: Unknown option \"%s\"!\n",
				option);
	}

	fputs("\n", stderr);
	fputs("Usage: prn_example -[options]\n", stderr);
	fputs("Options:\n", stderr);
	fputs("\n", stderr);
	fputs("-get_status    Get the current printer status.\n", stderr);
	fputs("-selected      Set the selected status to selected.\n", stderr);
	fputs("-not_selected  Set the selected status to NOT selected.\n",
			stderr);
	fputs("-error         Set the error status to error.\n", stderr);
	fputs("-no_error      Set the error status to NO error.\n", stderr);
	fputs("-paper_out     Set the paper status to paper out.\n", stderr);
	fputs("-paper_loaded  Set the paper status to paper loaded.\n",
			stderr);
	fputs("-read_data     Read printer data from driver.\n", stderr);
	fputs("-write_data    Write printer sata to driver.\n", stderr);
	fputs("\n\n", stderr);

	exit(1);
  }


  static int
  read_printer_data()
  {
	// Print Info
	if(display_printer_status() != 0)
		return -1;
	printf("[*] Start reading printjobs\n");

	/* Open device file for printer gadget. */
	struct pollfd fd[1];
	fd[0].fd = open(PRINTER_FILE, O_RDWR);
	if (fd[0].fd < 0) {
		printf("Error %d opening %s\n", fd[0].fd, PRINTER_FILE);
		close(fd[0].fd);
		return(-1);
	}

	fd[0].events = POLLIN | POLLRDNORM;
	int rec = -1;
	char filename[100];
	char pjob_outp_file[512];
	FILE* cur_job;

	while (1) {
		static char buf[BUF_SIZE];
		int bytes_read;
		int retval;

		if(rec == -1) {
			time_t     now;
    			struct tm  ts;

    			// Create file name
    			time(&now);
			ts = *localtime(&now);
    			strftime(filename, sizeof(filename), "%Y-%m-%d-%H-%M-%S", &ts);
			sprintf(pjob_outp_file,"%s%s.pcl", FILE_OUTPUT_PATH, filename);

			// Create PCL file
			cur_job = fopen(pjob_outp_file, "w+");
			if(cur_job == NULL){
				printf("Error opening %s. Exiting\n", pjob_outp_file);
				return -1;
			}

			rec = 0;
		}

		/* Wait for up to 1 second for data. */
		retval = poll(fd, 1, 1000);

		if (retval && (fd[0].revents & POLLRDNORM)) {

			/* Read data from printer gadget driver. */
			bytes_read = read(fd[0].fd, buf, BUF_SIZE);

			if (bytes_read < 0) {
				printf("Error %d reading from %s\n",
						fd[0].fd, PRINTER_FILE);
				close(fd[0].fd);
				return(-1);
			} else if (bytes_read > 0) {
				rec = 1;
				/* Write data to standard OUTPUT (stdout). */
				fwrite(buf, 1, bytes_read, cur_job);
				fflush(stdout);
			}

		} else if(rec == 1) {
			rec = -1;
			fclose(cur_job);

			// Start async printjob conversion
			if(fork() == 0){
				gpdl_parse_printjob(pjob_outp_file, filename);
			}
		}
	}

	/* Close the device file. */
	close(fd[0].fd);

	return 0;
  }


  static int
  get_printer_status()
  {
	int	retval;
	int	fd;

	/* Open device file for printer gadget. */
	fd = open(PRINTER_FILE, O_RDWR);
	if (fd < 0) {
		printf("Error %d opening %s\n", fd, PRINTER_FILE);
		close(fd);
		return(-1);
	}

	/* Make the IOCTL call. */
	retval = ioctl(fd, GADGET_GET_PRINTER_STATUS);
	if (retval < 0) {
		fprintf(stderr, "ERROR: Failed to set printer status\n");
		return(-1);
	}

	/* Close the device file. */
	close(fd);

	return(retval);
  }


  static int
  set_printer_status(unsigned char buf, int clear_printer_status_bit)
  {
	int	retval;
	int	fd;

	retval = get_printer_status();
	if (retval < 0) {
		fprintf(stderr, "ERROR: Failed to get printer status\n");
		return(-1);
	}

	/* Open device file for printer gadget. */
	fd = open(PRINTER_FILE, O_RDWR);

	if (fd < 0) {
		printf("Error %d opening %s\n", fd, PRINTER_FILE);
		close(fd);
		return(-1);
	}

	if (clear_printer_status_bit) {
		retval &= ~buf;
	} else {
		retval |= buf;
	}

	/* Make the IOCTL call. */
	if (ioctl(fd, GADGET_SET_PRINTER_STATUS, (unsigned char)retval)) {
		fprintf(stderr, "ERROR: Failed to set printer status\n");
		return(-1);
	}

	/* Close the device file. */
	close(fd);

	return 0;
  }


  int
  display_printer_status()
  {
	char	printer_status;

	printer_status = get_printer_status();
	if (printer_status < 0) {
		fprintf(stderr, "[!] ERROR\tFailed to get printer status\n");
		return(-1);
	}

	printf("[*] Printer status\n");
	if (printer_status & PRINTER_SELECTED) {
		printf("\tPrinter is Selected\n");
	} else {
		printf("\tPrinter is NOT Selected\n");
	}
	if (printer_status & PRINTER_PAPER_EMPTY) {
		printf("\tPaper is Out\n");
	} else {
		printf("\tPaper is Loaded\n");
	}
	if (printer_status & PRINTER_NOT_ERROR) {
		printf("\tPrinter OK\n");
	} else {
		printf("\tPrinter ERROR\n");
	}

	return(0);
  }


  int
  main(int  argc, char *argv[])
  {
	int	i;		/* Looping var */
	int	retval = 0;

	/* No Args */
	if (argc == 1) {
		usage(0);
		exit(0);
	}

	for (i = 1; i < argc && !retval; i ++) {

		if (argv[i][0] != '-') {
			continue;
		}

		if (!strcmp(argv[i], "-get_status")) {
			if (display_printer_status()) {
				retval = 1;
			}

		} else if (!strcmp(argv[i], "-paper_loaded")) {
			if (set_printer_status(PRINTER_PAPER_EMPTY, 1)) {
				retval = 1;
			}

		} else if (!strcmp(argv[i], "-paper_out")) {
			if (set_printer_status(PRINTER_PAPER_EMPTY, 0)) {
				retval = 1;
			}

		} else if (!strcmp(argv[i], "-selected")) {
			if (set_printer_status(PRINTER_SELECTED, 0)) {
				retval = 1;
			}

		} else if (!strcmp(argv[i], "-not_selected")) {
			if (set_printer_status(PRINTER_SELECTED, 1)) {
				retval = 1;
			}

		} else if (!strcmp(argv[i], "-error")) {
			if (set_printer_status(PRINTER_NOT_ERROR, 1)) {
				retval = 1;
			}

		} else if (!strcmp(argv[i], "-no_error")) {
			if (set_printer_status(PRINTER_NOT_ERROR, 0)) {
				retval = 1;
			}

		} else if (!strcmp(argv[i], "-read_data")) {
			if (read_printer_data()) {
				retval = 1;
			}
		} else {
			usage(argv[i]);
			retval = 1;
		}
	}

	exit(retval);
  }
