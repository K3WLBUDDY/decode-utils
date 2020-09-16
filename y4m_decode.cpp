#include <fstream>
#include<iostream>
#include <cstring>
#include <vector>
#include <cmath>
#include <iomanip>

static bool CheckY4MHeader(uint8_t *buffer)
{
    bool ret = true;

    // Check if the file starts with the bytes
    // YUV4MPEG2
    if (buffer[0] != 89 || buffer[1] != 85 ||
        buffer[2] != 86 || buffer[3] != 52 ||
        buffer[4] != 77 || buffer[5] != 80 ||
        buffer[6] != 69 || buffer[7] != 71 ||
        buffer[8] != 50) {
        ret = false;
    }

    return ret;
}

// TODO: Clean this mess up
static bool GetWidthAndHeight(uint8_t *buffer, uint32_t *width, uint32_t *height)
{
    bool ret = true;
    std::vector<uint32_t> temp;

    if (buffer[10] == 87) {
        uint32_t i = 11;
        while (buffer[i] != 0x20) {
            uint32_t tempInt = buffer[i] - '0';
            temp.emplace_back(tempInt);
            ++i;
        }
        for (auto idx = std::make_pair(temp.size() - 1, 0U); idx.first > 0U;
             idx.first--, idx.second++) {
            *width = *width + std::pow(10, idx.first)  * temp[idx.second];
        }
        temp.clear();
        if (buffer[i+1] == 72) {
            uint32_t idx = i+2;
            while (buffer[idx] != 0x20) {
                uint32_t tempInt = buffer[idx] - '0';
                temp.emplace_back(tempInt);
                ++idx;
            }
            for (auto idx_2 = std::make_pair(temp.size() - 1, 0U); idx_2.first > 0U;
                 idx_2.first--, idx_2.second++) {
                *height = *height + std::pow(10, idx_2.first)  * temp[idx_2.second];
            }
            temp.clear();
        } else {
            std::cout << "Failed to get height\n";
            ret = false;
        }
    } else {
        std::cout << "Failed to get width\n";
        ret = false;
    }

    return ret;
}

// Assume YUV420 as the input format
// TODO: Replace with memcpy?
static bool FillYUVPlanes(uint8_t *buffer, uint8_t *yPlaneData, uint8_t *uPlaneData,
                         uint8_t *vPlaneData, uint32_t width, uint32_t height, std::streampos size)
{
    bool ret = false;
    uint64_t idx = 0U;

    for (idx; idx < size; idx++) {
        if (buffer[idx] == 70 && buffer[idx+1] == 82 &&
            buffer[idx+2] == 65 && buffer[idx+3] == 77 &&
            buffer[idx+4] == 69) {
            if (buffer[idx + 5] == 0x0A) {
                ret = true;
                idx += 5;
                for (uint64_t i = 0U; i < width * height; i++) {
                    yPlaneData[i] = buffer[++idx];
                }
                for (uint64_t i = 0U; i < (width * height) / 4; i++) {
                    uPlaneData[i] = buffer[++idx];
                }
                for (uint64_t i = 0U; i < (width * height) / 4; i++) {
                    vPlaneData[i] = buffer[++idx];
                }
            }
            break;
        }
    }

    return ret;
}

static bool CreateHeaderFile(uint8_t *yPlaneData, uint8_t *uPlaneData, uint8_t *vPlaneData,
                             uint32_t width, uint32_t height)
{
    bool ret = true;

    std::fstream fs;
    const char *filePath = "/home/sruthik/repos/cpp_stuff/y4m_decode/ducks_nv12.hpp";

    fs.open(filePath, std::ios::out | std::ios::binary);

    if (!fs.is_open()) {
        std::cout << "Failed to open header file for writing!\n";
        return false;
    }

    fs << "#include <stdint.h>" << std::endl;
    fs << std::endl;
    fs << "#define YUV_IMAGE_WIDTH " << width << std::endl;
    fs << "#define YUV_IMAGE_HEIGHT " << height << std::endl;
    fs << std::endl;
    fs << "const uint8_t ducks_nv12[] = {" << std::endl;

    // Start spitting out Y plane data
    for (uint32_t idx = 0U; idx < width * height; idx++) {
        fs <<"    " << std::endl;
        fs << std::hex << yPlaneData[idx];
        if (idx % 10) {
            fs << std::endl;
        }
    }
    fs.close();

    return ret;
}

int main()
{
    // Modify filePath to point to the y4m file
    const char *filePath = "/home/sruthik/Downloads/ducks_take_off_420_720p50.y4m";
    std::streampos size;
    std::fstream fs;
    uint8_t *buffer = nullptr;
    uint32_t width = 0U;
    uint32_t height = 0U;
    
    fs.open(filePath, std::ios::in | std::ios::binary | std::ios::ate);

    if (!fs.is_open()) {
        std::cout << "Failed to open file. Is the file present?\n";
        return -1;
    } else {
        size = fs.tellg();
        buffer = static_cast<uint8_t *>(std::malloc(size * sizeof(uint8_t)));
        fs.seekg(0, std::ios::beg);
        fs.read((char *)(buffer), size);
        fs.close();
    }

    if (!CheckY4MHeader(buffer)) {
        std::cout << "File is not a valid y4m file\n";
        free(buffer);
        return -1;
    }

    if (!GetWidthAndHeight(buffer, &width, &height)) {
        std::cout << "Failed to get width and height of the buffer\n";
        free(buffer);
        return -1;
    } else {
    }

    uint8_t yPlaneData[width * height];
    uint8_t uPlaneData[(width * height) / 4];
    uint8_t vPlaneData[(width * height) / 4];

    if (!FillYUVPlanes(buffer, yPlaneData, uPlaneData, vPlaneData,
                      width, height, size)) {
        std::cout << "Failed to get plane data\n";
        free(buffer);
        return -1;
    }

    free(buffer);
}
