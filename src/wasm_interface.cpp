#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <vector>
#include <cstdio>      // For fopen, fwrite, fread, fclose, fseek, ftell, rewind
#include <stdexcept>   // For std::runtime_error
#include <string>      // For std::string
#include <algorithm>   // For std::transform
#include <iostream>    // For std::cerr / debugging
#include <dirent.h>    // For directory listing (opendir, readdir, closedir)
#include <sys/stat.h>  // For stat

#include "ncmcrypt.h"  // 假设这个头文件定义了 NeteaseCrypt 类

using namespace emscripten;

// Auxiliary function: convert string to lowercase (still useful for other string ops if needed)
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

// 解密 NCM 文件的 C++ 函数
// outputBaseNameFromJS 参数现在主要用于 JS 端生成最终下载文件名，
// 在 C++ 端不再用于查找 VFS 文件。
val decryptNCM(const val &inputData, const std::string &outputBaseNameFromJS) {
    std::vector<uint8_t> data = vecFromJSArray<uint8_t>(inputData);

    const std::string inputPath = "/work/input.ncm";
    const std::string workDir = "/work/"; // 工作目录

    FILE* inFile = fopen(inputPath.c_str(), "wb");
    if (!inFile) {
        throw std::runtime_error("Failed to open input file in VFS for writing: " + inputPath);
    }
    size_t bytesWritten = fwrite(data.data(), 1, data.size(), inFile);
    fclose(inFile);
    if (bytesWritten != data.size()) {
        throw std::runtime_error("Failed to write all input data to VFS: " + inputPath + " (Expected " + std::to_string(data.size()) + " bytes, wrote " + std::to_string(bytesWritten) + " bytes)");
    }

    std::string actualOutputFilePath;

    try {
        // Assume NeteaseCrypt's constructor takes std::string or const char* based on previous fixes.
        // If it still gives constructor error, provide the NeteaseCrypt class definition.
        NeteaseCrypt crypt(inputPath);
        crypt.Dump(workDir);           // 解密并输出到 /work 目录
        crypt.FixMetadata();           // 修复元数据

        // --- 核心改进：直接尝试查找 input.mp3 或 input.flac ---
        std::string assumedMp3Path = workDir + "input.mp3";
        std::string assumedFlacPath = workDir + "input.flac";

        FILE* outFile = fopen(assumedMp3Path.c_str(), "rb");
        if (outFile) {
            actualOutputFilePath = assumedMp3Path;
        } else {
            outFile = fopen(assumedFlacPath.c_str(), "rb");
            if (outFile) {
                actualOutputFilePath = assumedFlacPath;
            }
        }

        if (!outFile) {
            // 如果仍然找不到，说明 NeteaseCrypt 行为不同，或解密失败
            throw std::runtime_error("Decrypted output file (input.mp3 or input.flac) not found in VFS /work directory after decryption. "
                                     "NeteaseCrypt might have failed to create a valid output, "
                                     "or it named the file differently than 'input.mp3' or 'input.flac'.");
        }

        fseek(outFile, 0, SEEK_END);
        size_t size = ftell(outFile);
        rewind(outFile);
        std::vector<uint8_t> result(size);
        size_t bytesRead = fread(result.data(), 1, size, outFile);
        fclose(outFile);
        if (bytesRead != size) {
            throw std::runtime_error("Failed to read all output data from VFS: " + actualOutputFilePath +
                                     " (Expected " + std::to_string(size) + " bytes, read " + std::to_string(bytesRead) + " bytes)");
        }

        EM_ASM_({
            console.log('C++ Decryption successful, actual output file: ' + UTF8ToString($0));
        }, actualOutputFilePath.c_str());

        return val(typed_memory_view(result.size(), result.data()));

    } catch (const std::exception &e) {
        std::string errorMessage = "Decryption failed: " + std::string(e.what());
        std::cerr << "C++ Exception: " << errorMessage << std::endl;

        try {
            remove(inputPath.c_str());
            if (!actualOutputFilePath.empty()) {
                remove(actualOutputFilePath.c_str());
            }
        } catch (const std::exception& cleanup_e) {
            std::cerr << "Error during VFS cleanup after C++ exception: " << cleanup_e.what() << std::endl;
        }
        throw std::runtime_error(errorMessage);
    }
    // 成功时的清理
    try {
        remove(inputPath.c_str());
        if (!actualOutputFilePath.empty()) {
            remove(actualOutputFilePath.c_str());
        }
    } catch (const std::exception& cleanup_e) {
        std::cerr << "Error during VFS cleanup after success: " << cleanup_e.what() << std::endl;
    }
}

EMSCRIPTEN_BINDINGS(ncmdump_module) {
    // Make sure this matches the constructor in ncmcrypt.h for NeteaseCrypt
    // If you're still getting constructor errors for NeteaseCrypt(inputPath),
    // you MUST show me the NeteaseCrypt class definition from ncmcrypt.h
    function("decryptNCM", &decryptNCM);
}