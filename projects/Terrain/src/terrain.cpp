
#include "terrain.hpp"
#include "ubo.hpp"


// Chunk ----------------------------------------------------------------------

Chunk::Chunk(Renderer& renderer, Noiser& noiseGen, glm::vec3 center, float stride, unsigned numHorVertex, unsigned numVertVertex, std::vector<Light*> lights, unsigned layer)
    : renderer(renderer), noiseGen(noiseGen), stride(stride), numHorVertex(numHorVertex), numVertVertex(numVertVertex), lights(lights), layer(layer), modelOrdered(false)
{
    baseCenter   = center;
    groundCenter = baseCenter;
}

Chunk::~Chunk()
{
    if (renderer.getModelsCount() && modelOrdered)
        renderer.deleteModel(model);
}

void Chunk::render(const char* vertexShader, const char* fragmentShader, std::vector<texIterator>& usedTextures, std::vector<uint16_t>* indices)
{
    VertexLoader* vertexLoader = new VertexFromUser(
        VertexType(1, 0, 1, 1),
        numHorVertex * numVertVertex,
        vertex.data(),
        indices ? *indices : this->indices,
        true);

    model = renderer.newModel(
        1, 1, primitiveTopology::triangle,
        vertexLoader,
        UBOconfig(1, MMsize, VMsize, PMsize, MMNsize, 3 * vec4size),     // MM (mat4), VM (mat4), PM (mat4), MMN (mat3), camPos (vec3) + lightPos (vec3) + ligthDir (vec3)
        UBOconfig(1, sizeof(Light)),                                     // Light
        usedTextures,
        vertexShader,
        fragmentShader,
        false);
    
    uint8_t* dest = model->vsDynUBO.getUBOptr(0);
    int bytes = 0;
    memcpy(dest + bytes, &modelMatrix(), mat4size);
    bytes += mat4size;
    //memcpy(dest + bytes, &view, mat4size);
    bytes += mat4size;
    //memcpy(dest + bytes, &proj, mat4size);
    bytes += mat4size;
    memcpy(dest + bytes, &modelMatrixForNormals(modelMatrix()), mat4size);
    //bytes += mat4size
    //memcpy(dest + bytes, &camPos, vec3size);
    //bytes += vec4size;
    //memcpy(dest + bytes, &sunLight.position, vec3size);
    //bytes += vec4size;
    //memcpy(dest + bytes, &sunLight.direction, vec3size);
    //bytes += vec4size;
 
    dest = model->fsUBO.getUBOptr(0);
    memcpy(dest, lights[0], sizeof(Light));
 
    modelOrdered = true;
}

void Chunk::updateUBOs(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj)
{
    if (!modelOrdered) return;

    int bytes = 0;
    for (size_t i = 0; i < model->vsDynUBO.numDynUBOs; i++)
    {
        uint8_t* dest = model->vsDynUBO.getUBOptr(0);
        bytes += mat4size;
        memcpy(dest + bytes, &view, mat4size);
        bytes += mat4size;
        memcpy(dest + bytes, &proj, mat4size);
        bytes += mat4size + mat4size;
        memcpy(dest + bytes, &camPos, vec3size);
        bytes += vec4size;
        memcpy(dest + bytes, &sunLight.position, vec3size);
        bytes += vec4size;
        memcpy(dest + bytes, &sunLight.direction, vec3size);
        bytes += vec4size;
    }
}

void Chunk::computeIndices(std::vector<uint16_t>& indices, unsigned numHorVertex, unsigned numVertVertex)
{
    indices.reserve((numHorVertex - 1) * (numVertVertex - 1) * 2 * 3);

    for (size_t v = 0; v < numVertVertex - 1; v++)
        for (size_t h = 0; h < numHorVertex - 1; h++)
        {
            unsigned int pos = v * numHorVertex + h;

            indices.push_back(pos);
            indices.push_back(pos + numHorVertex + 1);
            indices.push_back(pos + numHorVertex);

            indices.push_back(pos);
            indices.push_back(pos + 1);
            indices.push_back(pos + numHorVertex + 1);
        }
}

// PlainChunk ----------------------------------------------------------------------

PlainChunk::PlainChunk(Renderer& renderer, Noiser& noiseGen, std::vector<Light*> lights, glm::vec3 center, float stride, unsigned numHorVertex, unsigned numVertVertex, unsigned layer)
    : Chunk(renderer, noiseGen, center, stride, numHorVertex, numVertVertex, lights, layer)
{
    groundCenter.z = noiseGen.GetNoise(baseCenter.x, baseCenter.y);

    computeSizes();
}

