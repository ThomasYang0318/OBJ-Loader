#include "TriangleMesh.h"


// Desc: Constructor of a triangle mesh.
TriangleMesh::TriangleMesh()
{
	numVertices = 0;
	numTriangles = 0;
	objCenter = glm::vec3(0.0f, 0.0f, 0.0f);
	vboId = 0;
	iboId = 0;
}

// Desc: Destructor of a triangle mesh.
TriangleMesh::~TriangleMesh()
{
	vertices.clear();
	vertexIndices.clear();
	glDeleteBuffers(1, &vboId);
	glDeleteBuffers(1, &iboId);
}

// Desc: Load the geometry data of the model from file and normalize it.
bool TriangleMesh::LoadFromFile(const std::string& filePath, const bool normalized)
{	
	// Add your code here.
	vertices.clear();
    vertexIndices.clear();

    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[TriangleMesh] Failed to open file: " << filePath << std::endl;
        return false;
    }

	std::vector<glm::vec3> posPool;
    std::vector<glm::vec2> texPool;
    std::vector<glm::vec3> nrmPool;
    std::string line;

    // 用來記錄已出現過的 (v/vt/vn) 組合
    std::unordered_map<std::string, unsigned> remap;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        // 頂點座標
        if (tag == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            posPool.push_back({x, y, z});
        }
        // 貼圖座標
        else if (tag == "vt") {
            float u, v; 
            iss >> u >> v;
            texPool.push_back({u, v});
        }
        // 法向量
        else if (tag == "vn") {
            float x, y, z; 
            iss >> x >> y >> z;
            nrmPool.push_back({x, y, z});
        }
  
        //面
        else if (tag == "f") {
            std::vector<unsigned> faceIdx;   // 這一個面的索引們
            std::string token;

            while (iss >> token) {
                // 以原字串 (e.g. "12/5/7") 當 key，達成 (v,vt,vn) 共享
                auto it = remap.find(token);
                unsigned idx;                // 單一索引的變數（不是 vector）

                if (it != remap.end()) {
                    // 已存在：直接重用
                    idx = it->second;
                } else {
                    // 解析 token 成 v/vt/vn（不處理負索引）
                    int v = 0, vt = 0, vn = 0;
                    if (sscanf(token.c_str(), "%d/%d/%d", &v, &vt, &vn) == 3) {
                    }
                    else if (sscanf(token.c_str(), "%d//%d",   &v, &vn) == 2) { 
                        vt = 0; 
                    }
                    else if (sscanf(token.c_str(), "%d/%d",    &v, &vt) == 2) { 
                        vn = 0; 
                    }
                    else { 
                        sscanf(token.c_str(), "%d", &v); 
                        vt = 0; 
                        vn = 0; 
                    }

                    // OBJ 是 1-based → 改成 0-based
                    v  = v  - 1;
                    vt = vt - 1;
                    vn = vn - 1;

                    VertexPTN vert{};
                    if (v  >= 0 && v  < (int)posPool.size()) vert.position = posPool[v];
                    if (vt >= 0 && vt < (int)texPool.size()) vert.texcoord = texPool[vt];
                    if (vn >= 0 && vn < (int)nrmPool.size()) vert.normal   = nrmPool[vn];

                    vertices.push_back(vert);
                    idx = (unsigned)vertices.size() - 1;
                    remap[token] = idx;  // 記住這組 (v/vt/vn)
                }

                faceIdx.push_back(idx);  // ← 把這個點加入本面
            }

            // 多邊形三角化
            for (size_t i = 1; i + 1 < faceIdx.size(); ++i) {
                vertexIndices.push_back(faceIdx[0]);
                vertexIndices.push_back(faceIdx[i]);
                vertexIndices.push_back(faceIdx[i + 1]);
            }
        }
    }
    file.close();

    numVertices  = (int)vertices.size();
    numTriangles = (int)vertexIndices.size() / 3;

	if (normalized) {
        // Normalize the geometry data.	
		// Add your code here.
        // 找出模型範圍
        glm::vec3 minP(FLT_MAX), maxP(-FLT_MAX);
        for (auto &v : vertices) {
            minP = glm::min(minP, v.position);
            maxP = glm::max(maxP, v.position);
        }

        // 中心點 & 尺寸
        glm::vec3 center = (minP + maxP) * 0.5f;
        glm::vec3 extent = maxP - minP;
        float maxExtent = std::max(std::max(extent.x, extent.y), extent.z);
        float scale = 1.0f / maxExtent;


        // 平移到原點並縮放
        for (auto &v : vertices)
            v.position = (v.position - center) * scale;
        
        // 更新中心（已在原點）
        objCenter = glm::vec3(0);
	}

	PrintMeshInfo();
	return true;
}

// Desc: Create vertex buffer and index buffer.
void TriangleMesh::CreateBuffers()
{
	// Add your code here.
    if (vertices.empty() || vertexIndices.empty()) return; // 沒資料就不做

    // 建立 VBO / IBO
    if (!vboId) glGenBuffers(1, &vboId);
    if (!iboId) glGenBuffers(1, &iboId);

    // 上傳頂點資料
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(vertices.size() * sizeof(VertexPTN)),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // 上傳索引
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(vertexIndices.size() * sizeof(unsigned int)),
                 vertexIndices.data(),
                 GL_STATIC_DRAW);

    // 告訴 OpenGL 頂點格式（用 offsetof 當作 VBO 內的位移量）
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, (GLsizei)sizeof(VertexPTN),
                    (const void*)offsetof(VertexPTN, position));

	/*
	// Debug: 列印三角形索引
	std::cout << "\n[Triangle Indices] (GL_UNSIGNED_INT)\n";
	for (size_t i = 0; i + 2 < vertexIndices.size(); i += 3) {
    	std::cout << "tri " << (i/3) << ": ("
              	  << vertexIndices[i]   << ", "
                  << vertexIndices[i+1] << ", "
                  << vertexIndices[i+2] << ")\n";
	}
	*/
}

// Desc: Apply transformation to all vertices (DON'T NEED TO TOUCH)
void TriangleMesh::ApplyTransformCPU(const glm::mat4x4& mvpMatrix)
{
	for (int i = 0 ; i < numVertices; ++i) {
        glm::vec4 p = mvpMatrix * glm::vec4(vertices[i].position, 1.0f);
        if (p.w != 0.0f) {
            float inv = 1.0f / p.w; 
            vertices[i].position.x = p.x * inv;
            vertices[i].position.y = p.y * inv;
            vertices[i].position.z = p.z * inv;
        }
    }
}

// Desc: Print mesh information.
void TriangleMesh::PrintMeshInfo() const
{
	std::cout << "[*] Mesh Information: " << std::endl;
	std::cout << "# Vertices: " << numVertices << std::endl;
	std::cout << "# Triangles: " << numTriangles << std::endl;
	std::cout << "Center: (" << objCenter.x << " , " << objCenter.y << " , " << objCenter.z << ")" << std::endl;
}
