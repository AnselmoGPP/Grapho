
#include <iostream>
#include <cmath>
#include <numbers>

#include "noise.hpp"


SingleNoise::SingleNoise(
    FastNoiseLite::NoiseType NoiseType,
    int NumOctaves,
    float Lacunarity,
    float Persistence,
    float Scale,
    float Multiplier,
    unsigned CurveDegree,
    float OffsetX, float OffsetY, float OffsetZ,
    int Seed)
    :
    Noiser(),
    noiseType(NoiseType), 
    numOctaves(NumOctaves), lacunarity(Lacunarity), persistence(Persistence), 
    scale(Scale), multiplier(Multiplier), 
    curveDegree(CurveDegree), 
    offsetX(OffsetX), offsetY(OffsetY), offsetZ(OffsetZ), 
    seed(Seed)
{
    noise.SetNoiseType(noiseType);
    noise.SetSeed(seed);

    noise.SetFractalLacunarity(lacunarity);
    noise.SetFractalGain(persistence);
    noise.SetFractalOctaves(numOctaves);    // Set to 0 or 1 if getProcessedNoise is used

    noise.SetFrequency(0.01f);
    noise.SetFractalType(FastNoiseLite::FractalType::FractalType_FBm);
    noise.SetFractalWeightedStrength(0.0f);

    noise.SetFractalPingPongStrength(2.0f);
    noise.SetRotationType3D(FastNoiseLite::RotationType3D::RotationType3D_None);

    // Cellular
    noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction::CellularDistanceFunction_EuclideanSq);
    noise.SetCellularReturnType(FastNoiseLite::CellularReturnType::CellularReturnType_Distance);
    noise.SetCellularJitter(1.0f);

    // Domain warp
    noise.SetDomainWarpType(FastNoiseLite::DomainWarpType::DomainWarpType_OpenSimplex2);
    noise.SetDomainWarpAmp(1.0f);
    //noise.DomainWarp(FNfloat & x, FNfloat & y, FNfloat & z);

    totalAmplitude = 0;
    float amplitude = 1;
    for (int i = 0; i < numOctaves; i++)
    {
        totalAmplitude += amplitude;
        amplitude *= persistence;
    }

    maxHeight = multiplier * scale;

    std::mt19937_64 engine;
    engine.seed(seed);
    std::uniform_int_distribution<int> distribution(-10000, 10000);
    std::cout << "Random: " << distribution(engine) << ", " << distribution(engine) << ", " << distribution(engine) << std::endl;

    std::cout << *this;
}

