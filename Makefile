CC=gcc
CFLAGS=-Wall -g
INCLUDES=-I/usr/include/pipewire-0.3/ -I/usr/include/spa-0.2/
LIBS=-lpipewire-0.3 -lpthread

BUILDFOLDER=build
LIBFOLDER=lib
BINFOLDER=bin

.PHONY: all
all: $(LIBFOLDER)/libpwmixer.a $(LIBFOLDER)/libpwmixer.so $(BINFOLDER)/pwmixer_test

$(LIBFOLDER)/libpwmixer.a: $(BUILDFOLDER)/pwmixer.o $(BUILDFOLDER)/io.o
	mkdir -p $(LIBFOLDER)
	ar rcs $@ $^

$(LIBFOLDER)/libpwmixer.so: $(BUILDFOLDER)/pwmixer.o $(BUILDFOLDER)/io.o
	mkdir -p $(LIBFOLDER)
	gcc -shared $(LIBS) $^ -o $@

$(BUILDFOLDER)/%.o: src/pwmixer/%.c
	mkdir -p $(BUILDFOLDER)
	$(CC) $(CFLAGS) -c $(INCLUDES) -fPIC $< -o $@

$(BINFOLDER)/pwmixer_test: src/pwmixer_test/pwmixer_test.c
	mkdir -p $(BINFOLDER)
	$(CC) $(CFLAGS) -L$(LIBFOLDER) $(INCLUDES) -Isrc/pwmixer/ $< $(LIBS) -Wl,-Bstatic -lpwmixer -Wl,-Bdynamic -o $@

.PHONY: clean
clean:
	rm -f *.o $(LIBFOLDER)/*.a $(LIBFOLDER)/*.so $(BINFOLDER)/pwmixer_test $(BUILDFOLDER)/*.o