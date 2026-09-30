CXX      = clang++
CXXFLAGS = -std=c++17 -O2 -DGL_SILENCE_DEPRECATION -DGLEW_NO_GLU -DUSE_GLUI=0
INCLUDES = -I/opt/homebrew/include -I/opt/homebrew/include/glm
LDFLAGS  = -L/opt/homebrew/lib
LIBS     = -framework OpenGL -framework GLUT -lGLEW   # 沒有 -lglut、沒有 -lglui

SRC = CG_HW1.cpp Interface.cpp TriangleMesh.cpp tinyfiledialogs.c
OUT = output/CG_HW1

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) $(INCLUDES) $(LDFLAGS) $(LIBS) -o $(OUT)

run:
	./$(OUT)
