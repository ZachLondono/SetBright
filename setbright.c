/*
*	SetBright - changes the brightness of the laptops backlight
*	* requires root privleges to set new brightness * 
*/


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

// File where the brightness value needs to be saved
const char* brightness_path = "/sys/class/backlight/intel_backlight/brightness";
// File that contains the maximum brightness
const char* brightness_max_path = "/sys/class/backlight/intel_backlight/max_brightness";

enum mode {Value, Set, Inc, Dec};

int set_by_val(long new_bright);
long parse_argument_value(char* arg);
long read_brightness(const char* path);

int main (int argc, char** argv) {

	enum mode exec_mode = Set;

	if (argc == 1) {
		fprintf(stderr, "Usage: %s brightness\n", argv[0]);
		fprintf(stderr, "list options: %s --help \n", argv[0]);
		exit(EXIT_FAILURE);
	} else {

		char* flag = argv[1];
		if (strcmp(flag, "--help") == 0|| strcmp(flag, "-h") == 0) {
			
			// ignore all other flags, just print help dialogue
			printf("Usage: %s <brightness>\n\n"
			"Adjust the brightness of the backlight\n\n"
			"   -h, --help	Print options\n"
			"   -v		Print current backlight brightness\n"
			"   -inc VAL	increment brightness\n"
			"   -dec VAL:	decrement brightness\n", argv[0]);
				
			exit(EXIT_SUCCESS);
				
		} if (strcmp(flag, "-v") == 0) {
			exec_mode = Value;
		} else if (strcmp(flag, "-inc") == 0) {
			exec_mode = Inc;
			if (argc != 3) {
				printf("Invalid input\nUsage: %s -inc VAL\n", argv[0]);
				exit(EXIT_FAILURE);
			}
		} else if (strcmp(flag, "-dec") == 0) {
			exec_mode = Dec;
			if (argc != 3) {
				printf("Invalid input\nUsage: %s -dec VAL\n", argv[0]);
				exit(EXIT_FAILURE);
			}
		} else {
			exec_mode = Set;
			if (argc != 2) {
				printf("Invalid input\nUsage: %s VAL\n", argv[0]);
				exit(EXIT_FAILURE);
			}
		}

	}


	long max_bright = read_brightness(brightness_max_path);
	if (max_bright == -1) perror("max");

	if (exec_mode == Value) {
		
		long curr_bright = read_brightness(brightness_path);
		if (curr_bright < 0) {
			perror("Failed to read current brightness value");
			exit(EXIT_FAILURE);
		}
		
		// Just print out brightness level as %
		double brightness_percent = (double) curr_bright / (double) max_bright;
		printf("Brightness: %f%%\n", brightness_percent * 100);
		
		exit(EXIT_SUCCESS);

	} 


	long new_bright = 1; // defualt to lowest without being completely off

	if (exec_mode == Set) {
		
		// If mode is to set the brightness, the user must enter in a valid brightness value
		long input = parse_argument_value(argv[1]);		
		if (input < 0 || input > 100) {
			perror("Invalid value, enter brightness in range 0-100");
			exit(EXIT_FAILURE);
		}

		new_bright = ((double) input / 100) * max_bright;
		printf("new_bright: %ld\n", new_bright);
		
	} else {

		long curr_bright = read_brightness(brightness_path);
		if (curr_bright < 0) {
			perror("Failed to read current brightness value");
			exit(EXIT_FAILURE);
		}

		long input_raw = parse_argument_value(argv[2]);		
		if (input_raw < 0 || input_raw > 100) {
			perror("Invalid value, enter brightness in range 0-100");
			exit(EXIT_FAILURE);
		}

		long input = ((double) input_raw / 100) * max_bright;

		if (exec_mode == Inc) {
			if (curr_bright + input > max_bright) 
				new_bright = max_bright;
			else new_bright = curr_bright + input;
		} else if (exec_mode == Dec) {
			if (curr_bright - input < 1) 
				new_bright = 1;
			else new_bright = curr_bright - input;
		}

	}
	
	int ret = set_by_val(new_bright);
	if (ret < 0) {
		perror("Failed to set brightness\n");
		exit(EXIT_FAILURE);
	}
	
}


/*
*	Set the brightness as a percentage of max brightness
*/
int set_by_val(long new_bright) {

	FILE* fptr = fopen(brightness_path, "w");
	if (fptr == NULL) {
		return -1;
	}

	int ret = fprintf(fptr, "%ld", new_bright);
	fclose(fptr);
	
	if (ret < 0) return -1;

	return 0;

}

long parse_argument_value(char* arg) {

	errno = 0;
	char* endptr;
	long parsed = strtol(arg, &endptr, 10);
	if ( (errno != 0 && parsed == 0)				// Error parsing value 
			|| endptr == arg 				// No numbers found
			|| (endptr[0] != '\0' && endptr[0] != '\n')) {	// Extra non-digit characters TODO: check if endptr = '.' for fractional percent
		return -1;
	}

	return parsed;

}


long read_brightness(const char* path) {

	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		perror("Could not change brightness value\n");
		if (errno == EACCES) perror("Failed to access brightness value, check permissions\n");
		else if (errno == ENOENT) perror("Brightness file does not exist\n");
		return -1;
	}

	char* bright_buff = malloc(10);
	int count = 0;
	int total = 0;
	while ((count = read(fd, &bright_buff[total], 9)) != 0 && (total += count < 9))	{}
	close(fd);
	bright_buff[9] = '\0';

	long parsed = parse_argument_value(bright_buff);		
	
	free(bright_buff);

	return parsed;

}
