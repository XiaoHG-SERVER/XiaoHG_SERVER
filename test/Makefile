

CC = g++ -std=c++11 -g 

INCLUDE_PATH = -I ../_include -I test_include/

SRCS = $(wildcard *.cxx)
OBJS = $(SRCS:.cxx=.o)

main: main.o ngx_c_crc32.o $(OBJS)
	$(CC) -o $@ $^ -lpthread

main.o: main.cxx
	$(CC) $(INCLUDE_PATH) -o $@ -c $^

ngx_c_crc32.o: ../misc/ngx_c_crc32.cxx
	$(CC) $(INCLUDE_PATH) -o $@ -c ../misc/ngx_c_crc32.cxx

%.o: %.cxx
	$(CC) $(INCLUDE_PATH) -o $@ -c $^

clean:
	rm *.o main