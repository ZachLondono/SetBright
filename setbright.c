#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

enum mode {Value, Set};

int main (int argc, char** argv) {

	if (argc == 1) {
		fprintf(stderr, "Usage: %s brightness\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (argc > 2) {
		perror("Invalid usage, enter a brightness value 0-100\n");
		fprintf(stderr, "Usage: %s brightness\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Check that the second argument is a number
	char* bright_str = argv[1];
	
	enum mode execution_mode = Set;

	if (strlen(bright_str) == 2) 
		if (bright_str[0] == '-' && bright_str[1] == 'v') execution_mode = Value;

	long new_bright = -1;
	if (execution_mode == Set) {
		// If mode is to set the brightness, the user must enter in a valid brightness value
		errno = 0;
		char* endptr;
		new_bright = strtol(bright_str, &endptr, 10);
		if ( (errno != 0 && new_bright == 0)			// Error parsing value 
				|| endptr == bright_str 		// No numbers found
				|| (new_bright < 0 || new_bright > 100) // Out of range
				|| endptr[0] != '\0') {			// Extra non-digit characters TODO: check if endptr = '.' for fractional percent
			perror("Brightness must be in range 0-100\n");
			exit(EXIT_FAILURE);
		}
	}


	// Opening file descriptors for the brightness file and max brightness file
	char* brightness_path = "/sys/class/backlight/intel_backlight/brightness";
	//char* brightness_path = "/home/zachary/Documents/Code/GTK/brightness/brightness_test";
	char* brightness_max_path = "/sys/class/backlight/intel_backlight/max_brightness";

	int max_fd = open(brightness_max_path, O_RDONLY);
	
	if (max_fd == -1) {
		perror("Could not change brightness value\n");
		if (errno == EACCES) perror("Failed to access brightness value, check permissions\n");
		else if (errno == ENOENT) perror("Brightness file does not exist\n");
	
		exit(EXIT_FAILURE);
	}

	// read the maximum brightness value
	char max_bright_buff[10];
	int count = 0;
	int total = 0;
	while ((count = read(max_fd, &max_bright_buff[total], 9)) != 0 && (total += count < 9))	{}
	close(max_fd);
	max_bright_buff[9] = '\0';

	errno = 0;
	char* max_endptr;
	long max_bright = strtol(max_bright_buff, &max_endptr, 10);
	if (errno != 0 && max_bright == 0) {
		perror("Error reading maximum brightness value\n");
		exit(EXIT_FAILURE);
	}

	if (execution_mode == Value) {
		
		int bright_fd = open(brightness_path, O_RDONLY);
		if (bright_fd == -1) {
			perror("Could not change brightness value\n");
			if (errno == EACCES) perror("Failed to access brightness value, check permissions\n");
			else if (errno == ENOENT) perror("Brightness file does not exist\n");
		
			exit(EXIT_FAILURE);
		}
		
		// Read in value from brightness
		char curr_bright_buff[10];
		size_t count = 0;
		size_t total = 0;
		while ((count = read(bright_fd, &curr_bright_buff[total], 9)) != 0 && (total += count < 9)) {}
		curr_bright_buff[9] = '\0';
		close(bright_fd);
		
		// Convert string to a number
		errno = 0;
		char* curr_endptr;
		long curr_bright = strtol(curr_bright_buff, &curr_endptr, 10);
		if (errno != 0 && curr_bright == 0) {
			perror("Error reading current brightness value\n");
			exit(EXIT_FAILURE);
		}
		
		// Just print out brightness level as %
		double brightness_percent = (double) curr_bright / (double) max_bright;
		printf("Brightness: %f%%\n", brightness_percent * 100);

	} else if (execution_mode == Set) {
		
		// Set new brightness value

		long new_actual_value = max_bright * ((double) new_bright / 100);

		FILE* fptr = fopen(brightness_path, "w");
		if (fptr == NULL) {
			perror("Error setting brightness\n");
			exit(EXIT_FAILURE);
		}

		fprintf(fptr, "%ld", new_actual_value);
		fclose(fptr);

		printf("Brightness set to: %ld\n", new_actual_value);

	}
	
	exit(EXIT_SUCCESS);
	
}
