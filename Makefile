NAME = emu_gba.so

CXX = gcc

CFLAGS = -std=c99 -Wall -Wextra -Ofast -pipe -g -fPIC -march=native

LD = ld

LDFLAGS = -shared -static-libgcc -Wl,--version-script=link.T -Wl,--no-undefined

INCLUDES = -I include

SRCS_PATH = src/

SRCS_NAME = libretro/libretro.c \
            apu.c \
            gpu.c \
            mem.c \
            mbc.c \
            cpu.c \
            gba.c \
            cpu/thumb.c \
            cpu/arm.c \

SRCS = $(addprefix $(SRCS_PATH), $(SRCS_NAME))

OBJS_PATH = obj/

OBJS_NAME = $(SRCS_NAME:.c=.o)

OBJS = $(addprefix $(OBJS_PATH), $(OBJS_NAME))

all: odir $(NAME)

$(NAME): $(OBJS)
	@echo "LD gbabios"
	@$(LD) -r -b binary -o gbabios.o gbabios.bin
	@echo "LD $(NAME)"
	@$(LD) -fPIC -shared -o $(NAME) $(OBJS) gbabios.o

$(OBJS_PATH)%.o: $(SRCS_PATH)%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDES)

odir:
	@mkdir -p $(OBJS_PATH)
	@mkdir -p $(OBJS_PATH)/libretro
	@mkdir -p $(OBJS_PATH)/cpu

clean:
	@rm -f $(OBJS)
	@rm -f $(NAME)

.PHONY: all clean odir
