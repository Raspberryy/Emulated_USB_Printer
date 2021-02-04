  /*
   * Emulated USB Printer
   * Copyright (C) 2021
   *
   * This program is free software: you can redistribute it and/or modify
   * it under the terms of the GNU General Public License as published by
   * the Free Software Foundation, either version 3 of the License, or
   * (at your option) any later version.
   *
   * This program is distributed in the hope that it will be useful,
   * but WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU General Public License for more details.
   *
   * You should have received a copy of the GNU General Public License
   * along with this program.  If not, see <https://www.gnu.org/licenses/>.
   *
   * ------------------------------------------------------------------------
   *
   * This program is contains some examples by Craig W. Nadler (2007):
   * https://www.kernel.org/doc/Documentation/usb/gadget_printer.rst
   *
   * ------------------------------------------------------------------------
   *
   * The Emulated USB Printer is based on the Linux USB Gadget API.
   * Using the USB Gadget API the g_printer module will create the device file
   * /dev/g_printer which is used to read data from the host and set or get
   * the current printer status.
   *
   * See Makefile for more details on how to load or unload the
   * g_printer driver.
   *
   * Instead of writing received print jobs to stdout we store all print jobs
   * in FILE_OUTPUT_PATH as files with PRINTJOB_FILE_EXTENSION as type. We recommend
   * using GhostPDL to process the received print jobs. Note that GhostPDL
   * might need to be installed separately from Ghostscript.
   *
   */

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
  #define PRINTJOB_FILE_EXTENSION	"pcl"
  #define GPDL_BIN_FILE			"/home/pi/Downloads/ghostpdl-9.53.3/bin/gpdl"
  #define GPDL_SDEVICE_METHOD		"pdfwrite"
  #define GPDL_FILE_EXTENSION		"pdf"

  const char* FILE_OUTPUT_PATH = "printjobs/";

  static int display_printer_status();


  // ******************************
  // ***   GhostPDL  Support	***
  // ******************************

  /*
   * 'gpdl_parse_printjob()' - Calls GPDL_BIN_FILE
   * with GPDL_SDEVICE_METHOD to convert printjob.
   * Saves output in FILE_OUTPUT_PATH.
   */

  static void
  gpdl_parse_printjob(const char* printjob, const char* outp_filename)
  {
	// Check if GPDL is usable
	if(strcmp(GPDL_BIN_FILE, "") == 0 || strcmp(GPDL_SDEVICE_METHOD, "") == 0
		|| printjob == NULL || outp_filename == NULL)
		return;

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
		printf("[!] Error\tGPDL failed to process printjob\n");
		_exit(-1);
	}

	while (fgets(line, sizeof(line), systemOutput) != NULL) {
   		if(error || strstr(line, "error") != NULL || strstr(line, "failed") != NULL){
			printf("[!] Error\tGPDL error: %s\n", line);
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


  // ******************************
  // ***   Print job reading	***
  // ******************************

  /*
   * 'read_printer_data()' - Polls PRINTER_FILE
   * to read and write print jobs to FILE_OUTPUT_PATH.
   * Uses fork() to enable async print job processing
   *
   * When receiving a print job:
   * 	Write data to FILE cur_job.
   * 	If no new data arrives for at least one second
   * 	Closes cur_job and creates new file.
   */

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
		printf("[!] Error\t%d opening %s\n", fd[0].fd, PRINTER_FILE);
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
                        	sprintf(pjob_outp_file,"%s%s.%s", FILE_OUTPUT_PATH, filename, PRINTJOB_FILE_EXTENSION);

                        	// Create PCL file
                        	cur_job = fopen(pjob_outp_file, "w+");
                        	if(cur_job == NULL){
                                	printf("[!] Error\tOpening %s. Exiting\n", pjob_outp_file);
                                	return -1;
                        	}

                        	rec = 0;
			}


			/* Read data from printer gadget driver. */
			bytes_read = read(fd[0].fd, buf, BUF_SIZE);

			if (bytes_read < 0) {
				printf("[!] Error\t%d reading from %s\n",
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

			printf("[*] FINSIEHD\tReceived print job %s.%s\n",
                        	filename, PRINTJOB_FILE_EXTENSION);
		}
	}

	/* Close the device file. */
	close(fd[0].fd);

	return 0;
  }


  // **************************
  // ***  Printer Status    ***
  // **************************

  /*
   * 'get_printer_status()' - Gets Bits as
   * defined in g_printer.h to get
   * current printer status as shown to the host
   */

  static int
  get_printer_status()
  {
	int	retval;
	int	fd;

	/* Open device file for printer gadget. */
	fd = open(PRINTER_FILE, O_RDWR);
	if (fd < 0) {
		printf("[!] Error\t%d opening %s\n", fd, PRINTER_FILE);
		close(fd);
		return(-1);
	}

	/* Make the IOCTL call. */
	retval = ioctl(fd, GADGET_GET_PRINTER_STATUS);
	if (retval < 0) {
		fprintf(stderr, "[!] Error\tFailed to set printer status\n");
		return(-1);
	}

	/* Close the device file. */
	close(fd);

	return(retval);
  }


  /*
   * 'set_printer_status()' - Sets Bits as
   * defined in g_printer.h to set
   * current printer status shown to the host
   */

  static int
  set_printer_status(unsigned char buf, int clear_printer_status_bit)
  {
	int	retval;
	int	fd;

	retval = get_printer_status();
	if (retval < 0) {
	fprintf(stderr, "[!] Error\tFailed to get printer status\n");
		return(-1);
	}

	/* Open device file for printer gadget. */
	fd = open(PRINTER_FILE, O_RDWR);

	if (fd < 0) {
		printf("[!] Error\t%d opening %s\n", fd, PRINTER_FILE);
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
		fprintf(stderr, "[!] Error\tFailed to set printer status\n");
		return(-1);
	}

	/* Close the device file. */
	close(fd);

	return 0;
  }


  /*
   * 'display_printer_status()' - Prints
   * current printer status as shown to the host
   */

  int
  display_printer_status()
  {
	char	printer_status;

	printer_status = get_printer_status();
	if (printer_status < 0) {
		fprintf(stderr, "[!] Error\tFailed to get printer status\n");
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
		printf("\tPrinter Error\n");
	}

	return(0);
  }


  /*
   * 'usage()' - Prints help and usage to stdout
   */

  static void
  usage(const char *option)		/* I - Option string or NULL */
  {
	if (option) {
		fprintf(stderr,"emulatedprinter: Unknown option \"%s\"!\n",
				option);
	}

	fputs("\n", stderr);
	fputs("Usage: emulatedprinter -[options]\n", stderr);
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