void PlainChunk::computeTerrain(bool computeIndices, float textureFactor)
{
    size_t index;
    float x0 = baseCenter.x - horChunkSize / 2;
    float y0 = baseCenter.y - vertChunkSize / 2;

    // Vertex data
    vertex.reserve(numHorVertex * numVertVertex * 8);

    for (size_t y = 0; y < numVertVertex; y++)
        for (size_t x = 0; x < numHorVertex; x++)
        {
            index = y * numHorVertex + x;

            // positions (0, 1, 2)
            vertex[index * 8 + 0] = x0 + x * stride;
            vertex[index * 8 + 1] = y0 + y * stride;
            vertex[index * 8 + 2] = noiseGen.GetNoise((float)vertex[index * 8 + 0], (float)vertex[index * 8 + 1]);
            //std::cout << vertex[pos * 8 + 0] << ", " << vertex[pos * 8 + 1] << ", " << vertex[pos * 8 + 2] << std::endl;

            // textures (3, 4)
            vertex[index * 8 + 3] = vertex[index * 8 + 0] * textureFactor;
            vertex[index * 8 + 4] = vertex[index * 8 + 1] * textureFactor;     // LOOK produces textures reflected in the x-axis
            //std::cout << vertex[pos * 8 + 3] << ", " << vertex[pos * 8 + 4] << std::endl;
        }

    // Normals (5, 6, 7)
    computeGridNormals();

    // Indices
    if (computeIndices)
        this->computeIndices(indices, numHorVertex, numVertVertex);
}

