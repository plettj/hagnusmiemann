CXX = g++
CXXFLAGS = -g -std=c++14 -Wall -MMD
EXEC = a4q4
OBJECTS = main.o board.o move.o
DEPENDS = ${OBJECTS:.o=.d}
${EXEC}: ${OBJECTS}
	${CXX} ${CXXFLAGS} ${OBJECTS} -o ${EXEC}

-include ${DEPENDS}

.PHONY: clean

clean: 
	rm ${OBJECTS} ${EXEC} ${DEPENDS}
