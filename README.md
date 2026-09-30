# OBJ Loader - 3D模型查看器

一個基於OpenGL的高效3D模型加載和可視化工具，支持OBJ文件格式，提供實時渲染和交互功能。

## 🎯 功能特性

- **OBJ文件加載**：支持加載標準OBJ格式的3D模型
- **實時渲染**：使用OpenGL進行高性能即時渲染
- **多種渲染模式**：
  - 頂點模式（F1）：顯示模型頂點
  - 線框模式（F2）：顯示模型輪廓
  - 填充模式（F3）：完整填充渲染
- **交互式界面**：直觀的HUD按鈕界面
- **跨平台支持**：支持macOS和Windows

## 📋 系統要求

### 依賴庫
- **OpenGL**：圖形API核心
- **GLEW**：OpenGL擴展加載庫
- **FreeGLUT/GLUT**：窗口和事件管理
- **GLM**：數學庫（矩陣計算）
- **GLUI**：用戶界面組件

### 編譯環境
- **macOS**：需要安裝 GLEW
  ```bash
  brew install glew glm
  ```
- **Windows**：Visual Studio 2019+ 或其他C++編譯器

## 🚀 安裝和編譯

### macOS
```bash
# 安裝依賴
brew install glew glm freeglut glui

# 編譯
make
```

### Windows
使用Visual Studio打開 `CG_HW1.vcxproj` 並編譯

## 📖 使用方法

### 快速開始
```bash
./CG_HW1
```

### 鍵盤快捷鍵

| 快捷鍵 | 功能 |
|--------|------|
| **L** | 加載模型文件（打開文件對話框選擇.obj文件） |
| **D** | 刪除當前模型 |
| **F1** | 切換到頂點渲染模式 |
| **F2** | 切換到線框渲染模式 |
| **F3** | 切換到填充渲染模式 |
| **ESC** | 退出程序 |

### 鼠標操作

- **左鍵點擊**：點擊HUD按鈕執行對應操作
- **鼠標移動**：按鈕悬停提示
- **拖曳**：支持模型交互

## 📁 項目結構

```
CG_HW1/
├── CG_HW1.cpp              # 主程序入口，渲染循環和事件處理
├── TriangleMesh.h/cpp      # 三角網格數據結構和OBJ加載邏輯
├── Interface.h/cpp         # HUD界面和用戶交互管理
├── tinyfiledialogs.h/c     # 文件對話框庫
├── Makefile                # macOS/Linux 編譯配置
├── CG_HW1.vcxproj          # Windows Visual Studio 項目文件
└── TestModels_HW1/         # 測試模型目錄
```

## 🔧 核心實現原理

### OBJ文件解析
- 逐行讀取OBJ文件
- 解析頂點位置(v)、法線(vn)、紋理坐標(vt)
- 構建面(f)索引表

### 3D變換管道
```
模型坐標系 → 世界坐標系 → 相機坐標系 → 裁剪坐標系 → 屏幕坐標系
           (M)       (V)        (P)        投影
```

### GPU渲染流程
1. **頂點緩衝對象 (VBO)**：存儲頂點數據（位置、法線、紋理坐標）
2. **索引緩衝對象 (IBO)**：存儲三角形面的索引
3. **頂點數組**：定義頂點屬性指針
4. **繪製調用**：`glDrawElements()` 渲染三角形

## 🎨 渲染管線

程序使用固定功能管線（Fixed Function Pipeline）：
- 正交投影用於UI（HUD按鈕）
- 透視投影用於3D模型
- CPU端執行MVP矩陣變換

## 📊 性能優化

- 使用頂點/索引緩衝減少CPU-GPU數據傳輸
- 索引繪製避免頂點數據重複
- 高效的OBJ文件解析（一次性讀取）

## 🐛 已知限制

- 僅支持三角形面（非四邊形）
- 不支持紋理映射（當前版本）
- 不支持骨骼動畫
- 沒有光照計算

## 📝 示例模型

項目包含 `TestModels_HW1/Gengar.obj` 作為示例模型

## 🔗 相關資源

- [OpenGL官方文檔](https://www.opengl.org/documentation/)
- [GLM - OpenGL數學庫](https://github.com/g-truc/glm)
- [Wavefront OBJ格式規範](https://en.wikipedia.org/wiki/Wavefront_.obj_file)

## 📄 許可證

MIT License - 詳見LICENSE文件

## 👤 作者

Thomas Yang

---

**最後更新**: 2026年9月
