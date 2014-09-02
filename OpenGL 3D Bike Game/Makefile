CC = g++
CFLAGS = 
PROG = fossilparkride

SRCS = game.cpp imageloader.cpp vec3f.cpp

ifeq ($(shell uname),Darwin)
	LIBS = -framework OpenGL -framework GLUT
else
	LIBS = -lopenal -lalut -lglut -lGLU -lGL
endif

all: $(PROG)

$(PROG):	$(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS)

clean:
	rm -f $(PROG)
