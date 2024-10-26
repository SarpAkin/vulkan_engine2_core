#include "util.hpp"

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <vector>

#include <cxxabi.h>

#ifndef _WIN32
#include <execinfo.h>
#endif

#include "arena_alloc.hpp"

#if defined(__linux__) || defined(__APPLE__)
    #include <pthread.h>
#elif defined(_WIN32)
    #include <windows.h>
    #include <processthreadsapi.h>
#endif


namespace vke {

std::vector<u8> read_file_binary(const char* name) {
    std::ifstream file(name, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<u8> buffer(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    return buffer;
}

std::span<u32> cast_u8_to_span_u32(std::span<u8> span) {
    if (span.size() % sizeof(u32) != 0) {
        throw std::runtime_error("cast_u8_to_span_u32: span size is not multiple of sizeof(u32)");
    }

    return std::span<u32>(reinterpret_cast<u32*>(span.data()), span.size() / sizeof(u32));
}

std::string read_file(const char* name) {
    std::ifstream file(name);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + std::string(name));
    }

    return std::string(std::istreambuf_iterator<char>(file), {});
}

std::string relative_path_impl(const char* source_path, const char* path) {
    std::filesystem::path p = source_path;
    return (p.parent_path() / path).string();
}

std::span<u8> read_file_binary(vke::ArenaAllocator* arena, const char* name) {
    std::ifstream file(name, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    u8* data        = arena->alloc<u8>(fileSize);

    file.seekg(0);
    file.read(reinterpret_cast<char*>(data), fileSize);
    return std::span(data, fileSize);
}

std::string_view read_file(vke::ArenaAllocator* arena, const char* name) {
    std::ifstream file(name, std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }
    size_t filesize = static_cast<size_t>(file.tellg());
    char* data      = arena->alloc<char>(filesize + 1);
    data[filesize] = '\0';

    file.seekg(0);
    file.read(reinterpret_cast<char*>(data), filesize);
    return std::string_view(data, filesize);
}



struct ParsedSymbol{
    char file_path[256];
    char function_name[240];
    size_t offset;
    size_t address;
};

bool parse_symbol(const char* input,ParsedSymbol* out) {

    // Try parsing the string assuming both function and offset are present
    if (sscanf(input, "%255[^()]%*[(]%239[^+]+%tx%*[)] [%tx]", out->file_path, out->function_name, &out->offset, &out->address) == 4) return true;
    
    if (sscanf(input, "%255[^()](+%tx%*[)] [%tx]", out->file_path, &out->offset, &out->address) == 3) {
        out->function_name[0] = '\0';

        return true;
    } 

    return false;
}


void demangle_symbols(const char* symbol,std::span<char> out_buf){
    ParsedSymbol parsed_symbol;

    char pretty_name[256];
    char* function_name = nullptr;

    if (parse_symbol(symbol, &parsed_symbol) && parsed_symbol.function_name[0] != '\0') {
        int status;
        size_t len = 256;
        abi::__cxa_demangle(parsed_symbol.function_name, pretty_name, &len, &status);

        if (status == 0) {
            function_name = pretty_name;
        }
    }

    if (function_name == nullptr) {
        function_name = parsed_symbol.function_name;
    }

    snprintf(out_buf.data(),out_buf.size(), "%s(%s) [%lx]", parsed_symbol.file_path,function_name, parsed_symbol.address);
}

void trace_stack(FILE* stream) {
#ifndef _WIN32
    void *buffer[100];
    int nptrs;

    // Get the array of return addresses
    nptrs = backtrace(buffer, sizeof(buffer) / sizeof(void*));

    // Print out all the frames to stderr

    char **symbols = backtrace_symbols(buffer, nptrs);

    char name_buffer[256];
    for(int i = 1;i < nptrs; ++i) {
        demangle_symbols(symbols[i], name_buffer);
        fprintf(stream, "%s\n", name_buffer);
    }

    free(symbols);
#endif
}

void name_thread(std::thread& thread, const char* name) {
#if defined(__linux__) || defined(__APPLE__)
    pthread_setname_np(thread.native_handle(), name);
#elif defined(_WIN32)
    SetThreadDescription(thread.native_handle(), std::wstring(name, name + strlen(name)).c_str());
#endif

}
} // namespace vke