void PlainChunk::computeGridNormals()
{
    // Initialize normals to 0
    unsigned numVertex = numHorVertex * numVertVertex;
    std::vector<glm::vec3> tempNormals(numVertex, glm::vec3(0.f, 0.f, 0.f));
    for (size_t i = 0; i < numVertex; i++) tempNormals[i] = glm::vec3(0.f, 0.f, 0.f);

    // Compute normals
    for (size_t y = 0; y < numVertVertex - 1; y++)
        for (size_t x = 0; x < numHorVertex - 1; x++)
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
    {
        size_t pos;
        glm::vec3 up, down, left, right, center;

        // Left & Right (no corners)
        for (size_t y = 1; y < numVertVertex - 1; y++)
        {
            // Left side:
            //     -Vertex vectors
            pos = getPos(0, y);
            center = getVertex(pos);
            up = getVertex(getPos(0, y + 1));
            down = getVertex(getPos(0, y - 1));
            left = glm::vec3(center.x - stride, center.y, noiseGen.GetNoise(center.x - stride, center.y));

            //     -Vector representing each side
            up = up - center;
            left = left - center;
            down = down - center;

            //     -Add normals to the existing normal of the vertex
            tempNormals[pos] += glm::cross(up, left) + glm::cross(left, down);

            // Right side:
            //     -Vertex vectors
            pos = getPos(numHorVertex - 1, y);
            center = getVertex(pos);
            up = getVertex(getPos(numHorVertex - 1, y + 1));
            down = getVertex(getPos(numHorVertex - 1, y - 1));
            right = glm::vec3(center.x + stride, center.y, noiseGen.GetNoise(center.x + stride, center.y));

            //     -Vector representing each side
            up = up - center;
            right = right - center;
            down = down - center;

            //     -Add normals to the existing normal of the vertex
            tempNormals[pos] += glm::cross(right, up) + glm::cross(down, right);
        }

        // Upper & Bottom (no corners)
        for (size_t x = 1; x < numHorVertex - 1; x++)
        {
            // Bottom side:
            //     -Vertex vectors
            pos = getPos(x, 0);
            center = getVertex(pos);
            right = getVertex(getPos(x + 1, 0));
            left = getVertex(getPos(x - 1, 0));
            down = glm::vec3(center.x, center.y - stride, noiseGen.GetNoise(center.x, center.y - stride));

            //     -Vector representing each side
            right = right - center;
            left = left - center;
            down = down - center;

            //     -Add normals to the existing normal of the vertex
            tempNormals[pos] += glm::cross(left, down) + glm::cross(down, right);

            // Upper side:
            //     -Vertex vectors
            pos = getPos(x, numVertVertex - 1);
            center = getVertex(pos);
            right = getVertex(getPos(x + 1, numVertVertex - 1));
            left = getVertex(getPos(x - 1, numVertVertex - 1));
            up = glm::vec3(center.x, center.y + stride, noiseGen.GetNoise(center.x, center.y + stride));

            //     -Vector representing each side
            right = right - center;
            left = left - center;
            up = up - center;

            //     -Add normals to the existing normal of the vertex
            tempNormals[pos] += glm::cross(up, left) + glm::cross(right, up);
        }

        // Corners
        {
            //  - Top left
            pos = getPos(0, numVertVertex - 1);
            center = getVertex(pos);
            right = getVertex(getPos(1, numVertVertex - 1));
            down = getVertex(getPos(0, numVertVertex - 2));
            up = glm::vec3(center.x, center.y + stride, noiseGen.GetNoise(center.x, center.y + stride));
            left = glm::vec3(center.x - stride, center.y, noiseGen.GetNoise(center.x - stride, center.y));

            right = right - center;
            left = left - center;
            up = up - center;
            down = down - center;

            tempNormals[pos] += glm::cross(right, up) + glm::cross(up, left) + glm::cross(left, down);

            //  - Top right
            pos = getPos(numHorVertex - 1, numVertVertex - 1);
            center = getVertex(pos);
            down = getVertex(getPos(numHorVertex - 1, numVertVertex - 2));
            left = getVertex(getPos(numHorVertex - 2, numVertVertex - 1));
            right = glm::vec3(center.x + stride, center.y, noiseGen.GetNoise(center.x + stride, center.y));
            up = glm::vec3(center.x, center.y + stride, noiseGen.GetNoise(center.x, center.y + stride));

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
            down = glm::vec3(center.x, center.y - stride, noiseGen.GetNoise(center.x, center.y - stride));
            left = glm::vec3(center.x - stride, center.y, noiseGen.GetNoise(center.x - stride, center.y));

            right = right - center;
            left = left - center;
            up = up - center;
            down = down - center;

            tempNormals[pos] += glm::cross(up, left) + glm::cross(left, down) + glm::cross(down, right);

            //  - Low right
            pos = getPos(numHorVertex - 1, 0);
            center = getVertex(pos);
            up = getVertex(getPos(numHorVertex - 1, 1));
            left = getVertex(getPos(numHorVertex - 2, 0));
            right = glm::vec3(center.x + stride, center.y, noiseGen.GetNoise(center.x + stride, center.y));
            down = glm::vec3(center.x, center.y - stride, noiseGen.GetNoise(center.x, center.y - stride));

            right = right - center;
            left = left - center;
            up = up - center;
            down = down - center;

            tempNormals[pos] += glm::cross(left, down) + glm::cross(down, right) + glm::cross(right, up);
        }
    }

    // Normalize the normals (normalized in shader)
    for (int i = 0; i < numVertex; i++)
    {
        tempNormals[i] = glm::normalize(tempNormals[i]);

        vertex[i * 8 + 5] = tempNormals[i].x;
        vertex[i * 8 + 6] = tempNormals[i].y;
        vertex[i * 8 + 7] = tempNormals[i].z;
    }
}

void PlainChunk::getSubBaseCenters(std::tuple<float, float, float>* centers)
{
    float quarterSide = horChunkSize / 4;

    centers[0] = std::tuple(baseCenter.x - quarterSide, baseCenter.y + quarterSide, baseCenter.z);
    centers[1] = std::tuple(baseCenter.x + quarterSide, baseCenter.y + quarterSide, baseCenter.z);
    centers[2] = std::tuple(baseCenter.x - quarterSide, baseCenter.y - quarterSide, baseCenter.z);
    centers[3] = std::tuple(baseCenter.x + quarterSide, baseCenter.y - quarterSide, baseCenter.z);
}

void PlainChunk::computeSizes()
{
    horBaseSize  = stride * (numHorVertex - 1);
    vertBaseSize = stride * (numVertVertex - 1);

    horChunkSize  = horBaseSize;
    vertChunkSize = vertBaseSize;
}

// SphericalChunk ----------------------------------------------------------------------

