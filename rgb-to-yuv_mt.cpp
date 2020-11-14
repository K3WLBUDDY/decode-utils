#include <iostream>
#include <cstring>
#include <chrono>

struct bufferState {
    uint32_t *sourceBuffer;
    uint32_t *yPlaneBuffer;
    uint32_t *uPlaneBuffer;
    uint32_t *vPlaneBuffer;
};
bufferState g_state;

struct threadState {
    struct bufferState *globalBufferState;
    uint32_t id;
};

static uint32_t stride = 552960U;

static void *RgbToYuv(void *arg)
{
    static int a = 10;
    struct threadState *currentThreadState = reinterpret_cast<threadState *>(arg);
    uint32_t idx = currentThreadState->id * stride;
    uint32_t currentPos = idx / 3;
    uint32_t test = idx;
    for (;idx < test + stride; idx+=3) {
        currentThreadState->globalBufferState->yPlaneBuffer[currentPos] = (0.257F * currentThreadState->globalBufferState->sourceBuffer[idx]) +
                                                                          (0.504F * currentThreadState->globalBufferState->sourceBuffer[idx + 1]) +
                                                                          (0.098F * currentThreadState->globalBufferState->sourceBuffer[idx + 2]) + 16;

        currentThreadState->globalBufferState->uPlaneBuffer[currentPos] = (0.439F * currentThreadState->globalBufferState->sourceBuffer[idx + 2]) -
                                                                          (0.148F * currentThreadState->globalBufferState->sourceBuffer[idx]) -
                                                                          (0.291F * currentThreadState->globalBufferState->sourceBuffer[idx + 1]) + 128;

        currentThreadState->globalBufferState->vPlaneBuffer[currentPos] = 0.439F * currentThreadState->globalBufferState->sourceBuffer[idx] -
                                                                          (0.368F * currentThreadState->globalBufferState->sourceBuffer[idx+1]) -
                                                                          (0.071F * currentThreadState->globalBufferState->sourceBuffer[idx + 2]) + 128;

        currentPos++;
    }
    return &a;
}

int main()
{
#ifdef __linux__
    const char *sourcePath = "/home/sruthik/repos/image-utils/rgb888_2560.ppm";
    const char *destinationPath = "/home/sruthik/repos/image-utils/nv12_image_mt.y4m";
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
    g_state.sourceBuffer = nullptr;
    g_state.yPlaneBuffer = nullptr;
    g_state.uPlaneBuffer = nullptr;
    g_state.vPlaneBuffer = nullptr;
    uint64_t sourceBufferSize = 0U;

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

    // number of threads
    const uint32_t numThreads = 20U;

    // thread pool
    pthread_t workerThreads[numThreads];
    struct threadState ts[numThreads];

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
    g_state.sourceBuffer = new uint32_t[sourceBufferSize];
    g_state.yPlaneBuffer = new uint32_t[sourceBufferSize / numComponents];
    g_state.uPlaneBuffer = new uint32_t[sourceBufferSize / numComponents];
    g_state.vPlaneBuffer = new uint32_t[sourceBufferSize / numComponents];

    uint32_t idx = 0U;
    while ((fgetc(sourceFile)) != EOF) {
        fscanf(sourceFile, "%i", &g_state.sourceBuffer[idx++]);
    }

    // Launch worker threads
    auto startTime = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0U; i < numThreads; i++) {
        ts[i].globalBufferState = &g_state;
        ts[i].id = i;
        if ((pthread_create(&workerThreads[i], nullptr, &RgbToYuv, &ts[i])) != 0) {
            printf("Failed to create Thread %i\n", i);
        }
    }

    // Wait for threads to finish
    for (uint32_t i = 0U; i < numThreads; i++) {
        pthread_join(workerThreads[i], nullptr);
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cout << "Decode duration: " << duration << "ms\n";

    fprintf(destinationFile, "%s", y4mHeader);
    fprintf(destinationFile, " %c%i %c%i", 'W', width, 'H', height);
    fprintf(destinationFile, " %s", frameRate);
    fprintf(destinationFile, " %s ", pixelAspectRatio);
    fprintf(destinationFile, "%s\n", colorSpace);

    startTime = std::chrono::high_resolution_clock::now();
    for (uint32_t n = 0U; n < 50; n++) {
        fprintf(destinationFile, "%s\n", frameBeginMarker);
        for (uint32_t idx = 0U; idx < (sourceBufferSize / numComponents); idx++) {
            fprintf(destinationFile, "%c", g_state.yPlaneBuffer[idx]);
        }

        for (uint32_t idx = 0U; idx < (sourceBufferSize / numComponents); idx++) {
            fprintf(destinationFile, "%c", g_state.uPlaneBuffer[idx]);
        }

        for (uint32_t idx = 0U; idx < (sourceBufferSize / numComponents); idx++) {
            fprintf(destinationFile, "%c", g_state.vPlaneBuffer[idx]);
        }
    }
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
    std::cout << "Writing time: " << duration << "s\n";

    delete[] g_state.sourceBuffer;
    delete[] g_state.yPlaneBuffer;
    delete[] g_state.uPlaneBuffer;
    delete[] g_state.vPlaneBuffer;
    fclose(sourceFile);
    fclose(destinationFile);
}
