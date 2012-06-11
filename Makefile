.SUFFIXES:	.c .o

OUT	= toysh
OBJS	= toysh.o parser.o dictionary.o

CC	= gcc
FLAGS	= \
	-lreadline -lefence \
	-Wall \
	-g -ggdb \
	-std=c99 -D_GNU_SOURCE \

all: $(OUT)
$(OUT): $(OBJS)
	$(CC) $(FLAGS) -o $@ $(OBJS)

clean:
	rm $(OUT)
	rm $(OBJS)

.c.o:
	$(CC) $(FLAGS) -c $<