SphericalChunk::SphericalChunk(Renderer& renderer, Noiser& noiseGen, glm::vec3 cubeSideCenter, float stride, unsigned numHorVertex, unsigned numVertVertex, std::vector<Light*> lights, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, unsigned layer)
    : Chunk(renderer, noiseGen, cubeSideCenter, stride, numHorVertex, numVertVertex, lights, layer), nucleus(nucleus), radius(radius)
{   
    glm::vec3 unitVec = glm::normalize(baseCenter - nucleus);
    glm::vec3 sphere = unitVec * radius;
    groundCenter = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

    //if ((cubePlane.x || cubePlane.y || cubePlane.z) && (!cubePlane.x || !cubePlane.y) && (!cubePlane.x || !cubePlane.z) && (!cubePlane.y || !cubePlane.z))
    if (!(cubePlane.x * cubePlane.y) && !(cubePlane.y * cubePlane.z) && !(cubePlane.x * cubePlane.z) && (cubePlane.x || cubePlane.y || cubePlane.z))
    {
        if (cubePlane.x == 1.f)
        {
            xAxis = glm::vec3(0, 1, 0);
            yAxis = glm::vec3(0, 0, 1);
        }
        else if (cubePlane.x == -1.f)
        {
            xAxis = glm::vec3(0, -1, 0);
            yAxis = glm::vec3(0, 0, 1);
        }
        else if (cubePlane.y == 1.f)
        {
            xAxis = glm::vec3(-1, 0, 0);
            yAxis = glm::vec3(0, 0, 1);
        }
        else if (cubePlane.y == -1.f)
        {
            xAxis = glm::vec3(1, 0, 0);
            yAxis = glm::vec3(0, 0, 1);
        }
        else if (cubePlane.z == 1.f)
        {
            xAxis = glm::vec3(1, 0, 0);
            yAxis = glm::vec3(0, 1, 0);
        }
        else if (cubePlane.z == -1.f)
        {
            xAxis = glm::vec3(-1, 0, 0);
            yAxis = glm::vec3(0, 1, 0);
        }
    }
    else std::cout << "cubePlane parameter has wrong format" << std::endl;   // cubePlane must contain 2 zeros

    computeSizes();
}

void SphericalChunk::computeTerrain(bool computeIndices, float textureFactor)
{
    // Vertex data
    glm::vec3 pos0 = baseCenter - (xAxis * horBaseSize / 2.f) - (yAxis * vertBaseSize / 2.f);   // Position of the initial coordinate in the cube side plane (lower left).
    glm::vec3 unitVec, cube, sphere, ground;
    float index;
    vertex.reserve(numHorVertex * numVertVertex * 8);

    for (float v = 0; v < numVertVertex; v++)
        for (float h = 0; h < numHorVertex; h++)
        {
            index = v * numHorVertex + h;

            // positions (0, 1, 2)
            cube = pos0 + (xAxis * h * stride) + (yAxis * v * stride);
            unitVec = glm::normalize(cube - nucleus);
            sphere = unitVec * radius;
            ground = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);
            vertex[index * 8 + 0] = ground.x;
            vertex[index * 8 + 1] = ground.y;
            vertex[index * 8 + 2] = ground.z;

            // textures (3, 4)
            vertex[index * 8 + 3] = h * textureFactor;
            vertex[index * 8 + 4] = v * textureFactor;     // LOOK produces textures reflected in the x-axis
        }

    // Normals (5, 6, 7)
    computeGridNormals(pos0, xAxis, yAxis);

    // Indices
    if (computeIndices)
        this->computeIndices(indices, numHorVertex, numVertVertex);
}

