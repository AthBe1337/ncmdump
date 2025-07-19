#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <vector>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include "ncmcrypt.h"

using namespace emscripten;

val decryptNCM(const val &inputData, const std::string &outputBaseNameFromJS) {
    // Convert the incoming JavaScript data (which should be a Uint8Array)
    // into a C++ std::vector<uint8_t>.
    std::vector<uint8_t> input_data_vector = vecFromJSArray<uint8_t>(inputData);

    // Pass data pointer and size to the in-memory constructor
    NeteaseCrypt crypt(input_data_vector.data(), input_data_vector.size());
    crypt.DumpToMemory();
    crypt.FixMetadata();

    const std::vector<uint8_t>& finalDecryptedData = crypt.getDecryptedAudioData();

    // Return the final decrypted data to JS as a Uint8Array.
    return val(typed_memory_view(finalDecryptedData.size(), finalDecryptedData.data()));
}

EMSCRIPTEN_BINDINGS(ncmdump_module) {
    function("decryptNCM", &decryptNCM);
}