
#include "terrain.hpp"


Chunk::Chunk(glm::vec3 center, float sideXSize, unsigned numVertexX, unsigned numVertexY)
    : center(center), sideXSize(sideXSize), numVertexX(numVertexX), numVertexY(numVertexY), modelOrdered(false), visible(false) { }

Chunk::Chunk(std::tuple<float, float, float> center, float sideXSize, unsigned numVertexX, unsigned numVertexY)
    : sideXSize(sideXSize), numVertexX(numVertexX), numVertexY(numVertexY), modelOrdered(false), visible(false)
{ 
    this->center.x = std::get<0>(center);
    this->center.y = std::get<1>(center);
    this->center.z = std::get<2>(center);
}

void Chunk::reset(glm::vec3 center, float sideXSize, unsigned numVertexX, unsigned numVertexY)
{
    this->center = center;
    this->sideXSize = sideXSize;
    this->numVertexX = numVertexX;
    this->numVertexY = numVertexY;
    vertex.clear();
    indices.clear();
}

void Chunk::reset(std::tuple<float, float, float> center, float sideXSize, unsigned numVertexX, unsigned numVertexY)
{
    this->center.x = std::get<0>(center);
    this->center.y = std::get<1>(center);
    this->center.z = std::get<2>(center);
    this->sideXSize = sideXSize;
    this->numVertexX = numVertexX;
    this->numVertexY = numVertexY;
    vertex.clear();
    indices.clear();
}

void Chunk::computeTerrain(Noiser& noise, bool computeIndices, float textureFactor)
{
    float stride = sideXSize / (numVertexX - 1);
    float x0 = center.x - sideXSize / 2;
    float y0 = center.y - (stride * (numVertexY - 1)) / 2;

    // Vertex data
    vertex.reserve(numVertexX * numVertexY * 8);

    for (size_t y = 0; y < numVertexY; y++)
        for (size_t x = 0; x < numVertexX; x++)
        {
            size_t pos = y * numVertexX + x;

            // positions (0, 1, 2)
            vertex[pos * 8 + 0] = x0 + x * stride;
            vertex[pos * 8 + 1] = y0 + y * stride;
            vertex[pos * 8 + 2] = noise.GetNoise((float)vertex[pos * 8 + 0], (float)vertex[pos * 8 + 1]);

            // textures (3, 4)
            vertex[pos * 8 + 3] = x * textureFactor;
            vertex[pos * 8 + 4] = y * textureFactor;     // LOOK produces textures reflected in the x-axis
        }

    // Normals (5, 6, 7)
    computeGridNormals(stride, noise);

    // Indices
    if (computeIndices)
    {
        indices.reserve((numVertexX - 1) * (numVertexY - 1) * 2 * 3);

        for (size_t y = 0; y < numVertexY - 1; y++)
            for (size_t x = 0; x < numVertexX - 1; x++)
            {
                unsigned int pos = getPos(x, y);

                indices.push_back(pos);
                indices.push_back(pos + numVertexX + 1);
                indices.push_back(pos + numVertexX);

                indices.push_back(pos);
                indices.push_back(pos + 1);
                indices.push_back(pos + numVertexX + 1);
            }
    }
}

void Chunk::render(Renderer* app, std::vector<texIterator> &usedTextures, std::vector<uint32_t>* indices)
{
    VertexLoader* vertexLoader = new VertexFromUser(
        VertexType(1, 0, 1, 1), 
        numVertexX * numVertexY, 
        vertex.data(), 
        indices? *indices : this->indices, 
        true);
    
    model = app->newModel(
        1, 1, primitiveTopology::triangle,
        vertexLoader,
        UBOconfig(1, MMsize, VMsize, PMsize, MMNsize),
        UBOconfig(1, lightSize, vec4size),
        usedTextures,
        (SHADERS_DIR + "v_terrainPTN.spv").c_str(),
        (SHADERS_DIR + "f_terrainPTN.spv").c_str(),
        false);

    model->vsDynUBO.setUniform(0, 0, modelMatrix());
    //model->vsDynUBO.setUniform(i, 1, view);
    //model->vsDynUBO.setUniform(i, 2, proj);
    model->vsDynUBO.setUniform(0, 3, modelMatrixForNormals(modelMatrix()));

    //sun.turnOff();
    sun.setDirectional(glm::vec3(-2, 2, 1), glm::vec3(0.1, 0.1, 0.1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1));
    //sun.setPoint(glm::vec3(0, 0, 50), glm::vec3(0.1, 0.1, 0.1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), 1, 0.1, 0.01);
    //sun.setSpot(glm::vec3(0, 0, 150), glm::vec3(0, 0, 1), glm::vec3(0.1, 0.1, 0.1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), 1, 0, 0., 0.9, 0.8);
    model->fsUBO.setUniform(0, 0, sun);
    //model->fsUBO.setUniform(0, 1, camPos);

    modelOrdered = true;
}

