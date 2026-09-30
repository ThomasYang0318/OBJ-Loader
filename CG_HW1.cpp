#ifdef __APPLE__
  // OpenGL and FreeGlut headers.
  #include <GL/glew.h>    // GLEW 在 macOS 仍可使用（需 brew install glew）
  #include <GLUT/glut.h>  // macOS 原生 GLUT
  // GLM.
  #include <glm/glm.hpp>
  #include <gtc/type_ptr.hpp>
  //GLUI
  #include <GL/glui.h>
  // tinyfiledialogs
  #include "tinyfiledialogs.h"
#else
  // OpenGL and FreeGlut headers.
  #include <glew.h>
  #include <freeglut.h>
  // GLM.
  #include <glm.hpp>
  #include <gtc/type_ptr.hpp>
  //GLUI
  #include <GL/glui.h>
  // tinyfiledialogs
  #include "tinyfiledialogs.h"
#endif
// C++ STL headers.
#include <iostream>
#include <algorithm>
#include <vector>
#include <iomanip>
// My headers.
#include "TriangleMesh.h"
#include "Interface.h"
// Global variables.
const int screenWidth = 650;
const int screenHeight = 600;
TriangleMesh* mesh = nullptr;
bool ui_model_loaded = 0;  // 介面用的旗標：是否已載入模型
// Function prototypes.
void SetupRenderState();
void SetupScene(const std::string&);
void ReleaseResources();
void RenderSceneCB();
void ReshapeCB(int, int);
void ProcessSpecialKeysCB(int, int, int);
void ProcessKeysCB(unsigned char, int, int);
// Callback function for glutDisplayFunc.
void RenderSceneCB()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Render the triangle mesh.
    // Add your code here.
    // 固定正交投影到整個視窗，避免任何殘留矩陣影響
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0,   // left, right
            -1.0, 1.0,   // bottom, top
            -1.0, 1.0);  // near, far
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    if (mesh && mesh->GetNumIndices() > 0) {
        glEnableClientState(GL_VERTEX_ARRAY);   // 使用 CreateBuffers 設過的頂點格式
        glDrawElements(GL_TRIANGLES,
                       (GLsizei)mesh->GetNumIndices(),
                       GL_UNSIGNED_INT,
                       (const void*)0);         // 從 IBO 開頭
        glDisableClientState(GL_VERTEX_ARRAY);
    }
    // Draw the HUD (buttons).
    DrawHUD();

    glutSwapBuffers();
}
// Callback function for glutReshapeFunc.
void ReshapeCB(int w, int h)
{
    // Adjust camera and projection here.
    // Implemented in HW2.
    //glViewport(0, 0, w, h);
    UpdateInterfaceSize(w, h);   // 讓按鈕依新視窗大小重新排版
}
// Callback function for glutSpecialFunc.
void ProcessSpecialKeysCB(int key, int x, int y)
{
    // Handle special (functional) keyboard inputs such as F1, spacebar, page up, etc. 
    switch (key) {
    case GLUT_KEY_F1:
        // Render with point mode.
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;
    case GLUT_KEY_F2:
        // Render with line mode.
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case GLUT_KEY_F3:
        // Render with fill mode.
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    default:
        break;
    }
}
// Callback function for glutKeyboardFunc.
void ProcessKeysCB(unsigned char key, int x, int y)
{
    // Handle other keyboard inputs those are not defined as special keys.
    if (key == 27) {
        // Release memory allocation if needed.
        ReleaseResources();
        exit(0);
    }
        
    // 'L' or 'l': load a model
    // 'D' or 'd': delete the model
    switch (key) {
    case 'l': case 'L':
    {
        const char* filters[] = { "*.obj" };
        const char* chosen = tinyfd_openFileDialog(
            "選擇模型檔案 (.obj)", "", 1, filters, nullptr, 0
        );
        if (!chosen) { std::cout << "[!] 使用者取消選擇\n"; break; }
        std::cout << "[*] loading: " << chosen << "\n";
        ReleaseResources();
        SetupScene(std::string(chosen));
        glutPostRedisplay();
        break;
    }
    case 'd': case 'D':
        ReleaseResources();
        std::cout << "[*] mesh deleted\n";
        glutPostRedisplay();
        break;
    default:
        break;
    }
}
void ReleaseResources()
{
    // Release memory if needed.
    // Add your code here.
    delete mesh;
    mesh = nullptr;
    ui_model_loaded = 0;
}
void SetupRenderState()
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glm::vec4 clearColor = glm::vec4(0.44f, 0.57f, 0.75f, 1.00f);
    glClearColor(
        (GLclampf)(clearColor.r), 
        (GLclampf)(clearColor.g), 
        (GLclampf)(clearColor.b), 
        (GLclampf)(clearColor.a)
    );
}
// Load a model from obj file and apply transformation.
// You can alter the parameters for dynamically loading a model.
void SetupScene(const std::string& modelPath)
{
    mesh = new TriangleMesh();
    mesh->LoadFromFile(modelPath);
 
    // Please DO NOT TOUCH the following code.
    // ------------------------------------------------------------------------
    // Build transformation matrices.
    // World.
    glm::mat4x4 M(1.0f);
    // Camera.
    glm::vec3 cameraPos = glm::vec3(0.0f, 0.5f, 2.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4x4 V = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    // Projection.
    float fov = 40.0f;
    float aspectRatio = (float)screenWidth / (float)screenHeight;
    float zNear = 0.1f;
    float zFar = 100.0f;
    glm::mat4x4 P = glm::perspective(glm::radians(fov), aspectRatio, zNear, zFar);
    // Apply CPU transformation.
    glm::mat4x4 MVP = P * V * M;
    
    mesh->ApplyTransformCPU(MVP);
    
    // Create and upload vertex/index buffers.
    mesh->CreateBuffers();
    ui_model_loaded = 1;  // 介面用的旗標：已載入模型
}
int main(int argc, char** argv)
{
    // Setting window properties.
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(screenWidth, screenHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("HW1: OBJ Loader");
    // Initialize GLEW.
    // Must be done after glut is initialized!
    GLenum res = glewInit();
    if (res != GLEW_OK) {
        std::cerr << "GLEW initialization error: " 
                  << glewGetErrorString(res) << std::endl;
        return 1;
    }
    // Initialization.
    SetupRenderState();
    //SetupScene("TODO: ADD FILE PATH");
    SetupScene("../TestModels_HW1/Gengar.obj");
    // 讓介面（HUD 按鈕）啟動
    InitInterface();

    // 介面處理（含滑鼠點按、滑過、鍵盤快捷鍵）
    glutKeyboardFunc(KeyboardCB);          // L / D / 1 / 2 / 3 / ESC
    glutMouseFunc(MouseButtonCB);          // 滑鼠左鍵點按
    glutMotionFunc(MouseMotionCB);         // 拖曳時的 hover 更新
    glutPassiveMotionFunc(MouseMotionCB);  // 滑過時的 hover 更新

    // Register callback functions.
    glutDisplayFunc(RenderSceneCB);
    glutIdleFunc(RenderSceneCB);
    glutReshapeFunc(ReshapeCB);
    glutSpecialFunc(ProcessSpecialKeysCB);
    glutKeyboardFunc(ProcessKeysCB);
    // Start rendering loop.
    glutMainLoop();
    return 0;
}