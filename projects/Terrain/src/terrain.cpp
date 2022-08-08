
#include "terrain.hpp"


// Chunk ----------------------------------------------------------------------

Chunk::Chunk(Renderer& renderer, Noiser& noiseGen, std::tuple<float, float, float> center, float stride, unsigned numHorVertex, unsigned numVertVertex, unsigned layer)
    : renderer(renderer), noiseGen(noiseGen), stride(stride), numHorVertex(numHorVertex), numVertVertex(numVertVertex), layer(layer), modelOrdered(false)
{ 
    horSize  = stride * (numHorVertex - 1);
    vertSize = stride * (numVertVertex - 1);

    baseCenter.x = std::get<0>(center);
    baseCenter.y = std::get<1>(center);
    baseCenter.z = std::get<2>(center);

    groundCenter.x = baseCenter.x;
    groundCenter.y = baseCenter.y;
    groundCenter.z = baseCenter.z;
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
        UBOconfig(1, MMsize, VMsize, PMsize, MMNsize),
        UBOconfig(1, lightSize, vec4size),
        usedTextures,
        vertexShader,
        fragmentShader,
        false);

    model->vsDynUBO.setUniform(0, 0, modelMatrix());
    //model->vsDynUBO.setUniform(i, 1, view);
    //model->vsDynUBO.setUniform(i, 2, proj);
    model->vsDynUBO.setUniform(0, 3, modelMatrixForNormals(modelMatrix()));

    //sun.turnOff();
    sunLight.setDirectional(-sunLightDirection(dayTime), glm::vec3(0.1, 0.1, 0.1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1));
    //sun.setPoint(glm::vec3(0, 0, 50), glm::vec3(0.1, 0.1, 0.1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), 1, 0.1, 0.01);
    //sun.setSpot(glm::vec3(0, 0, 150), glm::vec3(0, 0, 1), glm::vec3(0.1, 0.1, 0.1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), 1, 0, 0., 0.9, 0.8);
    model->fsUBO.setUniform(0, 0, sunLight);
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

PlainChunk::PlainChunk(Renderer& renderer, Noiser& noiseGen, std::tuple<float, float, float> center, float stride, unsigned numHorVertex, unsigned numVertVertex, unsigned layer)
    : Chunk(renderer, noiseGen, center, stride, numHorVertex, numVertVertex, layer) 
{ 
    groundCenter.z = noiseGen.GetNoise(baseCenter.x, baseCenter.y);
}