float SingleNoise::getProcessedNoise(float x, float y, float z)
{
    result = 0;
    frequency = 1;
    amplitude = 1;
    totalAmplitude = 0;   // Used for normalizing result to [-1.0, 1.0]

    for (int i = 0; i < numOctaves; i++)
    {
        result += noise.GetNoise(               // GetNoise returns values in range [-1, 1]
            ((x + offsetX) / scale) * frequency,
            ((y + offsetY) / scale) * frequency,
            ((z + offsetZ) / scale) * frequency) * amplitude;

        totalAmplitude += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    result = multiplier * scale * result / totalAmplitude;
    return result * std::pow(result / maxHeight, curveDegree);
}

float SingleNoise::GetNoise(float x, float y, float z)
{
    result = multiplier * scale * noise.GetNoise((x + offsetX) / scale, (y + offsetY) / scale, (z + offsetZ) / scale) / totalAmplitude;
    return result * std::pow(result / maxHeight, curveDegree);
}

float SingleNoise::GetNoise(float x, float y)
{
    result = multiplier * scale * noise.GetNoise((x + offsetX) / scale, (y + offsetY) / scale) / totalAmplitude;
    return result * std::pow(result / maxHeight, curveDegree);
}

float SingleNoise::powLinInterp(float base, float exponent)
{
    float down = std::floor(exponent);
    float up   = down + 1;
    float diff = exponent - down;
    
    up = std::pow(base, up);
    down = std::pow(base, down);

    return down + diff * (up - down);
}

void SingleNoise::noiseTester(size_t size)
{
    float max = 0, min = 0;
    float noise;

    for (size_t i = 1; i != size; i++)
    {
        std::cout << "%: " << 100 * (float)i / size << "     \r";
        for (size_t j = 1; j != size; j++)
        {
            noise = GetNoise(i, j);
            if (noise > max) max = noise;
            else if (noise < min) min = noise;
        }
    }
}

std::ostream& operator << (std::ostream& os, const SingleNoise& obj)
{
    std::cout
        << "Noise data: \n"
        << "   Octaves: " << obj.numOctaves << '\n'
        << "   Max amplitude: " << obj.totalAmplitude << '\n'
        << "   Max height: " << obj.maxHeight << '\n'
        << "   Scale: " << obj.scale << '\n'
        << "   Multiplier: " << obj.multiplier << '\n'
        << "   Lacunarity: " << obj.lacunarity << '\n'
        << "   Persistence: " << obj.persistence << '\n'
        << "   CurveDegree: " << obj.curveDegree << '\n'
        << "   Offsets: (" << obj.offsetX << ", " << obj.offsetY << ", " << obj.offsetZ << ")\n";

    return os;
}

// ----------------------------------------------------------------------------------

void fillAxis(float array[6][6], float sizeOfAxis)
{
    array[0][0] = 0.f;  // First vertex
    array[0][1] = 0.f;
    array[0][2] = 0.f;
    array[0][3] = 1.f;  // Color
    array[0][4] = 0.f;
    array[0][5] = 0.f;

    array[1][0] = sizeOfAxis;
    array[1][1] = 0.f;
    array[1][2] = 0.f;
    array[1][3] = 1.f;
    array[1][4] = 0.f;
    array[1][5] = 0.f;

    array[2][0] = 0.f;
    array[2][1] = 0.f;
    array[2][2] = 0.f;
    array[2][3] = 0.f;
    array[2][4] = 1.f;
    array[2][5] = 0.f;

    array[3][0] = 0.f;
    array[3][1] = sizeOfAxis;
    array[3][2] = 0.f;
    array[3][3] = 0.f;
    array[3][4] = 1.f;
    array[3][5] = 0.f;

    array[4][0] = 0.f;
    array[4][1] = 0.f;
    array[4][2] = 0.f;
    array[4][3] = 0.f;
    array[4][4] = 0.f;
    array[4][5] = 1.f;

    array[5][0] = 0.f;
    array[5][1] = 0.f;
    array[5][2] = sizeOfAxis;
    array[5][3] = 0.f;
    array[5][4] = 0.f;
    array[5][5] = 1.f;
}

void fillSea(float array[6][10], float height, float transparency, float x0, float y0, float x1, float y1)
{
    // 3 vertex position + 4 colors + 3 normal = 10 parameters

    float color[4] = { 0.1f, 0.1f, 0.8f, transparency };

    array[0][0] = x0;
    array[0][1] = y0;
    array[0][2] = height;
    array[0][3] = color[0];
    array[0][4] = color[1];
    array[0][5] = color[2];
    array[0][6] = color[3];
    array[0][7] = 0;
    array[0][8] = 0;
    array[0][9] = 1;

    array[1][0] = x1;
    array[1][1] = y0;
    array[1][2] = height;
    array[1][3] = color[0];
    array[1][4] = color[1];
    array[1][5] = color[2];
    array[1][6] = color[3];
    array[1][7] = 0;
    array[1][8] = 0;
    array[1][9] = 1;

    array[2][0] = x0;
    array[2][1] = y1;
    array[2][2] = height;
    array[2][3] = color[0];
    array[2][4] = color[1];
    array[2][5] = color[2];
    array[2][6] = color[3];
    array[2][7] = 0;
    array[2][8] = 0;
    array[2][9] = 1;

    array[3][0] = x1;
    array[3][1] = y0;
    array[3][2] = height;
    array[3][3] = color[0];
    array[3][4] = color[1];
    array[3][5] = color[2];
    array[3][6] = color[3];
    array[3][7] = 0;
    array[3][8] = 0;
    array[3][9] = 1;

    array[4][0] = x1;
    array[4][1] = y1;
    array[4][2] = height;
    array[4][3] = color[0];
    array[4][4] = color[1];
    array[4][5] = color[2];
    array[4][6] = color[3];
    array[4][7] = 0;
    array[4][8] = 0;
    array[4][9] = 1;

    array[5][0] = x0;
    array[5][1] = y1;
    array[5][2] = height;
    array[5][3] = color[0];
    array[5][4] = color[1];
    array[5][5] = color[2];
    array[5][6] = color[3];
    array[5][7] = 0;
    array[5][8] = 0;
    array[5][9] = 1;
}

void fillCubicSphere(float*** array, unsigned*** indices, float radius, unsigned numVertexPerSide, float R, float G, float B)
{
    float *** vertex = array;        //float array[6][numVertexPerSide * numVertexPerSide][6]
    unsigned *** ind = indices;      //unsigned indices[6][(numVertexPerSide-1)*(numVertexPerSide-1)*2][3]

    // Get position, color and normals

    for(size_t i = 0; i < numVertexPerSide; i++)
    {

    }




    // Get indices




}