void SphericalChunk::computeGridNormals(glm::vec3 pos0, glm::vec3 xAxis, glm::vec3 yAxis)
{
    // Initialize normals to 0
    unsigned numVertex = numHorVertex * numVertVertex;
    std::vector<glm::vec3> tempNormals(numVertex, glm::vec3(0));
    for (size_t i = 0; i < numVertex; i++) tempNormals[i] = glm::vec3(0);

    // Compute normals
    for (size_t y = 0; y < numVertVertex - 1; y++)
        for (size_t x = 0; x < numHorVertex - 1; x++)
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
    size_t pos;
    glm::vec3 center, up, down, left, right;
    glm::vec3 unitVec, cube, sphere;

    //  Left border
    for (float v = 1; v < numVertVertex - 1; v++)
    {
        // Vertex vectors
        pos = getPos(0, v);
        center = getVertex(pos);
        up = getVertex(getPos(0, v + 1));
        down = getVertex(getPos(0, v - 1));

        cube = pos0 + (yAxis * v * stride) - (xAxis * stride);
        unitVec = glm::normalize(cube - nucleus);
        sphere = unitVec * radius;
        left = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

        // Vector representing each side
        up = up - center;
        left = left - center;
        down = down - center;

        // Add normals to the existing normal of the vertex
        tempNormals[pos] += glm::cross(up, left) + glm::cross(left, down);
    }

    //  Right border
    for (float v = 1; v < numVertVertex - 1; v++)
    {
        // Vertex vectors
        pos = getPos(numHorVertex - 1, v);
        center = getVertex(pos);
        up = getVertex(getPos(numHorVertex - 1, v + 1));
        down = getVertex(getPos(numHorVertex - 1, v - 1));

        cube = pos0 + (xAxis * ((float)numHorVertex - 1.f) * stride) + (yAxis * v * stride) + (xAxis * stride);
        unitVec = glm::normalize(cube - nucleus);
        sphere = unitVec * radius;
        right = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

        // Vector representing each side
        up = up - center;
        right = right - center;
        down = down - center;

        // Add normals to the existing normal of the vertex
        tempNormals[pos] += glm::cross(right, up) + glm::cross(down, right);
    }

    //  Top border
    for (float h = 1; h < numHorVertex - 1; h++)
    {
        // Vertex vectors
        pos = getPos(h, numVertVertex - 1);
        center = getVertex(pos);
        right = getVertex(getPos(h + 1, numVertVertex - 1));
        left = getVertex(getPos(h - 1, numVertVertex - 1));

        cube = pos0 + (yAxis * ((float)numVertVertex - 1.f) * stride) + (xAxis * h * stride) + (yAxis * stride);
        unitVec = glm::normalize(cube - nucleus);
        sphere = unitVec * radius;
        up = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

        // Vector representing each side
        right = right - center;
        left = left - center;
        up = up - center;

        // Add normals to the existing normal of the vertex
        tempNormals[pos] += glm::cross(up, left) + glm::cross(right, up);
    }

    //  Bottom border
    for (float h = 1; h < numHorVertex - 1; h++)
    {
        // Vertex vectors
        pos = getPos(h, 0);
        center = getVertex(pos);
        right = getVertex(getPos(h + 1, 0));
        left = getVertex(getPos(h - 1, 0));

        cube = pos0 + (xAxis * h * stride) - (yAxis * stride);
        unitVec = glm::normalize(cube - nucleus);
        sphere = unitVec * radius;
        down = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

        // Vector representing each side
        right = right - center;
        left = left - center;
        down = down - center;

        // Add normals to the existing normal of the vertex
        tempNormals[pos] += glm::cross(left, down) + glm::cross(down, right);
    }

    //  Corners
    {
        //  - Top left
        pos = getPos(0, numVertVertex - 1);
        center = getVertex(pos);
        right = getVertex(getPos(1, numVertVertex - 1));
        down = getVertex(getPos(0, numVertVertex - 2));

        cube = pos0 + (yAxis * ((float)numVertVertex - 1.f) * stride) + (yAxis * stride);
        unitVec = glm::normalize(cube - nucleus);
        sphere = unitVec * radius;
        up = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

        cube = pos0 + (yAxis * ((float)numVertVertex - 1.f) * stride) - (xAxis * stride);
        unitVec = glm::normalize(cube - nucleus);
        sphere = unitVec * radius;
        left = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

        right = right - center;
        left = left - center;
        up = up - center;
        down = down - center;

        tempNormals[pos] += glm::cross(right, up) + glm::cross(up, left) + glm::cross(left, down);

        //  - Top right
        pos = getPos(numHorVertex - 1, numVertVertex - 1);
        center = getVertex(pos);
        down = getVertex(getPos(numHorVertex - 1, numVertVertex - 2));
        left = getVertex(getPos(numHorVertex - 2, numVertVertex - 1));

        cube = pos0 + (xAxis * ((float)numHorVertex - 1.f) * stride) + (yAxis * ((float)numVertVertex - 1.f) * stride) + (yAxis * stride);
        unitVec = glm::normalize(cube - nucleus);
        sphere = unitVec * radius;
        up = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

        cube = pos0 + (xAxis * ((float)numHorVertex - 1.f) * stride) + (yAxis * ((float)numVertVertex - 1.f) * stride) + (xAxis * stride);
        unitVec = glm::normalize(cube - nucleus);
        sphere = unitVec * radius;
        right = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

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

        cube = pos0 - (yAxis * stride);
        unitVec = glm::normalize(cube - nucleus);
        sphere = unitVec * radius;
        down = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

        cube = pos0 - (xAxis * stride);
        unitVec = glm::normalize(cube - nucleus);
        sphere = unitVec * radius;
        left = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

        right = right - center;
        left = left - center;
        up = up - center;
        down = down - center;

        tempNormals[pos] += glm::cross(up, left) + glm::cross(left, down) + glm::cross(down, right);

        //  - Low right
        pos = getPos(numHorVertex - 1, 0);
        center = getVertex(pos);
        up = getVertex(getPos(numHorVertex - 1, 1));
        left = getVertex(getPos(numHorVertex - 2, 0));

        cube = pos0 + (xAxis * ((float)numHorVertex - 1.f) * stride) - (yAxis * stride);
        unitVec = glm::normalize(cube - nucleus);
        sphere = unitVec * radius;
        down = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

        cube = pos0 + (xAxis * ((float)numHorVertex - 1.f) * stride) + (xAxis * stride);
        unitVec = glm::normalize(cube - nucleus);
        sphere = unitVec * radius;
        right = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

        right = right - center;
        left = left - center;
        up = up - center;
        down = down - center;

        tempNormals[pos] += glm::cross(left, down) + glm::cross(down, right) + glm::cross(right, up);
    }

    // Normalize the normals
    for (int i = 0; i < numVertex; i++)
    {
        tempNormals[i] = glm::normalize(tempNormals[i]);
        vertex[i * 8 + 5] = tempNormals[i].x;
        vertex[i * 8 + 6] = tempNormals[i].y;
        vertex[i * 8 + 7] = tempNormals[i].z;
    }
}

