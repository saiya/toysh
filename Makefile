.SUFFIXES:	.c .o

OUT_TOYSH	= toysh
OUT_TEST	= toysh_test
OBJS		= parser.o dictionary.o
OBJS_TOYSH	= toysh.o $(OBJS)
OBJS_TEST	= toysh_test.o $(OBJS) dictionary_test.o

CC	= gcc
FLAGS	= \
	-lreadline -lefence \
	-Wall \
	-g -ggdb \
	-std=c99 -D_GNU_SOURCE
FLAGS_TEST = $(FLAGS) -lcunit

.c.o:
	$(CC) $(FLAGS) -c $<

all: $(OUT_TOYSH) $(OUT_TEST)

$(OUT_TOYSH): toysh.o $(OBJS_TOYSH)
	$(CC) $(FLAGS) -o $@ $(OBJS_TOYSH)

$(OUT_TEST): $(OBJS_TEST)
	$(CC) $(FLAGS_TEST) -o $@ $(OBJS_TEST)

clean:
	rm -f $(OUT) $(OUT_TEST)
	rm -f $(OBJS_TOYSH) $(OBJS_TEST)
