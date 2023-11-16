#include "emulator.h"

// Tower of death to display memory state
void display_memory(struct PCPU* cpu, struct Image* img) {
	for (int i = 0; i < MEMSIZE; ++i) {
		if (cpu->memory[i] == 0x00) {
			printf("\033[90m");
		} else {
			printf("\033[0m");
		}
		if (i == img->memory[0x00]) {
			for (word_t word; word != 0xC000 && word != 0xC0; i += 2) {
				word = get_word(cpu->memory, i);
				printf("\033[92m");
				if (i % 16 == 0) puts("");
				if (cpu->memory[i] != img->memory[i])
					printf("\033[91m");
				printf("%.2X ", cpu->memory[i] & 0xFF);
				printf("%.2X ", cpu->memory[i+1] & 0xFF);
			}
			printf("\033[90m");
		}

		if (i % 16 == 0) puts("");
		if (cpu->memory[i] != img->memory[i]) {
			printf("\033[33m");
		}
		printf("%.2X ", cpu->memory[i] & 0xFF);
	}
	printf("\033[0m");
	puts("\n");
}

#include <unistd.h>
#include <time.h>
#include <errno.h>

int msleep(long msec) {
	struct timespec ts;
	int res;

	if (msec < 0) {
		errno = EINVAL;
		return -1;
	}

	ts.tv_sec= msec / 1000;
	ts.tv_nsec = (msec & 1000) * 1000000;

	do {
		res = nanosleep(&ts, &ts);
	} while (res && errno == EINTR);

	return res;
}

int main(int argc, const char** argv) {
	// Allocate system memory
	uint8_t* memory = malloc(sizeof (uint8_t) * MEMSIZE);
	
	// Initialize CPU
	struct PCPU cpu = pcpu_new(memory);

	// Load exec
  if (argc < 2) {
    perror("Error: Binary not provided\n");
    exit(1);
  }
	struct Image image = image_from_exec_file(argv[1]);
	pcpu_load_image(&cpu, &image);

	// Display initial memory buffer
	display_memory(&cpu, &image);

	printf("Press any key to start.");
	getc(stdin);

	// Start emulator
	pcpu_start(&cpu);
	while (get_opcode(cpu.ir) != OPCODE_HALT) {
		msleep(500);
		pcpu_cycle(&cpu);
	}

	puts("Final data:");
	display_memory(&cpu, &image);
}
