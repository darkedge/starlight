#pragma once
#include <string>

// NOTE: Platform layer can't use exported functions in debug (linker errors!)
namespace logger {
    SL_EXPORT(void) LogInfo(const std::string& str);

    void Render();

    SL_EXPORT(void) DestroyLogger();
}

#define LOGGER_LOGINFO(name) void name(const std::string&)
typedef LOGGER_LOGINFO(LogInfoFunc);

#ifdef SL_IMPL
LogInfoFunc* g_LogInfo;
#else 
extern LogInfoFunc* g_LogInfo;
#endif
