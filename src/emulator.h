#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

typedef uint8_t         r_t;
typedef uint8_t*        memblock_ptr_t;

typedef uint16_t        word_t;
typedef uint8_t         minifloat_t;
typedef uint8_t         memptr_t;
typedef uint8_t         opcode_t;

#define get_word(mem, addr) *((word_t*)&mem[addr])
#define increment_counter(c) c += sizeof (word_t)

#define get_opcode(word) (uint8_t)((word & 0xF000) >> 12)
#define op_param_1(word) (uint8_t)((word & 0x0F00) >> 8)
#define op_param_2(word) (uint8_t)((word & 0x00F0) >> 4)
#define op_param_3(word) (uint8_t)((word & 0x000F))

#define MEMSIZE 256

struct GenRegisters {
        union {
                struct {
                r_t r0;
                r_t r1;
                r_t r2;
                r_t r3;
                r_t r4;
                r_t r5;
                r_t r6;
                r_t r7;
                r_t r8;
                r_t r9;
                r_t ra;
                r_t rb;
                r_t rc;
                r_t rd;
                r_t re;
                r_t rf;
                };
                uint8_t slots[16];
        };
};

struct PCPU {
        struct GenRegisters     regs;
        uint16_t                ir;
        uint8_t                 pc;
        memblock_ptr_t          memory;
};

#define LOAD_REGISTER_WITH_MEM  0x1
#define LOAD_REGISTER_WITH_VAL  0x2
#define WRITE_REGISTER_TO_MEM   0x3
#define COPY_REGISTER           0x4
#define ADD_SIGNED_INT          0x5
#define ADD_MINIFLOAT           0x6
#define BOOLEAN_OR              0x7
#define BOOLEAN_AND             0x8
#define BOOLEAN_XOR             0x9
#define BIT_ROTATE              0xA
#define JMP                     0xB
#define HALT                    0xC

// Extension instructions (non-standard)
#define EXT			                0xE

// namespaced instruction names for external use
#define OPCODE_LMA 	LOAD_REGISTER_WITH_MEM
#define OPCODE_LBP 	LOAD_REGISTER_WITH_VAL
#define OPCODE_SMA 	WRITE_REGISTER_TO_MEM
#define OPCODE_CRP 	COPY_REGISTER
#define OPCODE_ADS 	ADD_SIGNED_INT
#define OPCODE_ADF 	ADD_MINIFLOAT
#define OPCODE_OR  	BOOLEAN_OR
#define OPCODE_AND 	BOOLEAN_AND
#define OPCODE_XOR 	BOOLEAN_XOR
#define OPCODE_ROT 	BIT_ROTATE
#define OPCODE_JMP  JMP
#define OPCODE_HALT	HALT

#define ERR_NO_MEMORY "CPU manages no memory block."

#define err(msg) \
        fprintf(stderr, "Error: "msg"\n"); \
        exit(EXIT_FAILURE);

void decode_extension_op(struct PCPU* cpu) {
	      printf(": Non-standard instruction.\n");
}

#define BIAS        3
#define EXP_MASK    0b01110000
#define SIGN_MASK   0b10000000
#define MANT_MASK   0b00001111
#define FLOAT_POINT 0b00010000

int16_t normalize_float(minifloat_t flt) {
        int8_t exp = ((flt & EXP_MASK) >> 4) - BIAS;
        bool sign = (flt & SIGN_MASK) == SIGN_MASK;
        flt &= MANT_MASK;
        flt |= FLOAT_POINT;
        uint16_t normalized = flt;
        normalized <<= BIAS;
        if (exp > 0)  normalized <<= exp;
        else          normalized >>= abs(exp);
        if (sign)     normalized *= -1;
        return normalized;
}

#define EXT_MSB       0b0000100000000000
#define EXT_MANT_MASK 0b0000000000001111

minifloat_t to_float(int16_t buffer) {
        bool sign = false;
        if (buffer < 0) {
          sign = true;
          buffer *= -1;
        }
        uint8_t exp;
        for (exp = 0; (buffer & EXT_MSB) != EXT_MSB; exp++) buffer <<= 1;
        buffer <<= 1;
        buffer >>= 8;
        exp = 7 - exp;
        exp <<= 4;
        exp = exp | (uint8_t) (buffer & EXT_MANT_MASK);
        if (sign) exp |= SIGN_MASK;
        return (minifloat_t) exp;
}

minifloat_t minifloat_add(minifloat_t a, minifloat_t b) {
        if (a == 0x00 && b == 0x00) return 0x00;
        else if (a == 0x00)         return b;
        else if (b == 0x00)         return a;
        return to_float(normalize_float(a) + normalize_float(b));
}