void SphericalChunk::getSubBaseCenters(std::tuple<float, float, float>* centers)
{
    float quarterSide = horBaseSize / 4;
    glm::vec3 vecs[4];

    vecs[0] = baseCenter - (quarterSide * xAxis) + (quarterSide * yAxis);
    vecs[1] = baseCenter + (quarterSide * xAxis) + (quarterSide * yAxis);
    vecs[2] = baseCenter - (quarterSide * xAxis) - (quarterSide * yAxis);
    vecs[3] = baseCenter + (quarterSide * xAxis) - (quarterSide * yAxis);

    centers[0] = std::tuple(vecs[0].x, vecs[0].y, vecs[0].z);
    centers[1] = std::tuple(vecs[1].x, vecs[1].y, vecs[1].z);
    centers[2] = std::tuple(vecs[2].x, vecs[2].y, vecs[2].z);
    centers[3] = std::tuple(vecs[3].x, vecs[3].y, vecs[3].z);
}

void SphericalChunk::computeSizes()
{
    horBaseSize = stride * (numHorVertex - 1);
    vertBaseSize = stride * (numVertVertex - 1);

    glm::vec3 left = radius * glm::normalize(baseCenter - (xAxis * horBaseSize / 2.f) - nucleus);
    glm::vec3 right = radius * glm::normalize(baseCenter + (xAxis * horBaseSize / 2.f) - nucleus);
    glm::vec3 bottom = radius * glm::normalize(baseCenter - (yAxis * vertBaseSize / 2.f) - nucleus);
    glm::vec3 top = radius * glm::normalize(baseCenter + (yAxis * vertBaseSize / 2.f) - nucleus);
    horChunkSize = glm::length(left - right);
    vertChunkSize = glm::length(bottom - top);

    //std::cout << horChunkSize << ", " << vertChunkSize << std::endl;
    //std::cout << baseCenter.x << ", " << baseCenter.y << ", " << baseCenter.z << ", " << std::endl;
}

// DynamicGrid ----------------------------------------------------------------------

DynamicGrid::DynamicGrid(glm::vec3 camPos, std::vector<Light*> lights, Renderer& renderer, Noiser noiseGenerator, unsigned activeTree, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier)
    : camPos(camPos), lights(lights), renderer(renderer), noiseGenerator(noiseGenerator), activeTree(activeTree), loadedChunks(0), rootCellSize(rootCellSize), numSideVertex(numSideVertex), numLevels(numLevels), minLevel(minLevel), distMultiplier(distMultiplier)
{ 
    root[0] = root[1] = nullptr;
    Chunk::computeIndices(indices, numSideVertex, numSideVertex);
};

DynamicGrid::~DynamicGrid()
{
    if (root[0]) delete root[0];
    if (root[1]) delete root[1];

    for (auto it = chunks.begin(); it != chunks.end(); it++)
        delete it->second;

    chunks.clear();
}

void DynamicGrid::addTextures(const std::vector<texIterator>& textures) { this->textures = textures; }

void DynamicGrid::addShaders(const char* vertShader, const char* fragShader)
{
    this->vertShader = std::string(vertShader);
    this->fragShader = std::string(fragShader);
}

