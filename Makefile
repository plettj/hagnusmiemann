CXX = g++
CXXFLAGS = -g -Wall -MMD
EXEC = chess
OBJECTS = main.o board.o move.o io.o zobrist.o moveorder.o evaluator.o easydifficulty.o window.o
DEPENDS = ${OBJECTS:.o=.d}
${EXEC}: ${OBJECTS}
	${CXX} ${CXXFLAGS} ${OBJECTS} -o ${EXEC} -lX11

-include ${DEPENDS}

.PHONY: clean

clean:
	rm ${OBJECTS} ${EXEC} ${DEPENDS}
