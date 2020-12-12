#include <iostream>
#include <cstring>
#include <chrono>
#include <map>
#include <vector>
#include <cmath>

constexpr float g_quantizationMatrix[] = {
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
};

constexpr uint32_t g_zigZagTraversalIdx[] = {
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};


// This does not pertain to just MCUs anymore
class MCU {
    private:
        std::map<uint32_t, std::vector<int>> elements;
        std::map<uint32_t, std::vector<float>> DCTCoefficientsMap;
        std::map<uint32_t, std::vector<int>> RLEMap;
        std::vector<int32_t> DCCoefficients;
        uint32_t currentIdx {0};
        uint32_t mcuSize {8};
    public:
        void CreateMCUs(uint32_t *array, uint32_t size, uint32_t rows, uint32_t cols);
        uint32_t NumMCUs() const;
        void PrintMCU(uint32_t idx) const;
        void PrintDCTCoefficients(uint32_t idx) const;
        void ComputeDCT();
        void Quantify();
        void PrintZigZagDCTCoefficients(uint32_t idx) const;
        void GenerateRLE();
        void PrintRLEMap(uint32_t idx) const;
        void GenerateDCCoefficientDifferences();
        void PrintDCCoefficientDifferences(uint32_t idx) const;
};

void MCU::PrintDCCoefficientDifferences(uint32_t idx) const {
    std::cout << "\n";
    for (uint32_t ele = 0U; ele < DCCoefficients.size(); ele++) {
        std::cout << " " << DCCoefficients[ele];
    }
}

void MCU::GenerateDCCoefficientDifferences() {
    for (uint32_t idx = 0U; idx < DCTCoefficientsMap.size(); idx++) {
        auto ret = DCTCoefficientsMap.at(idx);
        int32_t difference = 0;
        if (idx) {
            auto previousRet = DCTCoefficientsMap.at(idx - 1);
            std::cout << "Ret[0]: " << ret[0] << "\n";
            std::cout << "previousRet[0]: " << previousRet[0] << "\n";
            difference = ret[0] - previousRet[0];
        }
        DCCoefficients.emplace_back(difference);
    }
}

void MCU::PrintRLEMap(uint32_t idx) const {
    std::cout << "\n";
    auto ret = RLEMap.at(idx);
    for (uint32_t ele = 0U; ele < ret.size(); ele++) {
        std::cout << " " << ret[ele];
    }
}

void MCU::GenerateRLE() {
    std::cout << "Size: " << DCTCoefficientsMap.size() << "\n";
    for (uint32_t idx = 0U; idx < DCTCoefficientsMap.size(); idx++) {
        auto ret = DCTCoefficientsMap.at(idx);
        std::vector<int> RLE;
        uint32_t RLEidx = 0U;
        uint32_t numZeroes = 0U;
        for (uint32_t i = 1U; i < 64U; i++) {
            if (ret[g_zigZagTraversalIdx[i]] == 0) {
                numZeroes++;
            } else {
                RLE.emplace_back(numZeroes);
                numZeroes = 0U;
                RLE.emplace_back(ret[g_zigZagTraversalIdx[i]]);
            }
        }
        RLE.emplace_back(0);
        RLE.emplace_back(0);
        RLEMap.emplace(std::make_pair(idx, RLE));
    }
}

void MCU::PrintZigZagDCTCoefficients(uint32_t idx) const {
    auto ret = DCTCoefficientsMap.at(idx);
    for (uint32_t ele = 0; ele < 64; ele++) {
        std::cout << " " << ret[g_zigZagTraversalIdx[ele]];
        if ((ele + 1 ) % 8 == 0) {
            std::cout << "\n";
        }
    }
}

void MCU::Quantify() {
    for (uint32_t idx = 0U; idx < DCTCoefficientsMap.size(); idx++) {
        auto ret = DCTCoefficientsMap.at(idx);
        std::vector<float> quantifiedCoefficients(64, 0.0F);
        quantifiedCoefficients.reserve(64);
        for (uint32_t coefficientIdx = 0U; coefficientIdx < 64U; coefficientIdx++) {
            quantifiedCoefficients[coefficientIdx] = std::roundf(ret[coefficientIdx] / g_quantizationMatrix[coefficientIdx]);
        }
        DCTCoefficientsMap[idx] = quantifiedCoefficients;
    }
}