void DynamicGrid::updateTree(glm::vec3 newCamPos)
{
    if (!numLevels) return;

    unsigned nonActiveTree = (activeTree + 1) % 2;
    if (root[activeTree] && !root[nonActiveTree] && camPos.x == newCamPos.x && camPos.y == newCamPos.y && camPos.z == newCamPos.z) 
        return;  // ERROR: When updateTree doesn't run in each frame (i.e., when command buffer isn't created each frame), no validation error appears after resizing window

    // Build tree if the non-active tree is nullptr
    if (!root[nonActiveTree])
    {
        camPos = newCamPos;

        std::tuple<float, float, float> center = closestCenter();
        root[nonActiveTree] = getNode(center, rootCellSize, 0);     // Create root node and chunk
        
        renderedChunks = 0;
        createTree(root[nonActiveTree], 0);                         // Build tree and load leaf-chunks
    }

    // Check whether non-active tree has fully constructed leaf-chunks. If so, switch trees
    if (fullConstChunks(root[nonActiveTree]))
    {
        changeRenders(root[activeTree], false);
        if (root[activeTree]) delete root[activeTree];
        root[activeTree] = nullptr;

        changeRenders(root[nonActiveTree], true);
        activeTree = nonActiveTree;

        removeFarChunks(4, newCamPos);
    }
}

void DynamicGrid::createTree(QuadNode<Chunk*>* node, size_t depth)
{
    Chunk* chunk = node->getElement();
    glm::vec3 gCenter = chunk->getGroundCenter();
    float chunkLength = chunk->getHorChunkSide();
    float sqrSide = chunkLength * chunkLength;
    float sqrDist = (camPos.x - gCenter.x) * (camPos.x - gCenter.x) + (camPos.y - gCenter.y) * (camPos.y - gCenter.y) + (camPos.z - gCenter.z) * (camPos.z - gCenter.z);
    
    // Is leaf node > Compute terrain > Children are nullptr by default
    if (depth >= minLevel && (sqrDist > sqrSide * distMultiplier || depth == numLevels - 1))
    {
        //std::cout << ' ' << chunk->getLayer();
        //std::cout << ' ' << std::sqrt(sqrDist) / std::sqrt(sqrSide);
        renderedChunks++;
        if (chunk->modelOrdered == false)
        {
            chunk->computeTerrain(false);       //, std::pow(2, numLevels - 1 - depth));
            chunk->render(vertShader.c_str(), fragShader.c_str(), textures, &indices);
            //chunk->updateUBOs(camPos, view, proj);
            renderer.setRenders(chunk->model, 0);
            loadedChunks++;
        }
    }
    // Is not leaf node > Create children > Recursion
    else
    {
        depth++;
        std::tuple<float, float, float> subBaseCenters[4];
        chunk->getSubBaseCenters(subBaseCenters);
        float halfSide = chunk->getHorBaseSide() / 2;

        node->setA(getNode(subBaseCenters[0], halfSide, depth));
        createTree(node->getA(), depth);

        node->setB(getNode(subBaseCenters[1], halfSide, depth));
        createTree(node->getB(), depth);

        node->setC(getNode(subBaseCenters[2], halfSide, depth));
        createTree(node->getC(), depth);

        node->setD(getNode(subBaseCenters[3], halfSide, depth));
        createTree(node->getD(), depth);
    }
}

void DynamicGrid::updateUBOs(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj)
{
    this->camPos = camPos;
    this->view = view;
    this->proj = proj;

    //preorder<Chunk*, void (QuadNode<Chunk*>*)>(root, nodeVisitor);
    updateUBOs_help(root[activeTree]);  // Preorder traversal
}

void DynamicGrid::updateUBOs_help(QuadNode<Chunk*>* node)
{
    if (!node) return;

    if (node->isLeaf())
    {
        node->getElement()->updateUBOs(camPos, view, proj);
        return;
    }
    else
    {
        updateUBOs_help(node->getA());
        updateUBOs_help(node->getB());
        updateUBOs_help(node->getC());
        updateUBOs_help(node->getD());
    }
}

bool DynamicGrid::fullConstChunks(QuadNode<Chunk*>* node)
{
    if (!node) return false;

    if (node->isLeaf())
    {
        if (node->getElement()->model->fullyConstructed)
            return true;
        else
            return false;
    }
    else
    {
        char full[4];
        full[0] = fullConstChunks(node->getA());
        full[1] = fullConstChunks(node->getB());
        full[2] = fullConstChunks(node->getC());
        full[3] = fullConstChunks(node->getD());

        return full[0] && full[1] && full[2] && full[3];
    }
}