void Chunk::updateUBOs(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj)
{
    if (!modelOrdered) return;

    for (size_t i = 0; i < model->vsDynUBO.dynBlocksCount; i++) {
        model->vsDynUBO.setUniform(i, 1, view);
        model->vsDynUBO.setUniform(i, 2, proj);
    }
    model->fsUBO.setUniform(0, 1, camPos);
}

void Chunk::computeIndices(std::vector<uint32_t>& indices)
{
    indices.reserve((numVertexX - 1) * (numVertexY - 1) * 2 * 3);

    for (size_t y = 0; y < numVertexY - 1; y++)
        for (size_t x = 0; x < numVertexX - 1; x++)
        {
            unsigned int pos = getPos(x, y);

            indices.push_back(pos);
            indices.push_back(pos + numVertexX + 1);
            indices.push_back(pos + numVertexX);

            indices.push_back(pos);
            indices.push_back(pos + 1);
            indices.push_back(pos + numVertexX + 1);
        }
}

void Chunk::computeGridNormals(float stride, Noiser& noise)
{
    // Initialize normals to 0
    unsigned numVertex = numVertexX * numVertexY;
    glm::vec3* tempNormals = new glm::vec3[numVertex];
    for (size_t i = 0; i < numVertex; i++) tempNormals[i] = glm::vec3(0.f, 0.f, 0.f);

    // Compute normals
    for (size_t y = 0; y < numVertexY - 1; y++)
        for (size_t x = 0; x < numVertexX - 1; x++)
        {
            /*
                In each iteration, we operate in each square of the grid (4 vertex):

                           Cside
                       (D)------>(C)
                        |         |
                  Dside |         | Bside
                        v         v
                       (A)------>(B)
                           Aside
             */

             // Vertex positions in the array
            size_t posA = getPos(x, y);
            size_t posB = getPos(x + 1, y);
            size_t posC = getPos(x + 1, y + 1);
            size_t posD = getPos(x, y + 1);

            // Vertex vectors
            glm::vec3 A = getVertex(posA);
            glm::vec3 B = getVertex(posB);
            glm::vec3 C = getVertex(posC);
            glm::vec3 D = getVertex(posD);

            // Vector representing each side
            glm::vec3 Aside = B - A;
            glm::vec3 Bside = B - C;
            glm::vec3 Cside = C - D;
            glm::vec3 Dside = A - D;

            // Normal computed for each vertex from the two side vectors it has attached
            glm::vec3 Anormal = glm::cross(Aside, -Dside);
            glm::vec3 Bnormal = glm::cross(-Bside, -Aside);
            glm::vec3 Cnormal = glm::cross(-Cside, Bside);
            glm::vec3 Dnormal = glm::cross(Dside, Cside);

            // Add to the existing normal of the vertex
            tempNormals[posA] += Anormal;
            tempNormals[posB] += Bnormal;
            tempNormals[posC] += Cnormal;
            tempNormals[posD] += Dnormal;
        }

    // Special cases: Vertex at the border

    // Left & Right
    for (size_t y = 1; y < numVertexY - 1; y++)
    {
        size_t pos;
        glm::vec3 up, down, left, right, center;

        // Left side:
        //     -Vertex vectors
        pos = getPos(0, y);
        center = getVertex(pos);
        up = getVertex(getPos(0, y + 1));
        down = getVertex(getPos(0, y - 1));
        left = glm::vec3(center.x - stride, center.y, noise.GetNoise(center.x - stride, center.y));

        //     -Vector representing each side
        up = up - center;
        left = left - center;
        down = down - center;

        //     -Add normals to the existing normal of the vertex
        tempNormals[pos] += glm::cross(up, left) + glm::cross(left, down);

        // Right side:
        //     -Vertex vectors
        pos = getPos(numVertexX - 1, y);
        center = getVertex(pos);
        up = getVertex(getPos(numVertexX - 1, y + 1));
        down = getVertex(getPos(numVertexX - 1, y - 1));
        right = glm::vec3(center.x + stride, center.y, noise.GetNoise(center.x + stride, center.y));

        //     -Vector representing each side
        up = up - center;
        right = right - center;
        down = down - center;

        //     -Add normals to the existing normal of the vertex
        tempNormals[pos] += glm::cross(right, up) + glm::cross(down, right);
    }

    // Upper & Bottom
    for (size_t x = 1; x < numVertexX - 1; x++)
    {
        size_t pos;
        glm::vec3 up, down, left, right, center;

        // Bottom side:
        //     -Vertex vectors
        pos = getPos(x, 0);
        center = getVertex(pos);
        right = getVertex(getPos(x + 1, 0));
        left = getVertex(getPos(x - 1, 0));
        down = glm::vec3(center.x, center.y - stride, noise.GetNoise(center.x, center.y - stride));

        //     -Vector representing each side
        right = right - center;
        left = left - center;
        down = down - center;

        //     -Add normals to the existing normal of the vertex
        tempNormals[pos] += glm::cross(left, down) + glm::cross(down, right);

        // Upper side:
        //     -Vertex vectors
        pos = getPos(x, numVertexY - 1);
        center = getVertex(pos);
        right = getVertex(getPos(x + 1, numVertexY - 1));
        left = getVertex(getPos(x - 1, numVertexY - 1));
        up = glm::vec3(center.x, center.y + stride, noise.GetNoise(center.x, center.y + stride));

        //     -Vector representing each side
        right = right - center;
        left = left - center;
        up = up - center;

        //     -Add normals to the existing normal of the vertex
        tempNormals[pos] += glm::cross(up, left) + glm::cross(right, up);
    }

    // Corners
    glm::vec3 center, right, left, up, down;
    size_t pos;

    //  - Top left
    pos = getPos(0, numVertexY - 1);
    center = getVertex(pos);
    right = getVertex(getPos(1, numVertexY - 1));
    down = getVertex(getPos(0, numVertexY - 2));
    up = glm::vec3(center.x, center.y + stride, noise.GetNoise(center.x, center.y + stride));
    left = glm::vec3(center.x - stride, center.y, noise.GetNoise(center.x - stride, center.y));

    right = right - center;
    left = left - center;
    up = up - center;
    down = down - center;

    tempNormals[pos] += glm::cross(right, up) + glm::cross(up, left) + glm::cross(left, down);

    //  - Top right
    pos = getPos(numVertexX - 1, numVertexY - 1);
    center = getVertex(pos);
    down = getVertex(getPos(numVertexX - 1, numVertexY - 2));
    left = getVertex(getPos(numVertexX - 2, numVertexY - 1));
    right = glm::vec3(center.x + stride, center.y, noise.GetNoise(center.x + stride, center.y));
    up = glm::vec3(center.x, center.y + stride, noise.GetNoise(center.x, center.y + stride));

    right = right - center;
    left = left - center;
    up = up - center;
    down = down - center;

    tempNormals[pos] += glm::cross(down, right) + glm::cross(right, up) + glm::cross(up, left);

    //  - Low left
    pos = getPos(0, 0);
    center = getVertex(pos);
    right = getVertex(getPos(1, 0));
    up = getVertex(getPos(0, 1));
    down = glm::vec3(center.x, center.y - stride, noise.GetNoise(center.x, center.y - stride));
    left = glm::vec3(center.x - stride, center.y, noise.GetNoise(center.x - stride, center.y));

    right = right - center;
    left = left - center;
    up = up - center;
    down = down - center;

    tempNormals[pos] += glm::cross(up, left) + glm::cross(left, down) + glm::cross(down, right);

    //  - Low right
    pos = getPos(numVertexX - 1, 0);
    center = getVertex(pos);
    up = getVertex(getPos(numVertexX - 1, 1));
    left = getVertex(getPos(numVertexX - 2, 0));
    right = glm::vec3(center.x + stride, center.y, noise.GetNoise(center.x + stride, center.y));
    down = glm::vec3(center.x, center.y - stride, noise.GetNoise(center.x, center.y - stride));

    right = right - center;
    left = left - center;
    up = up - center;
    down = down - center;

    tempNormals[pos] += glm::cross(left, down) + glm::cross(down, right) + glm::cross(right, up);

    // Normalize the normals
    for (int i = 0; i < numVertex; i++)
    {
        tempNormals[i] = glm::normalize(tempNormals[i]);

        vertex[i * 8 + 5] = tempNormals[i].x;
        vertex[i * 8 + 6] = tempNormals[i].y;
        vertex[i * 8 + 7] = tempNormals[i].z;
    }

    delete[] tempNormals;
}