void MCU::CreateMCUs(uint32_t *array, uint32_t size, uint32_t rows, uint32_t cols)
{
    // Four nested for loops because why not?
    uint32_t mapIdx = 0U;
    for (uint32_t outerRows = 0U; outerRows <= (size - (rows * mcuSize)); outerRows += rows * mcuSize) {
        for (uint32_t innerRow = outerRows; innerRow <= ((outerRows + rows) - mcuSize); innerRow += mcuSize) {
            std::vector<int> tempVector;
            for (uint32_t stride = 0U; stride < mcuSize; stride++) {
                for (uint32_t idx = 0U; idx < mcuSize; idx++) {
                    int val = array[innerRow + (stride * rows) + idx];
                    tempVector.emplace_back(val);
                }
            }
            elements.emplace(std::make_pair(mapIdx++, tempVector));
        }
    }
}

void MCU::PrintMCU(uint32_t idx) const {
    auto ret = elements.at(idx);
    std::cout << "--------------\n";
    for (uint32_t ele = 0; ele < 64; ele++) {
        std::cout << " " << ret[ele];
        if ((ele + 1 ) % 8 == 0) {
            std::cout << "\n";
        }
    }
}

void MCU::PrintDCTCoefficients(uint32_t idx) const {
    auto ret = DCTCoefficientsMap.at(idx);
    std::cout << "--------------\n";
    for (uint32_t ele = 0; ele < 64; ele++) {
        std::cout << " " << ret[ele];
        if ((ele + 1 ) % 8 == 0) {
            std::cout << "\n";
        }
    }
}

void MCU::ComputeDCT()
{
    for (uint32_t idx = 0U; idx < elements.size(); idx++) {

        std::vector<float> DCTCoefficients(64, 0.0F);
        DCTCoefficients.reserve(64);
        auto currentMCU = elements.at(idx);
        for (uint32_t v = 0; v < 8; v++) {
            for (uint32_t u = 0; u < 8; u++) {
                float DCTCoefficient = 0;
                for (uint32_t x = 0U; x < 8U; x++) {
                    for (uint32_t y = 0U; y < 8U; y++) {
                        DCTCoefficient += (currentMCU[(x * 8) + y]) * (std::cos((((2 * x) + 1) * u * M_PI) / 16.0)) *
                                          (std::cos((((2 * y) + 1) * v * M_PI) / 16.0));
                    }
                }
                float alphaU = ((u == 0) ? (1.0 / std::sqrt(2.0)) : 1.0);
                float alphaV = ((v == 0) ? (1.0 / std::sqrt(2.0)) : 1.0);
                float tempValue = 0.25 * alphaU * alphaV;
                DCTCoefficients[8*u + v] = tempValue * DCTCoefficient;
            }
        }
        DCTCoefficientsMap.emplace(std::make_pair(idx, DCTCoefficients));
    }
}



uint32_t MCU::NumMCUs() const {
    return elements.size();
}

