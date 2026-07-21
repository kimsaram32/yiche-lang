src_dir = src
objects = $(src_dir)/main.o $(src_dir)/error.o $(src_dir)/vector.o $(src_dir)/character.o \
          $(src_dir)/input.o $(src_dir)/tokenizer.o $(src_dir)/parser.o
headers = $(src_dir)/error.h $(src_dir)/vector.h $(src_dir)/character.h \
          $(src_dir)/input.h $(src_dir)/tokenizer.h $(src_dir)/parser.h
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