void decode(struct PCPU* cpu) {
        opcode_t op = get_opcode(cpu->ir);
        printf("0x%.4X -> %X ", cpu->ir & 0xFFFF, op);

        uint8_t param_1 = op_param_1(cpu->ir);
        uint8_t param_2 = op_param_2(cpu->ir);
        uint8_t param_3 = op_param_3(cpu->ir);

        memptr_t addr;
        switch (op) {
        case LOAD_REGISTER_WITH_MEM:
                printf(": Load register with memory.\n");
                addr = (memptr_t)(cpu->ir & 0x00FF);
                cpu->regs.slots[param_1] = cpu->memory[addr];
                break;
        case LOAD_REGISTER_WITH_VAL:
                printf(": Load register with literal.\n");
                cpu->regs.slots[param_1] = (uint8_t)(cpu->ir & 0x00FF);
                break;
        case WRITE_REGISTER_TO_MEM:
                printf(": Write register to memory.\n");
                addr = (memptr_t)(cpu->ir & 0x00FF);
                cpu->memory[addr] = cpu->regs.slots[param_1];
                break;
        case COPY_REGISTER:
                printf(": Copy register to other register.\n");
                break;
        case ADD_SIGNED_INT:
                printf(": Add two signed ints.\n");
                break;
        case ADD_MINIFLOAT:
                printf(": Add two minifloats.\n");
                cpu->regs.slots[param_1]
                        = minifloat_add(
                                cpu->regs.slots[param_2],
                                cpu->regs.slots[param_3]
                          );
                break;
        case BOOLEAN_OR:
                printf(": OR two registers, store in dest.\n");
                cpu->regs.slots[param_1]
                        = cpu->regs.slots[param_2]
                        | cpu->regs.slots[param_3];
                break;
        case BOOLEAN_AND:
                printf(": AND two registers, store in dest.\n");
                cpu->regs.slots[param_1]
                        = cpu->regs.slots[param_2]
                        & cpu->regs.slots[param_3];
                break;
        case BOOLEAN_XOR:
                printf(": XOR two registers, store in dest.\n");
                cpu->regs.slots[param_1]
                        = cpu->regs.slots[param_2]
                        ^ cpu->regs.slots[param_3];
                break;
        case BIT_ROTATE:
                printf(": Rotate bits of a register.\n");
                uint8_t b = cpu->regs.slots[param_1];
                uint8_t r = param_3 % 8;
                cpu->regs.slots[param_1]
                        = (b >> r) | ((b & (0xFF >> (8-r))) << (8-r));
                break;
        case JMP:
                printf(": Jump if RX == R0\n");
                if (cpu->regs.slots[param_1] == cpu->regs.r0) {
                        cpu->pc = (uint8_t)(cpu->ir & 0x00FF);
                }
                break;
        case HALT:
                printf(": Halt.\n");
                break;
	case EXT:
		decode_extension_op(cpu);
		break;
        default:
                printf(": Unsupported instruction.\n");
                break;
        }
}

void pcpu_cycle(struct PCPU* cpu) {
        cpu->ir = get_word(cpu->memory, cpu->pc);
        decode(cpu);
        if (cpu->ir != OPCODE_HALT)
                increment_counter(cpu->pc);
}

struct PCPU pcpu_new(uint8_t memory[restrict MEMSIZE]) {
        struct PCPU cpu = {
                .regs = {},
                .ir = 0x0000,
                .pc = 0x00,
                .memory = memory
        };

        return cpu;
}

void pcpu_start(struct PCPU* cpu) {
        if (cpu->memory == NULL) {
                err(ERR_NO_MEMORY);
        }
        cpu->pc = cpu->memory[0];
}

struct Image {
        union {
                uint8_t pc;
                uint8_t memory[MEMSIZE];
        };
};

void pcpu_load_image(struct PCPU* cpu, struct Image* image) {
        if (cpu->memory == NULL) {
                err(ERR_NO_MEMORY);
        }
        memcpy(cpu->memory, image->memory, sizeof image->memory);
}

#define ADDR_PROGRAM_COUNTER 0x00

#define ERR_IMAGE_MEMCAP "Image generated exceeds memory capacity."

struct Image image_from_exec(uint8_t* exec_bytes, size_t len) {
        if (len > 255) {
                err(ERR_IMAGE_MEMCAP);
        }

        struct Image image = {};

        struct {
                uint8_t addr;
                uint8_t value;
        } pair;

        bool pc_set = false;
        for (int i = 0; i < len; i += 2) {
                if (i > 255) {
                        err(ERR_IMAGE_MEMCAP);
                }

                pair.addr =  exec_bytes[i];
                pair.value = exec_bytes[i+1];

                if (pc_set && pair.addr == image.pc) {
                        size_t len = pair.value;
                        memcpy(image.memory + pair.addr,
                                        &exec_bytes[i+2], len);
                        i += len;
                        continue;
                }

                switch (pair.addr) {
                case ADDR_PROGRAM_COUNTER:
                        if (!pc_set) {
                                image.pc = pair.value;
                                pc_set = true;
                        }
                        break;
                default:
                        image.memory[pair.addr] = pair.value;
                        break;
                }
        }

        return image;
}

struct Image image_from_exec_file(const char* path) {
        FILE* fs = fopen(path, "r");
        fseek(fs, 0, SEEK_END);
        size_t len = ftell(fs);
        rewind(fs);

        uint8_t* buf = malloc(len * sizeof *buf);
        fread(buf, len, 1, fs);

        fclose(fs);

        struct Image image = image_from_exec(buf, len);
        free(buf);

        return image;
}

struct Image image_from_file(const char* path) {
	struct Image image = {};

	FILE* fs = fopen(path, "r");
	fseek(fs, 0, SEEK_END);
	size_t len = ftell(fs);
	rewind(fs);

	if (len > MEMSIZE) {
		err(ERR_IMAGE_MEMCAP);
	}

	fread(image.memory, len, 1, fs);
	fclose(fs);

	return image;
}

void image_write_to_file(struct Image* img, const char* path) {
	FILE* fs = fopen(path, "w");
	fwrite(img->memory, MEMSIZE, 1, fs);
	fclose(fs);
}