size_t Chunk::getPos(size_t x, size_t y) const { return y * numVertexX + x; }

glm::vec3 Chunk::getVertex(size_t position) const
{
    return glm::vec3(vertex[position * 8 + 0], vertex[position * 8 + 1], vertex[position * 8 + 2]);
}


TerrainGrid::TerrainGrid(Noiser noiseGenerator, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier)
    : root(nullptr), app(nullptr), noiseGenerator(noiseGenerator), camPos(glm::vec3(0.1f,0.1f,0.1f)), nodeCount(0), leafCount(0), rootCellSize(rootCellSize), numSideVertex(numSideVertex), numLevels(numLevels), minLevel(minLevel), distMultiplier(distMultiplier) 
{ 
    Chunk temp(glm::vec3(0, 0, 0), 1, numSideVertex, numSideVertex);
    temp.computeIndices(indices);
}

TerrainGrid::~TerrainGrid() 
{ 
    delete root;
    chunks.clear();
}

void TerrainGrid::addApp(Renderer& app) { this->app = &app; }

void TerrainGrid::addTextures(const std::vector<texIterator>& textures) { this->textures = textures; }

void TerrainGrid::updateTree(glm::vec3 newCamPos)
{
    //std::cout << app->getFrameCount() << std::endl;
    if (camPos == newCamPos || !app || !textures.size() || !numLevels) return;  //<<< quitar camPos==newCamPos

    camPos = newCamPos;
    nodeCount = 0;
    leafCount = 0;
    if (root) delete root;

    // Create root node and chunk
    std::tuple<float, float, float> center = closestCenter();
    root = getNode(center, rootCellSize);

    // Mark all chunks as non-visible, and remove out-of-range chunks
    float maxX = std::get<0>(center) + rootCellSize / 2;
    float maxY = std::get<1>(center) + rootCellSize / 2;
    float minX = std::get<0>(center) - rootCellSize / 2;
    float minY = std::get<1>(center) - rootCellSize / 2;
    std::vector<std::tuple<float, float, float>> deletionKeys;

    for (auto it = chunks.begin(); it != chunks.end(); it++)
    {
        it->second->visible = false;

        if (it->second->getCenter().x < minX || it->second->getCenter().x > maxX ||
            it->second->getCenter().y < minY || it->second->getCenter().y > maxY )
        {
            app->deleteModel(it->second->model);
            deletionKeys.push_back(it->first);
        }
    }
    for(auto key : deletionKeys) chunks.erase(key);

    // Build tree and load leaf-chunks
    updateTree_help(root, 0);

    // Un-render non-visible chunks
    for (auto it = chunks.begin(); it != chunks.end(); it++)
        if (it->second->modelOrdered && it->second->visible == false)
            app->setRenders(it->second->model, 0);
}