int main()
{
#ifdef __linux__
    //const char *sourcePath = "/home/sruthik/repos/image-utils/rgb888_2560.ppm";
    const char *sourcePath = "/home/sruthik/repos/image-utils/rgb888_24x13.ppm";
    const char *destinationPath = "/home/sruthik/repos/image-utils/nv12_image_st.y4m";
#else
    const char *sourcePath = "C:\\Users\\psrut\\Downloads\\image.ppm";
    const char *destinationPath = "C:\\Users\\psrut\\Downloads\\image_yuv444.y4m";
#endif // __linux__
    const uint32_t numComponents = 3U;
    FILE *sourceFile = nullptr;
    FILE *destinationFile = nullptr;
    char fileHeader[1024];
    uint32_t width = 0U;
    uint32_t height = 0U;
    uint32_t maxValue = 0U;
    uint32_t *sourceBuffer = nullptr;
    //uint32_t *yPlaneBuffer = nullptr;
    //uint32_t *uPlaneBuffer = nullptr;
    //uint32_t *vPlaneBuffer = nullptr;
    uint64_t sourceBufferSize = 0U;
    MCU yMCU;
    MCU uMCU;
    MCU vMCU;

    // Y4M Header constants
    const char* y4mHeader = "YUV4MPEG2";
    const char* frameRate = "F25:1 Ip";
    const char* frameBeginMarker = "FRAME";
    const char* colorSpace = "C444";
    const char* pixelAspectRatio = "A1:1";

    // YUV conversion constants
    const float Wr = 0.299F;
    const float Wb = 0.114F;
    const float Wg = 0.587F;
    const float Umax = 0.436F;
    const float Vmax = 0.615F;

    // sourcePath is moved to esi
    sourceFile = fopen(sourcePath, "rb"); // call fopen. return value of fopen is stored in eax/rax which is then moved to -56(%rbp)
    if (!sourceFile) { // We jump to L2 if sourceFile != 0
        printf("Failed to open file for reading\n"); // The string is stored at LC13 and is moved to edi before calling puts.
        return -1; // -1 is stored in eax before going to L49
    }

    // destinationPath is moved to esi
    destinationFile = fopen(destinationPath, "wb");
    if (!destinationFile) { // Jump to L4 if destinationFile != 0
        printf("Failed to open file for writing\n");
        return -1;
    }

    fscanf(sourceFile, "%s", fileHeader);
    if (strcmp(fileHeader, "P3") != 0) {
        printf("Invalid PPM File\n");
        return -1;
    }

    fscanf(sourceFile, "%i %i", &width, &height);

    fscanf(sourceFile, "%i", &maxValue);
    sourceBufferSize = width * height * numComponents;
    std::cout << "sourceBufferSize : " << sourceBufferSize << "\n";
    sourceBuffer = new uint32_t[sourceBufferSize];
    std::cout << "Plane size: " << sourceBufferSize / numComponents << "\n";
    uint32_t *yPlaneBuffer = new uint32_t[sourceBufferSize / numComponents];
    uint32_t *uPlaneBuffer = new uint32_t[sourceBufferSize / numComponents];
    uint32_t *vPlaneBuffer = new uint32_t[sourceBufferSize / numComponents];

    uint32_t idx = 0U;
    while ((fgetc(sourceFile)) != EOF) {
        fscanf(sourceFile, "%i", &sourceBuffer[idx++]);
    }

    // Convert RGB to YUV444
    uint32_t currentPos = 0U;
    idx = 0U;
    std::cout << "Source buffer size: " << sourceBufferSize << "\n";
    auto startTime = std::chrono::high_resolution_clock::now();
    for (; idx < sourceBufferSize; idx+=3) {
        yPlaneBuffer[currentPos] = (0.257F * sourceBuffer[idx]) + (0.504F * sourceBuffer[idx + 1]) + (0.098F * sourceBuffer[idx + 2]) + 16;
        uPlaneBuffer[currentPos] = (0.439F * sourceBuffer[idx + 2]) - (0.148F * sourceBuffer[idx]) - (0.291F * sourceBuffer[idx + 1]) + 128;
        vPlaneBuffer[currentPos] = 0.439F * sourceBuffer[idx] - (0.368F * sourceBuffer[idx+1]) - (0.071F * sourceBuffer[idx + 2]) + 128;
        if (vPlaneBuffer[currentPos] > 255U) {
            vPlaneBuffer[currentPos] = 255U;
        }
        currentPos++;
    }

    int shift = 10;
    for (int i = 0; i < (sourceBufferSize / numComponents) - width; i += 8 * width) {
        int k = i;
        for (; k < i + (8 * width); k += width) {
            for (int j = 0; j < 8; j++) {
                uPlaneBuffer[k + j] = shift;
            }
        }
        shift += shift;
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cout << "RGB to YUV Conversion time: " << duration << "ms\n";
/*
    fprintf(destinationFile, "%s", y4mHeader);
    fprintf(destinationFile, " %c%i %c%i", 'W', width, 'H', height);
    fprintf(destinationFile, " %s", frameRate);
    fprintf(destinationFile, " %s ", pixelAspectRatio);
    fprintf(destinationFile, "%s\n", colorSpace);

    startTime = std::chrono::high_resolution_clock::now();
    for (uint32_t n = 0U; n < 50; n++) {
        fprintf(destinationFile, "%s\n", frameBeginMarker);
        for (uint32_t idx = 0U; idx < (sourceBufferSize / numComponents); idx++) {
            fprintf(destinationFile, "%c", yPlaneBuffer[idx]);
        }

        for (uint32_t idx = 0U; idx < (sourceBufferSize / numComponents); idx++) {
            fprintf(destinationFile, "%c", uPlaneBuffer[idx]);
        }

        for (uint32_t idx = 0U; idx < (sourceBufferSize / numComponents); idx++) {
            fprintf(destinationFile, "%c", vPlaneBuffer[idx]);
        }
    }
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
    std::cout << "Writing time: " << duration << "s\n";

*/
    // Create a copy of the YUV buffers for JPEG Encoding
    uint32_t *yPlaneCopy = new uint32_t[sourceBufferSize / numComponents];
    uint32_t *uPlaneCopy = new uint32_t[sourceBufferSize / numComponents];
    uint32_t *vPlaneCopy = new uint32_t[sourceBufferSize / numComponents];

    memcpy(yPlaneCopy, yPlaneBuffer, sizeof(yPlaneBuffer));
    memcpy(uPlaneCopy, uPlaneBuffer, sizeof(uPlaneBuffer));
    memcpy(vPlaneCopy, vPlaneBuffer, sizeof(vPlaneBuffer));

    // Shift range of pixels from [0, 255] to [-128, 128]
    for (uint32_t idx = 0U; idx < sourceBufferSize / numComponents; idx++) {
        yPlaneCopy[idx] -= 128U;
        uPlaneCopy[idx] -= 128U;
        vPlaneCopy[idx] -= 128U;
    }

    // The raw pixel data now needs to be divided into 8x8 blocks
    // (MCUs).
    //
    // For now assume the dimensions of the image to be divisible by 8
    startTime = std::chrono::high_resolution_clock::now();
    yMCU.CreateMCUs(yPlaneCopy, (sourceBufferSize / numComponents), height, width);
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cerr << "Y MCU Creation time: " << duration << "ms\n";
    startTime = std::chrono::high_resolution_clock::now();
    uMCU.CreateMCUs(uPlaneCopy, (sourceBufferSize / numComponents), height, width);
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cerr << "U MCU Creation time: " << duration << "ms\n";
    startTime = std::chrono::high_resolution_clock::now();
    vMCU.CreateMCUs(vPlaneCopy, (sourceBufferSize / numComponents), height, width);
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cerr << "V MCU Creation time: " << duration << "ms\n";
    startTime = std::chrono::high_resolution_clock::now();
    yMCU.ComputeDCT();
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cerr << "Y DCT time: " << duration << "ms\n";
    startTime = std::chrono::high_resolution_clock::now();
    uMCU.ComputeDCT();
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cout << "U DCT time: " << duration << "ms\n";
    startTime = std::chrono::high_resolution_clock::now();
    vMCU.ComputeDCT();
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cout << "V DCT time: " << duration << "ms\n";

    // Quantization step
    startTime = std::chrono::high_resolution_clock::now();
    yMCU.Quantify();
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cerr << "Y Quantization time: " << duration << "ms\n";
    startTime = std::chrono::high_resolution_clock::now();
    uMCU.Quantify();
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cerr << "U Quantization time: " << duration << "ms\n";
    startTime = std::chrono::high_resolution_clock::now();
    vMCU.Quantify();
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cerr << "V Quantization time: " << duration << "ms\n";

    // Run length encoding of the quantized MCUs
    yMCU.GenerateRLE();
    uMCU.GenerateRLE();
    vMCU.GenerateRLE();

    // Store the differences of the DC coefficients
    yMCU.GenerateDCCoefficientDifferences();
    uMCU.GenerateDCCoefficientDifferences();
    vMCU.GenerateDCCoefficientDifferences();

    yMCU.PrintDCCoefficientDifferences(47);

    delete[] sourceBuffer;
    //delete[] yPlaneBuffer;
    //delete[] uPlaneBuffer;
    if (vPlaneBuffer)
    delete[] vPlaneBuffer;
    delete[] yPlaneCopy;
    delete[] uPlaneCopy;
    delete[] vPlaneCopy;
    fclose(sourceFile);
    fclose(destinationFile);
}
