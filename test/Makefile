

CC = g++ -std=c++11 -g 

INCLUDE_PATH = -I ../_include -I test_include/

SRCS = $(wildcard *.cxx)
OBJS = $(SRCS:.cxx=.o)

main: main.o XiaoHG_C_Crc32.o $(OBJS)
	$(CC) -o $@ $^ -lpthread

main.o: main.cxx
	$(CC) $(INCLUDE_PATH) -o $@ -c $^

XiaoHG_C_Crc32.o: ../misc/XiaoHG_C_Crc32.cxx
	$(CC) $(INCLUDE_PATH) -o $@ -c ../misc/XiaoHG_C_Crc32.cxx

%.o: %.cxx
	$(CC) $(INCLUDE_PATH) -o $@ -c $^

clean:
	rm *.o main