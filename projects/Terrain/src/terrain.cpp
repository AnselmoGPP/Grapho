
#include "terrain.hpp"


Chunk::Chunk(glm::vec3 center, float sideXSize, unsigned numVertexX, unsigned numVertexY)
    : center(center), sideXSize(sideXSize), numVertexX(numVertexX), numVertexY(numVertexY), modelLoaded(false) { }

Chunk::Chunk(std::tuple<float, float, float> center, float sideXSize, unsigned numVertexX, unsigned numVertexY)
    : sideXSize(sideXSize), numVertexX(numVertexX), numVertexY(numVertexY), modelLoaded(false)
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

void Chunk::computeTerrain(noiseSet& noise, bool computeIndices, float textureFactor)
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

    modelLoaded = true;
}

void Chunk::updateUBOs(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj)
{
    if (!modelLoaded) return;

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

void Chunk::computeGridNormals(float stride, noiseSet& noise)
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


TerrainGrid::TerrainGrid(noiseSet noiseGenerator, glm::vec3 camPos, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier)
    : root(nullptr), app(nullptr), noiseGenerator(noiseGenerator), camPos(camPos), nodeCount(0), leafCount(0), rootCellSize(rootCellSize), numSideVertex(numSideVertex), numLevels(numLevels), minLevel(minLevel), distMultiplier(distMultiplier) 
{ 

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
    camPos = newCamPos;
    if (root) delete root;
    if (!numLevels) return;

    std::tuple<float, float, float> center = closestCenter();
    
    if (chunks.find(center) == chunks.end());
        chunks[center] = new Chunk(center, rootCellSize, numSideVertex, numSideVertex);

    root = new QuadNode<Chunk*>(chunks[center]);
    chunks[center]->computeIndices(indices);

    updateTree_help(root, 0);
}

void TerrainGrid::updateTree_help(QuadNode<Chunk*> *node, size_t depth)
{
    nodeCount++;

    glm::vec3 center = node->getElement()->getCenter();
    float squareSide = node->getElement()->getSide() * node->getElement()->getSide();
    float squareDist = (camPos.x - center.x) * (camPos.x - center.x) + (camPos.y - center.y) * (camPos.y - center.y);

    if (depth >= minLevel && (squareDist > squareSide * distMultiplier || depth == numLevels - 1))  // Is leaf node > Compute terrain > Children are nullptr by default
    {
        leafCount++;

        if(!node->getElement()->vertex.size())
            node->getElement()->computeTerrain(noiseGenerator, false, std::pow(2, numLevels - 1 - depth));

        if (!node->getElement()->modelLoaded)
            node->getElement()->render(app, textures, &indices);
     }
    else // Is not leaf node > Create children > Recursion
    {
        depth++;
        
        std::tuple<float, float, float> newCenter;
        float side = node->getElement()->getSide();
        float halfSide = side / 2;
        float quarterSide = side / 4;

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

// BinaryKey --------------------------------------------

BinaryKey::BinaryKey(int first, int second) { x = first; y = second; }

void BinaryKey::set(int first, int second) { x = first; y = second; }

bool BinaryKey::operator <( const BinaryKey &rhs ) const
{
    if( x < rhs.x) return true;
    if( x > rhs.x) return false;
    if( y < rhs.y) return true;
    if( y > rhs.y) return false;
    return false;
}

bool BinaryKey::operator ==( const BinaryKey &rhs ) const
{
    if(x == rhs.x && y == rhs.y) return true;
    return false;
}

// terrainChunks --------------------------------------------

int terrainChunks::getNumVertex()   { return vertexPerSide * vertexPerSide; }
int terrainChunks::getNumIndices()  { return (vertexPerSide-1) * (vertexPerSide-1) * 2 * 3; }
int terrainChunks::getMaxViewDist() { return maxViewDist; }

terrainChunks::terrainChunks(noiseSet noise, float maxViewDist, float chunkSize, unsigned vertexPerSide)
{
    updateTerrainParameters(noise, maxViewDist, chunkSize, vertexPerSide);
}

terrainChunks::~terrainChunks() { }

void terrainChunks::updateVisibleChunks(glm::vec3 viewerPos)
{
    // Viewer coordinates in chunk coordinates (origin at chunk (0, 0))
    int viewerChunkCoord_X = std::round(viewerPos.x / chunkSize);
    int viewerChunkCoord_Y = std::round(viewerPos.y / chunkSize);
    int viewerChunkCoord_Z = std::round(viewerPos.z / chunkSize);

    // Delete chunks out of range    
    typedef std::map<BinaryKey, NoiseSurface> dictionary;

    std::vector<dictionary::const_iterator> toErase;

    for(dictionary::const_iterator it = chunkDict.begin(); it != chunkDict.end(); ++it)
    {
        BinaryKey key = it->first;

        // Circular area
        float sqrtDistInChunks = (key.x-viewerChunkCoord_X)*(key.x-viewerChunkCoord_X) + (key.y-viewerChunkCoord_Y)*(key.y-viewerChunkCoord_Y);
        float maxSqrtDistInChunks = (chunksVisible + 0.5) * (chunksVisible + 0.5);

        if(sqrtDistInChunks > maxSqrtDistInChunks)
            toErase.push_back(it);

        // Square area
        //if(key.x < viewerChunkCoord_X - chunksVisible || key.x > viewerChunkCoord_X + chunksVisible ||
        //   key.y < viewerChunkCoord_Y - chunksVisible || key.y > viewerChunkCoord_Y + chunksVisible )
        //    toErase.push_back(it);
    }

    for(size_t i = 0; i < toErase.size(); ++i)
        chunkDict.erase(toErase[i]);

    // Save chunks in range
    NoiseSurface generator;

    for(int yOffset = viewerChunkCoord_Y - chunksVisible; yOffset <= viewerChunkCoord_Y + chunksVisible; yOffset++)
        for(int xOffset = viewerChunkCoord_X - chunksVisible; xOffset <= viewerChunkCoord_X + chunksVisible; xOffset++)
        {
            float sqrtDistInChunks = (xOffset-viewerChunkCoord_X)*(xOffset-viewerChunkCoord_X) + (yOffset-viewerChunkCoord_Y)*(yOffset-viewerChunkCoord_Y);
            float maxSqrtDistInChunks = (chunksVisible + 0.5) * (chunksVisible + 0.5);
            if(sqrtDistInChunks > maxSqrtDistInChunks)
                continue;                                       // if out of range, skip iteration

            BinaryKey chunkCoord(xOffset, yOffset);             // chunk name (key)

            if(chunkDict.find(chunkCoord) == chunkDict.end())   // if(chunk doesn't exist in chunkDict) add new chunk
            {
                generator.computeTerrain( noise,
                                          xOffset * chunkSize,
                                          yOffset * chunkSize,
                                          chunkSize/(vertexPerSide-1),
                                          vertexPerSide,
                                          vertexPerSide  );

                //chunkDict.insert( {chunkCoord, generator} );  // Doesn't require default constructor. If element already exists, insert does nothing
                chunkDict[chunkCoord] = generator;              // Requires default constructor
            }
        }

}

void terrainChunks::updateTerrainParameters(noiseSet noise, float maxViewDist, float chunkSize, unsigned vertexPerSide)
{
    chunkDict.clear();

    this->noise         = noise;
    this->maxViewDist   = maxViewDist;
    this->chunkSize     = chunkSize;
    this->chunksVisible = std::round(maxViewDist/chunkSize);
    this->vertexPerSide = vertexPerSide;
}

void terrainChunks::setNoise(noiseSet newNoise)
{
    this->noise = newNoise;
}