void TerrainGrid::updateTree_help(QuadNode<Chunk*> *node, size_t depth)
{
    nodeCount++;
    Chunk* chunk = node->getElement();
    glm::vec3 center = chunk->getCenter();
    float squareSide = chunk->getSide() * chunk->getSide();
    float squareDist = (camPos.x - center.x) * (camPos.x - center.x) + (camPos.y - center.y) * (camPos.y - center.y);

    /*
        Avoid chunks-swap gap. Cases:
            - Childs required but not loaded yet: Check if childs are leaf > If childs not loaded, load parent.
                Recursive call to childs must return "leafButNotLoaded" > Then, parent is rendered.
            - Parent required but not loaded yet: Check if this node is leaf > If not loaded, load childs.
                Check whether childs exist > If so, render them.
    */
    
    if (depth >= minLevel && (squareDist > squareSide * distMultiplier || depth == numLevels - 1))  // Is leaf node > Compute terrain > Children are nullptr by default
    {
        leafCount++;

        if (chunk->modelOrdered == false)
        {
            chunk->computeTerrain(noiseGenerator, false, std::pow(2, numLevels - 1 - depth));
            chunk->render(app, textures, &indices);
        }

        app->setRenders(chunk->model, 1);

        chunk->visible = true;
     }
    else // Is not leaf node > Create children > Recursion
    {
        depth++;
        
        std::tuple<float, float, float> newCenter;
        float halfSide = chunk->getSide() / 2;
        float quarterSide = chunk->getSide() / 4;

        node->setA(getNode(std::tuple(center.x - quarterSide, center.y + quarterSide, 0), halfSide));
        updateTree_help(node->getA(), depth);

        node->setB(getNode(std::tuple(center.x + quarterSide, center.y + quarterSide, 0), halfSide));
        updateTree_help(node->getB(), depth);

        node->setC(getNode(std::tuple(center.x - quarterSide, center.y - quarterSide, 0), halfSide));
        updateTree_help(node->getC(), depth);

        node->setD(getNode(std::tuple(center.x + quarterSide, center.y - quarterSide, 0), halfSide));
        updateTree_help(node->getD(), depth);

        /*
        newCenter = { center.x - quarterSide, center.y + quarterSide, 0};
        if (chunks.find(newCenter) == chunks.end())
            chunks[newCenter] = new Chunk(newCenter, halfSide, numSideVertex, numSideVertex);
        node->setA(new QuadNode<Chunk*>(chunks[newCenter]));
        updateTree_help(node->getA(), depth);

        newCenter = { center.x + quarterSide, center.y + quarterSide, 0 };
        if (chunks.find(newCenter) == chunks.end())
            chunks[newCenter] = new Chunk(newCenter, halfSide, numSideVertex, numSideVertex);
        node->setB(new QuadNode<Chunk*>(chunks[newCenter]));
        updateTree_help(node->getB(), depth);

        newCenter = { center.x - quarterSide, center.y - quarterSide, 0 };
        if (chunks.find(newCenter) == chunks.end())
            chunks[newCenter] = new Chunk(newCenter, halfSide, numSideVertex, numSideVertex);
        node->setC(new QuadNode<Chunk*>(chunks[newCenter]));
        updateTree_help(node->getC(), depth);

        newCenter = { center.x + quarterSide, center.y - quarterSide, 0 };
        if (chunks.find(newCenter) == chunks.end())
            chunks[newCenter] = new Chunk(newCenter, halfSide, numSideVertex, numSideVertex);
        node->setD(new QuadNode<Chunk*>(chunks[newCenter]));
        updateTree_help(node->getD(), depth);
        */
    }
}

