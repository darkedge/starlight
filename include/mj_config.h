#pragma once
#include <map>
#include <string>

class MJConfig {
public:
    void Save() {
        const char* filename = "config.ini";

        FILE* f = fopen(filename, "wt");
        if (!f) {
            return;
        }
        for (const auto& pair : config) {
            fprintf(f, "%s=%s\n", pair.first.c_str(), pair.second.c_str());
        }
        fclose(f);
    }

    bool Load() {
        const char* filename = "config.ini";
        if (!filename) {
            return false;
        }

        int numBytes;
        char* buffer = LoadFileInMemory(filename, &numBytes, 1);
        if (!buffer) {
            return false;
        }

        const char* bufferEnd = buffer + numBytes;
        char key[64] = {};
        char val[64] = {};
        const char* lineStart = buffer;
        while (lineStart < bufferEnd) {
            const char* ch = lineStart;
            while (ch < bufferEnd && *ch != '\n' && *ch != '\r') {
                ch++;
            }
            if (sscanf(lineStart, "%[^=\n]=%[^=\n]", &key, &val) == 2) {
                config.insert(std::make_pair(std::string(key), std::string(val)));
            }

            lineStart = ch + 1;
        }

        delete[] buffer;

        return true;
    }

    void SetIntProperty(const std::string& key, int val) {
        config[key] = std::to_string(val);
    }

    int GetIntProperty(const std::string& key, int fallback) {
        if (!config.count(key)) {
            return fallback;
        }
        return std::stoi(config.at(key));
    }

private:
    char* LoadFileInMemory(const char* filename, int* outNumBytes, int padding_bytes)
    {
        if (!filename) {
            return NULL;
        }

        if (outNumBytes) {
            *outNumBytes = 0;
        }

        FILE* f = fopen(filename, "rb");
        if (!f) {
            return NULL;
        }

        long numBytes;
        if (fseek(f, 0, SEEK_END) || (numBytes = ftell(f)) == -1 || fseek(f, 0, SEEK_SET)) {
            fclose(f);
            return NULL;
        }

        // Create output buffer
        char* buffer = new char[numBytes + padding_bytes];
        if (!buffer) {
            fclose(f);
            return NULL;
        }

        // Read entire file
        if (fread(buffer, 1, (size_t)numBytes, f) != (size_t)numBytes) {
            fclose(f);
            delete[] buffer;
            return NULL;
        }

        // Add padding bytes if specified
        if (padding_bytes > 0) {
            memset(buffer + numBytes, 0, padding_bytes);
        }

        fclose(f);
        if (outNumBytes) {
            *outNumBytes = numBytes;
        }

        return buffer;
    }

    std::map<std::string, std::string> config;
};
