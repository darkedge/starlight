#pragma once
#include <string>


namespace logger {
	void Init();
	void LogInfo(const std::string& str);
	void Render();
	void Destroy();
}

// Pulled out of the namespace for convenience
using logger::LogInfo;