void TerrainGrid::updateUBOs(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj)
{
    this->camPos = camPos;
    this->view = view;
    this->proj = proj;

    //preorder<Chunk*, void (QuadNode<Chunk*>*)>(root, nodeVisitor);
    updateUBOs_help(root);  // Preorder traversal
}

void TerrainGrid::updateUBOs_help(QuadNode<Chunk*>* node)
{
    if (!node) return;

    if (node->isLeaf())
    {
        node->getElement()->updateUBOs(camPos, view, proj);
        return;
    }

    updateUBOs_help(node->getA());
    updateUBOs_help(node->getB());
    updateUBOs_help(node->getC());
    updateUBOs_help(node->getD());
}

void updateUBOs_visitor(QuadNode<Chunk*>* node, const TerrainGrid& terrGrid)
{
    if (node->isLeaf())
        node->getElement()->updateUBOs(terrGrid.camPos, terrGrid.view, terrGrid.proj);
}

std::tuple<float, float, float> TerrainGrid::closestCenter()
{
    float maxUsedCellSize = rootCellSize;
    for(size_t level = 0; level < minLevel; level++) maxUsedCellSize /= 2;

    return std::tuple<float, float, float>( 
        maxUsedCellSize * std::round(camPos.x / maxUsedCellSize),
        maxUsedCellSize* std::round(camPos.y / maxUsedCellSize), 
        0 );
}

QuadNode<Chunk*>* TerrainGrid::getNode(std::tuple<float, float, float> center, float sideLength)
{
    if (chunks.find(center) == chunks.end())
        chunks[center] = new Chunk(center, sideLength, numSideVertex, numSideVertex);
    
    return new QuadNode<Chunk*>(chunks[center]);
}