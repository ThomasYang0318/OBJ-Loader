# 技術實現細節

本文檔詳細說明OBJ Loader的技術架構和實現方法。

## 目錄

1. [架構概覽](#架構概覽)
2. [OBJ文件解析](#obj文件解析)
3. [3D圖形渲染](#3d圖形渲染)
4. [對象模型](#對象模型)
5. [事件系統](#事件系統)
6. [跨平台處理](#跨平台處理)

---

## 架構概覽

### 模塊化設計

```
┌─────────────────────────────────────────────┐
│             CG_HW1.cpp                      │
│  主程序 - 事件循環和渲染控制                 │
└──────┬──────────────────────────┬────────────┘
       │                          │
       ▼                          ▼
┌──────────────────┐    ┌─────────────────────┐
│  TriangleMesh    │    │   Interface/HUD     │
│  - OBJ加載       │    │ - 按鈕管理          │
│  - 頂點變換      │    │ - 鼠標/鍵盤處理     │
│  - GPU緩衝       │    │ - 渲染UI元素        │
└──────────────────┘    └─────────────────────┘
```

### 核心職責分工

| 文件 | 責任 |
|------|------|
| **CG_HW1.cpp** | 主循環、OpenGL初始化、事件分發 |
| **TriangleMesh.h/cpp** | 模型管理、轉換、GPU上傳 |
| **Interface.h/cpp** | UI/交互、按鈕繪製、事件回調 |

---

## OBJ文件解析

### Wavefront OBJ格式結構

OBJ文件是文本格式，主要元素：

```
# 註釋
v  x y z          # 頂點位置
vn x y z          # 頂點法線
vt u v            # 紋理坐標
f v1 v2 v3        # 三角形面（簡化格式）
f v1/vt1/vn1 ... # 完整格式（位置/紋理/法線）
```

### 解析算法

`TriangleMesh::LoadFromFile()` 的實現流程：

```cpp
bool LoadFromFile(const std::string& filePath, bool normalized)
{
    std::ifstream file(filePath);
    std::string line;
    
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            positions.push_back({x, y, z});
        }
        else if (type == "vn") {
            float x, y, z;
            iss >> x >> y >> z;
            normals.push_back(glm::normalize({x, y, z}));
        }
        else if (type == "vt") {
            float u, v;
            iss >> u >> v;
            texcoords.push_back({u, v});
        }
        else if (type == "f") {
            // 解析面索引
            // 支持格式: v  或  v/vt  或  v/vt/vn
        }
    }
}
```

### 頂點組織

解析後構建 `VertexPTN` 結構體數組：

```cpp
struct VertexPTN
{
    glm::vec3 position;    // 4字節 × 3 = 12字節
    glm::vec3 normal;      // 4字節 × 3 = 12字節
    glm::vec2 texcoord;    // 4字節 × 2 = 8字節
};                         // 總計: 32字節/頂點
```

**頂點緩衝佈局** (Vertex Format)：
```
[P.x P.y P.z][N.x N.y N.z][T.u T.v]
   (4字節) (4字節) (4字節)
```

### 模型歸一化

當 `normalized=true` 時，執行以下步驟：

1. **計算邊界框 (Bounding Box)**
   ```
   min_x, max_x = 所有頂點X的最小/最大值
   min_y, max_y = 所有頂點Y的最小/最大值
   min_z, max_z = 所有頂點Z的最小/最大值
   ```

2. **計算中心和縮放**
   ```cpp
   glm::vec3 center = (min + max) / 2.0f;
   glm::vec3 scale = max - min;
   float maxScale = std::max({scale.x, scale.y, scale.z});
   ```

3. **變換至標準空間**
   ```
   v_normalized = (v - center) / maxScale * 2.0f
   // 結果: 模型在 [-1, 1]³ 立方體內
   ```

---

## 3D圖形渲染

### 變換管線 (Transformation Pipeline)

#### 1. 模型變換 (Model Transform)

```cpp
glm::mat4x4 M(1.0f);  // 單位矩陣
// 可在此添加：
// M = glm::rotate(...);
// M = glm::scale(...);
// M = glm::translate(...);
```

#### 2. 相機變換 (View Transform)

```cpp
glm::vec3 cameraPos = glm::vec3(0.0f, 0.5f, 2.0f);     // 相機位置
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);  // 目標位置
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);      // 向上向量

glm::mat4x4 V = glm::lookAt(cameraPos, cameraTarget, cameraUp);
// 構建right, up, forward向量，生成旋轉+平移矩陣
```

#### 3. 投影變換 (Projection Transform)

```cpp
float fov = 40.0f;                              // 視野角度
float aspectRatio = (float)screenWidth / (float)screenHeight;
float zNear = 0.1f;                            // 近平面
float zFar = 100.0f;                           // 遠平面

glm::mat4x4 P = glm::perspective(
    glm::radians(fov),  // 轉換為弧度
    aspectRatio,
    zNear,
    zFar
);
```

**透視投影矩陣**：
```
⎡ 1/(ar·tan(fov/2))    0          0              0    ⎤
⎢       0        1/tan(fov/2)     0              0    ⎥
⎢       0             0      -(f+n)/(f-n)   -2fn/(f-n)⎥
⎣       0             0           -1              0    ⎦
```

#### 4. MVP矩陣組合

```cpp
glm::mat4x4 MVP = P * V * M;  // 左乘順序很重要
// 應用到頂點：v_clip = MVP * v_model
```

### CPU端頂點變換

```cpp
void ApplyTransformCPU(const glm::mat4x4& mvpMatrix)
{
    for (int i = 0; i < vertices.size(); ++i)
    {
        glm::vec4 pos(vertices[i].position, 1.0f);
        glm::vec4 transformedPos = mvpMatrix * pos;
        
        // 透視除法
        float w = transformedPos.w;
        vertices[i].position = glm::vec3(
            transformedPos.x / w,
            transformedPos.y / w,
            transformedPos.z / w
        );
    }
}
```

### GPU頂點緩衝設置

#### 創建VBO和IBO

```cpp
void CreateBuffers()
{
    // 1. 創建VBO
    glGenBuffers(1, &vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(VertexPTN),
        vertices.data(),
        GL_STATIC_DRAW
    );
    
    // 2. 創建IBO
    glGenBuffers(1, &iboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        vertexIndices.size() * sizeof(unsigned int),
        vertexIndices.data(),
        GL_STATIC_DRAW
    );
    
    // 3. 設置頂點指針
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(VertexPTN), (void*)0);
}
```

#### 渲染調用

```cpp
void RenderSceneCB()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 設置矩陣
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    
    // 渲染模型
    if (mesh && mesh->GetNumIndices() > 0) {
        glDrawElements(
            GL_TRIANGLES,                    // 繪製模式
            mesh->GetNumIndices(),           // 索引個數
            GL_UNSIGNED_INT,                 // 索引類型
            (const void*)0                   // IBO偏移
        );
    }
    
    glutSwapBuffers();
}
```

### 渲染模式

透過 `glPolygonMode()` 改變渲染模式：

```cpp
// 頂點模式：只顯示頂點
glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

// 線框模式：顯示邊界
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

// 填充模式：完整填充
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
```

---

## 對象模型

### TriangleMesh 類設計

```cpp
class TriangleMesh
{
private:
    GLuint vboId;                          // 頂點緩衝對象ID
    GLuint iboId;                          // 索引緩衝對象ID
    std::vector<VertexPTN> vertices;       // 頂點數據
    std::vector<unsigned int> vertexIndices; // 面索引
    
    int numVertices;                       // 頂點數
    int numTriangles;                      // 三角形數
    glm::vec3 objCenter;                   // 模型中心
    
public:
    // 生命週期
    TriangleMesh();
    ~TriangleMesh();
    
    // 核心功能
    bool LoadFromFile(const std::string& filePath, bool normalized = true);
    void CreateBuffers();
    void ApplyTransformCPU(const glm::mat4x4& mvpMatrix);
    
    // 查詢
    int GetNumVertices() const;
    int GetNumTriangles() const;
    int GetNumIndices() const;
    glm::vec3 GetObjCenter() const;
};
```

### 生命週期管理

```cpp
// 構造
TriangleMesh* mesh = new TriangleMesh();

// 加載模型
mesh->LoadFromFile("model.obj", true);

// 應用變換並上傳GPU
mesh->ApplyTransformCPU(mvpMatrix);
mesh->CreateBuffers();

// 銷毀
delete mesh;
mesh = nullptr;
```

---

## 事件系統

### 事件流程

```
GLUT事件 → 回調函數 → 應用邏輯
```

### 鍵盤事件

```cpp
void ProcessKeysCB(unsigned char key, int x, int y)
{
    switch (key) {
    case 'L':  // 加載模型
        tinyfd_openFileDialog(...);
        break;
    case 'D':  // 刪除模型
        delete mesh;
        mesh = nullptr;
        break;
    case 27:   // ESC - 退出
        exit(0);
        break;
    }
    glutPostRedisplay();  // 觸發重繪
}

void ProcessSpecialKeysCB(int key, int x, int y)
{
    switch (key) {
    case GLUT_KEY_F1:
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;
    case GLUT_KEY_F2:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case GLUT_KEY_F3:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    }
}
```

### 鼠標事件

```cpp
void MouseButtonCB(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // 檢查點擊是否在按鈕區域內
        for (const auto& btn : buttons) {
            if (btn.IsInside(x, y)) {
                btn.OnClick();
                break;
            }
        }
    }
}

void MouseMotionCB(int x, int y)
{
    // 更新按鈕hover狀態
    for (auto& btn : buttons) {
        btn.SetHovered(btn.IsInside(x, y));
    }
}
```

### 事件後的重繪

```cpp
glutPostRedisplay();  // 標記窗口需要重繪
// → glutDisplayFunc 自動被調用
// → RenderSceneCB() 執行
```

---

## 跨平台處理

### 條件編譯

```cpp
#ifdef __APPLE__
    // macOS 特定包含
    #include <GL/glew.h>
    #include <GLUT/glut.h>
#else
    // Windows/Linux 特定包含
    #include <glew.h>
    #include <freeglut.h>
#endif
```

### 平台差異

| 功能 | macOS | Windows |
|------|-------|---------|
| OpenGL header | `<GL/glew.h>` | `<glew.h>` |
| GLUT header | `<GLUT/glut.h>` | `<GL/freeglut.h>` |
| 編譯工具 | Clang/Make | MSVC/VS |
| 依賴安裝 | Homebrew | vcpkg/手動 |

---

## 性能特性

### 時間複雜度

| 操作 | 複雜度 | 備註 |
|------|--------|------|
| OBJ解析 | O(V + F) | V=頂點數, F=面數 |
| CPU變換 | O(V) | 頂點級別變換 |
| GPU上傳 | O(V + I) | I=索引數 |
| 繪製 | O(F) | 三角形光柵化 |

### 內存佈局

```
頂點緩衝 (VBO):
┌─────────────────────┐
│ VertexPTN[0]        │ (32字節)
│ VertexPTN[1]        │ (32字節)
│ ...                 │
│ VertexPTN[N-1]      │ (32字節)
└─────────────────────┘

索引緩衝 (IBO):
┌─────────────────────┐
│ unsigned int[0]     │ (4字節)
│ unsigned int[1]     │ (4字節)
│ ...                 │
│ unsigned int[M-1]   │ (4字節)
└─────────────────────┘
```

### 優化機會

1. **使用VAO (Vertex Array Object)** - 簡化狀態管理
2. **實裝著色器** - 利用可編程管線
3. **法線計算** - 提高光照效果
4. **紋理映射** - 增強視覺效果
5. **剔除算法** - 背面和視錐剔除

---

## 調試技巧

### 打印網格信息

```cpp
void PrintMeshInfo() const
{
    std::cout << "Mesh Info:" << std::endl;
    std::cout << "  Vertices: " << numVertices << std::endl;
    std::cout << "  Triangles: " << numTriangles << std::endl;
    std::cout << "  Center: (" << objCenter.x << ", " 
              << objCenter.y << ", " << objCenter.z << ")" << std::endl;
}
```

### OpenGL錯誤檢查

```cpp
GLenum err;
while ((err = glGetError()) != GL_NO_ERROR) {
    std::cerr << "OpenGL Error: " << err << std::endl;
}
```

### 關鍵值驗證

```cpp
assert(numVertices > 0);
assert(vertexIndices.size() % 3 == 0);  // 必須是3的倍數
assert(vboId != 0);  // VBO已創建
```

---

## 相關標準和規範

- **OpenGL 1.3+** - 核心圖形API
- **Wavefront OBJ** - 3D模型格式規範
- **GLM** - GLSL數學庫（與著色器兼容）

---

**文檔版本**: 1.0  
**最後更新**: 2026年9月
