CC=/usr/local/opt/llvm/bin/clang++
LDFLAGS=-L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib
CPPFLAGS=-I/usr/local/opt/llvm/include -fcoroutines-ts -std=c++14 -Wall

all: generator awaiter

clean:
	rm -f awaiter generator *.o

generator.o: generator.cpp
	$(CC) $(CPPFLAGS) -c $^

generator: generator.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

awaiter.o: awaiter.cpp
	$(CC) $(CPPFLAGS) -c $^

awaiter: awaiter.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@