void PlainChunk::computeTerrain(bool computeIndices, float textureFactor)
{
    size_t index;
    float x0 = baseCenter.x - horSize / 2;
    float y0 = baseCenter.y - vertSize / 2;

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
            vertex[index * 8 + 3] = x * textureFactor;
            vertex[index * 8 + 4] = y * textureFactor;     // LOOK produces textures reflected in the x-axis
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

// SphericalChunk ----------------------------------------------------------------------

SphericalChunk::SphericalChunk(Renderer& renderer, Noiser& noiseGen, std::tuple<float, float, float> cubeSideCenter, float stride, unsigned numHorVertex, unsigned numVertVertex, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, unsigned layer)
    : Chunk(renderer, noiseGen, cubeSideCenter, stride, numHorVertex, numVertVertex, layer), cubePlane(cubePlane), nucleus(nucleus), radius(radius)
{   
    glm::vec3 unitVec = glm::normalize(baseCenter - nucleus);
    glm::vec3 sphere = unitVec * radius;
    groundCenter = sphere + unitVec * noiseGen.GetNoise(sphere.x, sphere.y, sphere.z);

    if ((cubePlane.x || cubePlane.y || cubePlane.z) && (!cubePlane.x || !cubePlane.y) && (!cubePlane.x || !cubePlane.z) && (!cubePlane.y || !cubePlane.z));
    else std::cout << "cubePlane parameter has wrong format" << std::endl;   // cubePlane must contain 2 zeros
}

void SphericalChunk::computeTerrain(bool computeIndices, float textureFactor)
{
    glm::vec3 pos0;             // Position of the initial coordinate in the cube side plane (lower left).
    glm::vec3 xAxis, yAxis;		// Vectors representing the relative XY coord. system of the cube side plane.

    if (cubePlane.x == 1.f)
    {
        pos0 = { baseCenter.x, baseCenter.y - horSize / 2, baseCenter.z - vertSize / 2 };
        xAxis = glm::vec3(0, 1, 0);
        yAxis = glm::vec3(0, 0, 1);
    }
    else if (cubePlane.x == -1.f)
    {
        pos0 = { baseCenter.x, baseCenter.y + horSize / 2, baseCenter.z - vertSize / 2 };
        xAxis = glm::vec3(0, -1, 0);
        yAxis = glm::vec3(0, 0, 1);
    }
    else if (cubePlane.y == 1.f)
    {
        pos0 = { baseCenter.x + horSize / 2, baseCenter.y, baseCenter.z - vertSize / 2 };
        xAxis = glm::vec3(-1, 0, 0);
        yAxis = glm::vec3(0, 0, 1);
    }
    else if (cubePlane.y == -1.f)
    {
        pos0 = { baseCenter.x - horSize / 2, baseCenter.y, baseCenter.z - vertSize / 2 };
        xAxis = glm::vec3(1, 0, 0);
        yAxis = glm::vec3(0, 0, 1);
    }
    else if (cubePlane.z == 1.f)
    {
        pos0 = { baseCenter.x - horSize / 2, baseCenter.y - vertSize / 2, baseCenter.z };
        xAxis = glm::vec3(1, 0, 0);
        yAxis = glm::vec3(0, 1, 0);
    }
    else if (cubePlane.z == -1.f)
    {
        pos0 = { baseCenter.x + horSize / 2, baseCenter.y - vertSize / 2, baseCenter.z };
        xAxis = glm::vec3(-1, 0, 0);
        yAxis = glm::vec3(0, 1, 0);
    }

    // Vertex data
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

// TerrainGrid ----------------------------------------------------------------------

TerrainGrid::TerrainGrid(Renderer& renderer, Noiser noiseGenerator, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier)
    : activeTree(0), 
    renderer(renderer), 
    noiseGenerator(noiseGenerator), 
    camPos(glm::vec3(0.1f,0.1f,0.1f)), 
    nodeCount(0), 
    leafCount(0), 
    rootCellSize(rootCellSize), 
    numSideVertex(numSideVertex), 
    numLevels(numLevels), 
    minLevel(minLevel), 
    distMultiplier(distMultiplier)
{ 
    root[0] = root[1] = nullptr;
    Chunk::computeIndices(indices, numSideVertex, numSideVertex);
}

TerrainGrid::~TerrainGrid() 
{
    if (root[0]) delete root[0];
    if (root[1]) delete root[1];

    for (auto it = chunks.begin(); it != chunks.end(); it++)
        delete it->second;

    chunks.clear();
}

//void TerrainGrid::addApp(Renderer& app) { this->app = &app; }

void TerrainGrid::addTextures(const std::vector<texIterator>& textures) { this->textures = textures; }

void TerrainGrid::updateTree(glm::vec3 newCamPos)
{
    if (!numLevels) return;

    unsigned nonActiveTree = (activeTree + 1) % 2;
    if (root[activeTree] && !root[nonActiveTree] && camPos.x == newCamPos.x && camPos.y == newCamPos.y && camPos.z == newCamPos.z) return;  // ERROR: When updateTree doesn't run in each frame (i.e., when command buffer isn't created each frame), no validation error appears after resizing window

    // Build tree if the non-active tree is nullptr
    if (!root[nonActiveTree])
    {
        camPos = newCamPos;
        nodeCount = 0;
        leafCount = 0;

        std::tuple<float, float, float> center = closestCenter();
        root[nonActiveTree] = getNode(center, rootCellSize, 0);     // Create root node and chunk

        createTree(root[nonActiveTree], 0);                         // Build tree and load leaf-chunks
    }

    // Check whether non-active tree has fully constructed leaf-chunks. If so, switch trees
    if (fullConstChunks(root[nonActiveTree]))
    {
        changeRenders(root[activeTree], false);
        if(root[activeTree]) delete root[activeTree];
        root[activeTree] = nullptr;

        changeRenders(root[nonActiveTree], true);
        activeTree = nonActiveTree;

        removeFarChunks(4, newCamPos);
    }
}

void TerrainGrid::createTree(QuadNode<PlainChunk*> *node, size_t depth)
{
    /*
    Avoid chunks-swap gap. Cases:
        - Childs required but not loaded yet: Check if childs are leaf > If childs not loaded, load parent.
            Recursive call to childs must return "leafButNotLoaded" > Then, parent is rendered.
        - Parent required but not loaded yet: Check if this node is leaf > If not loaded, load childs.
            Check whether childs exist > If so, render them.
    */

    nodeCount++;
    PlainChunk* chunk = node->getElement();
    glm::vec3 center = chunk->getCenter();
    float sqrSide = chunk->getHorSide() * chunk->getHorSide();
    float sqrDist = (camPos.x - center.x) * (camPos.x - center.x) + (camPos.y - center.y) * (camPos.y - center.y) + (camPos.z - center.z) * (camPos.z - center.z);
    
    // Is leaf node > Compute terrain > Children are nullptr by default
    if (depth >= minLevel && (sqrDist > sqrSide * distMultiplier || depth == numLevels - 1))
    {
        leafCount++;

        if (chunk->modelOrdered == false)
        {
            chunk->computeTerrain(false, std::pow(2, numLevels - 1 - depth));
            chunk->render((SHADERS_DIR + "v_terrainPTN.spv").c_str(), (SHADERS_DIR + "f_terrainPTN.spv").c_str(), textures, &indices);
            //chunk->updateUBOs(camPos, view, proj);
            renderer.setRenders(chunk->model, 0);
        }

        //app->setRenders(chunk->model, 1);
        //chunk->visible = true;
     }
    // Is not leaf node > Create children > Recursion
    else
    {
        depth++;
        float halfSide = chunk->getHorSide() / 2;
        float quarterSide = chunk->getHorSide() / 4;

        node->setA(getNode(std::tuple(center.x - quarterSide, center.y + quarterSide, 0), halfSide, depth));
        createTree(node->getA(), depth);

        node->setB(getNode(std::tuple(center.x + quarterSide, center.y + quarterSide, 0), halfSide, depth));
        createTree(node->getB(), depth);

        node->setC(getNode(std::tuple(center.x - quarterSide, center.y - quarterSide, 0), halfSide, depth));
        createTree(node->getC(), depth);

        node->setD(getNode(std::tuple(center.x + quarterSide, center.y - quarterSide, 0), halfSide, depth));
        createTree(node->getD(), depth);
    }
}

void TerrainGrid::updateUBOs(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj)
{
    this->camPos = camPos;
    this->view = view;
    this->proj = proj;

    //preorder<Chunk*, void (QuadNode<Chunk*>*)>(root, nodeVisitor);
    updateUBOs_help(root[activeTree]);  // Preorder traversal
}

void TerrainGrid::updateUBOs_help(QuadNode<PlainChunk*>* node)
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

void updateUBOs_visitor(QuadNode<PlainChunk*>* node, const TerrainGrid& terrGrid)
{
    if (node->isLeaf())
        node->getElement()->updateUBOs(terrGrid.camPos, terrGrid.view, terrGrid.proj);
}

/*
    Check fullyConstructed
    Show tree once per second
*/
bool TerrainGrid::fullConstChunks(QuadNode<PlainChunk*>* node)
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

void TerrainGrid::changeRenders(QuadNode<PlainChunk*>* node, bool renderMode)
{
    if (!node) return;

    if(node->isLeaf())
        renderer.setRenders(node->getElement()->model, (renderMode ? 1 : 0));
    else
    {
        changeRenders(node->getA(), renderMode);
        changeRenders(node->getB(), renderMode);
        changeRenders(node->getC(), renderMode);
        changeRenders(node->getD(), renderMode);
    }
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

QuadNode<PlainChunk*>* TerrainGrid::getNode(std::tuple<float, float, float> center, float sideLength, unsigned layer)
{
    if (chunks.find(center) == chunks.end())
    {
        chunks[center] = new PlainChunk(renderer, noiseGenerator, center, sideLength/(numSideVertex-1), numSideVertex, numSideVertex, layer);

        //glm::vec3 exactCenter = chunks[center]->getCenter();
        //exactCenter.z = noiseGenerator.GetNoise(exactCenter.x, exactCenter.y);
        //chunks[center]->setCenter(exactCenter);
    }

    return new QuadNode<PlainChunk*>(chunks[center]);
}

void TerrainGrid::removeFarChunks(unsigned relDist, glm::vec3 camPosNow)
{
    glm::vec3 center;
    glm::vec3 distVec;
    float targetSqrDist;
    std::map<std::tuple<float, float, float>, PlainChunk*>::iterator it = chunks.begin();
    std::map<std::tuple<float, float, float>, PlainChunk*>::iterator nextIt;

    while (it != chunks.end())
    {
        nextIt = it;
        nextIt++;
        
        if (it->second->modelOrdered && it->second->model->fullyConstructed && !it->second->model->activeRenders)
        {
            center = it->second->getCenter();
            distVec.x = center.x - camPosNow.x;
            distVec.y = center.y - camPosNow.y;
            distVec.z = center.z - camPosNow.z;
            targetSqrDist = it->second->getHorSide() * relDist * it->second->getHorSide() * relDist;
            
            if ((distVec.x * distVec.x + distVec.y * distVec.y + distVec.z * distVec.z) > targetSqrDist)
            {
                delete chunks[it->first];
                chunks.erase(it->first);
            }
        }

        it = nextIt;
    }
}