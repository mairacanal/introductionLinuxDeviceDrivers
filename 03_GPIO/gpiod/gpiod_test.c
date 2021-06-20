#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

/*  
 *  @brief Opens the gpio chip and get the requested gpio line
 *  @param chipname The gpiochip name
 *  @param gpio The gpiod_line number
 *  @return returns the correspondent gpiod_line
 */

struct gpiod_line *get_gpio_line(const char* chipname, int gpio);

int main(int argc, char **argv) {

	const char *buttonChipname = "gpiochip1";
	const char *ledChipname = "gpiochip3";

	struct gpiod_line *ledLine, *buttonLine;
	struct gpiod_line_event event;
	struct timespec tm = {1,0};

	int val = 0;

	ledLine = get_gpio_line(ledChipname, 19);
	buttonLine = get_gpio_line(buttonChipname, 17);

	// Request a interrupt at the gpiod_line
	if (gpiod_line_request_rising_edge_events(buttonLine, "gpio-test") < 0) {
		perror("Request events failed\n");
		return EXIT_FAILURE;
	}

	// Sets the gpiod_line to output
	if (gpiod_line_request_output(ledLine, "gpio-test", val) < 0) {
		perror("Request output failed\n");
		return EXIT_FAILURE;
	}

	while(1) {

		// Waits the event be triggered
		gpiod_line_event_wait(buttonLine, &tm);

		if (gpiod_line_event_read(buttonLine, &event) != 0)
			continue;

		if (event.event_type != GPIOD_LINE_EVENT_RISING_EDGE)
			continue;

		// Toggle the LED
		val = !val;
		gpiod_line_set_value(ledLine, val);

	}

	return EXIT_SUCCESS;	

}

struct gpiod_line *get_gpio_line(const char* chipname, int gpio) {

	struct gpiod_chip *chip;
	struct gpiod_line *line;

	// Open GPIO chip
	chip = gpiod_chip_open_by_name(chipname);
	if (chip == NULL) {
		perror("Error opening GPIO chip");
		return NULL;
	}

	// Open GPIO lines
	line = gpiod_chip_get_line(chip, gpio);
	if (line == NULL) {
		perror("Error opening GPIO line");
		return NULL;
	}

	return line;

}
