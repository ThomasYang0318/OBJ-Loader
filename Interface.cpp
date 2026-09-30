#include "Interface.h"
#include "TriangleMesh.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// tinyfiledialogs（用你的專案已有的 tinyfiledialogs.c／.h）
extern "C" {
  char const* tinyfd_openFileDialog(
      char const* aTitle, char const* aDefaultPathAndFile,
      int aNumOfFilterPatterns, char const* const* aFilterPatterns,
      char const* aSingleFilterDescription, int aAllowMultipleSelects);
}

extern TriangleMesh* mesh;  // 由主程式定義

extern void SetupScene(const std::string& modelPath);

int mainWindow = 0;
int ui_draw_mode = 1;   // default: wireframe
int ui_menu_id = 0;

namespace {

// ====== 右鍵選單項目代碼 ======
enum {
  MENU_LOAD = 1,
  MENU_DELETE,
  MENU_MODE_POINT,
  MENU_MODE_WIRE,
  MENU_MODE_FILL,
  MENU_EXIT
};

struct UIButton {
  std::string label;
  int command = 0;
  bool requiresModel = false;
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
};

std::vector<UIButton> g_buttons;
int g_hover_button = -1;
int g_active_button = -1;
int g_window_width = 650;
int g_window_height = 600;

void ApplyPolygonMode()
{
  if (ui_draw_mode == 0) glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
  else if (ui_draw_mode == 1) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void LayoutButtons()
{
  const int padding = 12;
  const int buttonHeight = 32;
  void* font = GLUT_BITMAP_HELVETICA_12;
  int cursorX = padding;
  int baseY = 20;  // pixels from bottom

  const auto computeTextWidth = [&](const std::string& text) {
    int width = 0;
    for (unsigned char c : text) width += glutBitmapWidth(font, c);
    return width;
  };

  for (auto& btn : g_buttons) {
    btn.h = buttonHeight;
    btn.w = computeTextWidth(btn.label) + 26;  // padding for left/right
    btn.x = cursorX;
    btn.y = baseY;
    cursorX += btn.w + padding;
  }
}

int ButtonIndexAt(int x, int y)
{
  if (g_buttons.empty()) return -1;
  LayoutButtons();

  for (size_t i = 0; i < g_buttons.size(); ++i) {
    const auto& btn = g_buttons[i];
    if (x >= btn.x && x <= btn.x + btn.w &&
        y >= btn.y && y <= btn.y + btn.h) {
      if (btn.requiresModel && !ui_model_loaded) continue;
      return static_cast<int>(i);
    }
  }
  return -1;
}

void ExecuteCommand(int cmd)
{
  switch (cmd) {
    case MENU_LOAD: {
      char const* filters[] = { "*.obj" };
      const char* path = tinyfd_openFileDialog(
          "Select OBJ model", "", 1, filters, "OBJ Files", 0);
      if (path) {
        std::cout << "[Load] " << path << "\n";
        SetupScene(path);
      }
      glutPostRedisplay();
    } break;

    case MENU_DELETE:
      if (mesh) {
        std::cout << "[Delete] Releasing model resources...\n";
      }
      ReleaseResources();
      glutPostRedisplay();
      break;

    case MENU_MODE_POINT:
      SetDrawMode(0);
      break;
    case MENU_MODE_WIRE:
      SetDrawMode(1);
      break;
    case MENU_MODE_FILL:
      SetDrawMode(2);
      break;

    case MENU_EXIT:
      std::cout << "[Exit] Bye.\n";
      ReleaseResources();
      exit(0);
      break;

    default:
      break;
  }
}

} // namespace

void InitInterface()
{
  std::cout << "================= Controls (Cross-Platform) =================\n";
  std::cout << "[L] Load OBJ   [D] Delete   [1]Point [2]Wire [3]Fill   [Esc] Exit\n";
  std::cout << "(Right-click) menu + on-screen buttons: Load/Delete/Mode/Exit\n";
  std::cout << "=============================================================\n";

  // 建立右鍵選單
  ui_menu_id = glutCreateMenu(MenuCB);
  glutAddMenuEntry("Load OBJ...",     MENU_LOAD);
  glutAddMenuEntry("Delete model",    MENU_DELETE);
  glutAddMenuEntry("----",            0);
  glutAddMenuEntry("Mode: Point",     MENU_MODE_POINT);
  glutAddMenuEntry("Mode: Wireframe", MENU_MODE_WIRE);
  glutAddMenuEntry("Mode: Solid",     MENU_MODE_FILL);
  glutAddMenuEntry("----",            0);
  glutAddMenuEntry("Exit",            MENU_EXIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  g_buttons = {
      {"Load OBJ...",     MENU_LOAD,       false},
      {"Delete Model",    MENU_DELETE,     true},
      {"Point Mode",      MENU_MODE_POINT, false},
      {"Wireframe Mode",  MENU_MODE_WIRE,  false},
      {"Solid Mode",      MENU_MODE_FILL,  false},
      {"Exit",            MENU_EXIT,       false},
  };

  ApplyPolygonMode();
}

void MenuCB(int cmd)
{
  if (cmd == 0) return;  // ignore separators
  ExecuteCommand(cmd);
}

void KeyboardCB(unsigned char key, int, int)
{
  switch (key) {
    case 27: // ESC
      ExecuteCommand(MENU_EXIT);
      break;
    case 'l':
    case 'L':
      ExecuteCommand(MENU_LOAD);
      break;
    case 'd':
    case 'D':
      ExecuteCommand(MENU_DELETE);
      break;
    case '1':
      ExecuteCommand(MENU_MODE_POINT);
      break;
    case '2':
      ExecuteCommand(MENU_MODE_WIRE);
      break;
    case '3':
      ExecuteCommand(MENU_MODE_FILL);
      break;
    default:
      break;
  }
}

void MouseButtonCB(int button, int state, int x, int y)
{
  if (button != GLUT_LEFT_BUTTON) return;

  const int px = x;
  const int py = glutGet(GLUT_WINDOW_HEIGHT) - y;
  const int index = ButtonIndexAt(px, py);

  if (state == GLUT_DOWN) {
    g_active_button = index;
    glutPostRedisplay();
    return;
  }

  if (state == GLUT_UP) {
    if (g_active_button == index && index >= 0) {
      ExecuteCommand(g_buttons[index].command);
    }
    g_active_button = -1;
    glutPostRedisplay();
  }
}

void MouseMotionCB(int x, int y)
{
  const int px = x;
  const int py = g_window_height - y;
  const int index = ButtonIndexAt(px, py);
  if (index != g_hover_button) {
    g_hover_button = index;
    glutPostRedisplay();
  }
}

void UpdateInterfaceSize(int width, int height)
{
  g_window_width = std::max(width, 1);
  g_window_height = std::max(height, 1);
}

void SetDrawMode(int mode)
{
  const int clamped = std::clamp(mode, 0, 2);
  if (ui_draw_mode == clamped) return;
  ui_draw_mode = clamped;
  ApplyPolygonMode();
  glutPostRedisplay();
}

void DrawHUD()
{
  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);
  //UpdateInterfaceSize(w, h);
  gluOrtho2D(0, w, 0, h);
  
  LayoutButtons();

  const int activeModeCommand =
      (ui_draw_mode == 0) ? MENU_MODE_POINT :
      (ui_draw_mode == 1) ? MENU_MODE_WIRE :
                            MENU_MODE_FILL;

  // Draw button backgrounds.
  for (size_t i = 0; i < g_buttons.size(); ++i) {
    const auto& btn = g_buttons[i];
    const bool disabled = (btn.requiresModel && !ui_model_loaded);
    const bool isActiveCmd = (btn.command == activeModeCommand);
    const bool isHover = (static_cast<int>(i) == g_hover_button);
    const bool isDown = (static_cast<int>(i) == g_active_button);

    float r = 0.18f, g = 0.21f, b = 0.28f;
    if (isActiveCmd) { r = 0.20f; g = 0.40f; b = 0.55f; }
    if (isHover)     { r += 0.08f; g += 0.08f; b += 0.08f; }
    if (isDown)      { r -= 0.05f; g -= 0.05f; b -= 0.05f; }
    if (disabled)    { r = g = b = 0.12f; }

    glColor3f(r, g, b);
    glBegin(GL_QUADS);
      glVertex2i(btn.x,         btn.y);
      glVertex2i(btn.x + btn.w, btn.y);
      glVertex2i(btn.x + btn.w, btn.y + btn.h);
      glVertex2i(btn.x,         btn.y + btn.h);
    glEnd();

    glColor3f(0.85f, 0.87f, 0.90f);
    glBegin(GL_LINE_LOOP);
      glVertex2i(btn.x,         btn.y);
      glVertex2i(btn.x + btn.w, btn.y);
      glVertex2i(btn.x + btn.w, btn.y + btn.h);
      glVertex2i(btn.x,         btn.y + btn.h);
    glEnd();

    void* font = GLUT_BITMAP_HELVETICA_12;
    int textWidth = 0;
    for (unsigned char c : btn.label) textWidth += glutBitmapWidth(font, c);
    glColor3f(disabled ? 0.50f : 1.0f, disabled ? 0.50f : 1.0f, disabled ? 0.50f : 1.0f);
    glRasterPos2i(btn.x + (btn.w - textWidth) / 2,
                  btn.y + (btn.h / 2) - 5);
    for (unsigned char c : btn.label) glutBitmapCharacter(font, c);
  }

  // HUD text.
  const char* modeLabel =
      (ui_draw_mode == 0) ? "Mode: POINTS" :
      (ui_draw_mode == 1) ? "Mode: WIREFRAME" : "Mode: SOLID";

  glColor3f(1.f, 1.f, 1.f);
  glRasterPos2i(10, h - 24);
  for (const char* c = modeLabel; *c; ++c)
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

  const std::string hint =
      "[L] Load  [D] Delete  [F1/F2/F3] Mode  [Right-Click] Menu  [Esc] Exit";
  glRasterPos2i(10, h - 48);
  for (unsigned char c : hint)
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
}
