#pragma once
#include <GL/glew.h>
#ifdef __APPLE__
  #include <GLUT/glut.h>
#else
  #include <GL/freeglut.h>
#endif

class TriangleMesh;
extern TriangleMesh* mesh;   // 只宣告，不定義

void ReleaseResources();

void InitInterface();
void KeyboardCB(unsigned char key, int x, int y);
void MenuCB(int cmd);
void MouseButtonCB(int button, int state, int x, int y);
void MouseMotionCB(int x, int y);
void UpdateInterfaceSize(int width, int height);
void SetDrawMode(int mode);
void DrawHUD();

extern int mainWindow;
extern int ui_draw_mode;
extern bool ui_model_loaded;
extern int ui_menu_id;
