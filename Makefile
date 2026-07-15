src_dir = src
objects = $(src_dir)/error.o $(src_dir)/main.o $(src_dir)/tokenizer.o
headers = $(src_dir)/error.h $(src_dir)/tokenizer.h
bin = yiche

CC = gcc

all: $(bin)

debug: CFLAGS += -g
debug: $(bin)

$(bin): $(objects)
	$(CC) $(CFLAGS) -o $@ $^

$(objects): $(src_dir)/yiche.h $(headers)

.PHONY: clean
clean:
	rm $(bin) $(objects)
