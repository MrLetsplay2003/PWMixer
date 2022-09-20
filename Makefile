CC=gcc
CFLAGS=-Wall -g

LIBFOLDER=lib
BINFOLDER=bin

.PHONY: all
all: $(LIBFOLDER)/libpwmixer.a $(BINFOLDER)/pwmixer_test

$(LIBFOLDER)/libpwmixer.a: pwmixer.o io.o
	mkdir -p $(LIBFOLDER)
	ar rcs $@ $^
	rm -f $^

%.o: src/pwmixer/%.c
	$(CC) $(CFLAGS) -c -I/usr/include/pipewire-0.3/ -I/usr/include/spa-0.2/ -fPIC $< -o $@

$(BINFOLDER)/pwmixer_test: src/pwmixer_test/pwmixer_test.c
	mkdir -p $(BINFOLDER)
	$(CC) $(CFLAGS) -L$(LIBFOLDER) -Isrc/pwmixer/ -I/usr/include/pipewire-0.3/ -I/usr/include/spa-0.2/ $< -lpipewire-0.3 -lpwmixer -o $@

.PHONY: clean
clean:
	rm -f *.o lib/*.a bin/pwmixer_test