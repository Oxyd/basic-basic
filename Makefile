TARGET=		basic
OBJECTS=	src/main.o src/lexer.o src/parser.o src/statements.o src/interpreter.o src/number.o

CXXFLAGS=	$(INCLUDE_DIRS) -Wall -ansi -pedantic -g

.PHONY:		all clean

all : $(TARGET) Makefile
	
clean:
	rm -f $(OBJECTS) $(DEPFILES) $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS)

$(OBJECTS) : %.o : %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^
