#include "../../include/log/MultiOutputLogConfig.h"
#include <fstream>

bool MultiOutputConfigSerializer::SaveToFile(const MultiOutputLogConfig& config, const std::wstring& filePath) {
	try {
		std::wofstream file(filePath);
		if (!file.is_open()) {
			return false;
		}

		file << L"ConfigVersion=" << config.configVersion << std::endl;
		file << L"Enabled=" << (config.enabled ? L"true" : L"false") << std::endl;
		file << L"GlobalMinLevel=" << static_cast<int>(config.globalMinLevel) << std::endl;
		file << L"OutputCount=" << config.outputs.size() << std::endl;

		file.close();
		return true;
	}
	catch (...) {
		return false;
	}
}

bool MultiOutputConfigSerializer::LoadFromFile(const std::wstring& filePath, MultiOutputLogConfig& config) {
	try {
		std::wifstream file(filePath);
		if (!file.is_open()) {
			return false;
		}

		config.enabled = true;
		config.configVersion = L"1.0";
		config.globalMinLevel = LogLevel::Trace;
		config.outputs.clear();

		std::wstring line;
		while (std::getline(file, line)) {
			if (line.find(L"Enabled=") == 0) {
				config.enabled = (line.find(L"true") != std::wstring::npos);
			}
			else if (line.find(L"ConfigVersion=") == 0) {
				config.configVersion = line.substr(14);
			}
			else if (line.find(L"GlobalMinLevel=") == 0) {
				int level = std::stoi(line.substr(15));
				config.globalMinLevel = static_cast<LogLevel>(level);
			}
		}

		file.close();
		return true;
	}
	catch (...) {
		return false;
	}
}