void DynamicGrid::changeRenders(QuadNode<Chunk*>* node, bool renderMode)
{
    if (!node) return;

    if (node->isLeaf())
        renderer.setRenders(node->getElement()->model, (renderMode ? 1 : 0));
    else
    {
        changeRenders(node->getA(), renderMode);
        changeRenders(node->getB(), renderMode);
        changeRenders(node->getC(), renderMode);
        changeRenders(node->getD(), renderMode);
    }
}

void DynamicGrid::removeFarChunks(unsigned relDist, glm::vec3 camPosNow)
{
    glm::vec3 center;
    glm::vec3 distVec;
    float targetDist;
    float targetSqrDist;
    std::map<std::tuple<float, float, float>, Chunk*>::iterator it = chunks.begin();
    std::map<std::tuple<float, float, float>, Chunk*>::iterator nextIt;

    while (it != chunks.end())
    {
        nextIt = it;
        nextIt++;
        targetDist = it->second->getHorChunkSide() * relDist;

        if (it->second->modelOrdered && it->second->model->fullyConstructed && !it->second->model->activeRenders)
        {
            center = it->second->getGroundCenter();
            distVec.x = center.x - camPosNow.x;
            distVec.y = center.y - camPosNow.y;
            distVec.z = center.z - camPosNow.z;
            targetSqrDist = targetDist * targetDist;

            if ((distVec.x * distVec.x + distVec.y * distVec.y + distVec.z * distVec.z) > targetSqrDist)
            {
                delete chunks[it->first];
                chunks.erase(it->first);
                loadedChunks--;
            }
        }

        it = nextIt;
    }
}

//void updateUBOs_visitor(QuadNode<PlainChunk*>* node, const TerrainGrid& terrGrid)
//{
//    if (node->isLeaf())
//        node->getElement()->updateUBOs(terrGrid.camPos, terrGrid.view, terrGrid.proj);
//}


// TerrainGrid ----------------------------------------------------------------------

TerrainGrid::TerrainGrid(Renderer& renderer, Noiser noiseGenerator, std::vector<Light*> lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier)
    : DynamicGrid(glm::vec3(0.1f, 0.1f, 0.1f), lights, renderer, noiseGenerator, 0, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier)
{ }

QuadNode<Chunk*>* TerrainGrid::getNode(std::tuple<float, float, float> center, float sideLength, unsigned layer)
{
    if (chunks.find(center) == chunks.end())
        chunks[center] = new PlainChunk(renderer, noiseGenerator, lights, glm::vec3(std::get<0>(center), std::get<1>(center), std::get<2>(center)), sideLength/(numSideVertex-1), numSideVertex, numSideVertex, layer);

    return new QuadNode<Chunk*>(chunks[center]);
}

std::tuple<float, float, float> TerrainGrid::closestCenter()
{
    float maxUsedCellSize = rootCellSize;
    for (size_t level = 0; level < minLevel; level++) maxUsedCellSize /= 2;

    return std::tuple<float, float, float>(
        maxUsedCellSize * std::round(camPos.x / maxUsedCellSize),
        maxUsedCellSize * std::round(camPos.y / maxUsedCellSize),
        0);
}

// PlanetGrid ----------------------------------------------------------------------

PlanetGrid::PlanetGrid(Renderer& renderer, Noiser noiseGenerator, std::vector<Light*> lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, glm::vec3 cubeSideCenter)
    : DynamicGrid(glm::vec3(0.1f, 0.1f, 0.1f), lights, renderer, noiseGenerator, 0, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier), radius(radius), nucleus(nucleus), cubePlane(cubePlane), cubeSideCenter(cubeSideCenter) { }

QuadNode<Chunk*>* PlanetGrid::getNode(std::tuple<float, float, float> center, float sideLength, unsigned layer)
{
    if (chunks.find(center) == chunks.end())
        chunks[center] = new SphericalChunk(renderer, noiseGenerator, glm::vec3(std::get<0>(center), std::get<1>(center), std::get<2>(center)), sideLength / (numSideVertex - 1), numSideVertex, numSideVertex, lights, radius, nucleus, cubePlane, layer);

    return new QuadNode<Chunk*>(chunks[center]);
}

std::tuple<float, float, float> PlanetGrid::closestCenter() 
{ 
    return std::tuple<float, float, float>(cubeSideCenter.x, cubeSideCenter.y, cubeSideCenter.z);
}
