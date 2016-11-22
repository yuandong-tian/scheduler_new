OPT = -O3 -msse3

INCLUDES = 
LIBS = -lstdc++
CXX_FLAGS = -std=c++11 
GCC = gcc

main: *.cc *.h
	${GCC} ${OPT} ${CXX_FLAGS} ${INCLUDES} ${LIBS} schedule_lib.cc schedule.cc -o schedule

clean:
	rm *.o schedule

