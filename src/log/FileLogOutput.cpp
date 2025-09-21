#include "log/FileLogOutput.h"
#include "UniConv.h"
#include <filesystem>
#include <iostream>

FileLogOutput::FileLogOutput(const std::wstring& outputName, bool autoFlush)
    : BaseLogOutput(outputName, L"File")
    , m_autoFlush(autoFlush)
    , m_bufferSize(0)
{
}

FileLogOutput::~FileLogOutput() {
    CloseFileStream();
}

void FileLogOutput::SetBufferSize(size_t bufferSize) {
    std::lock_guard<std::mutex> lock(m_fileMutex);
    m_bufferSize = bufferSize;
    
    if (m_fileStream.is_open()) {
        if (bufferSize == 0) {
            m_fileStream.rdbuf()->pubsetbuf(nullptr, 0);  // 无缓冲
        }
    }
}

std::wstring FileLogOutput::GetFilePath() const {
    return m_filePath;
}

LogOutputResult FileLogOutput::WriteLogInternal(const std::wstring& formattedLog, const LogCallbackInfo& originalInfo) {
    std::lock_guard<std::mutex> lock(m_fileMutex);
    
    try {
        if (!m_fileStream.is_open()) {
            return LogOutputResult::Failed;
        }
        
        // 写入日志消息
        m_fileStream << formattedLog << std::endl;
        
        // 自动刷新
        if (m_autoFlush) {
            m_fileStream.flush();
        }
        
        // 更新统计信息
        UpdateStats(LogOutputResult::Success, 0.0, formattedLog.length() * sizeof(wchar_t));
        
        return LogOutputResult::Success;
    }
    catch (const std::exception&) {
        UpdateStats(LogOutputResult::Failed, 0.0, 0);
        return LogOutputResult::Failed;
    }
}

void FileLogOutput::FlushInternal() {
    std::lock_guard<std::mutex> lock(m_fileMutex);
    
    if (m_fileStream.is_open()) {
        m_fileStream.flush();
    }
}

bool FileLogOutput::IsAvailableInternal() const {
    return m_fileStream.is_open() && m_fileStream.good();
}

bool FileLogOutput::InitializeInternal(const std::wstring& config) {
    try {
        m_filePath = config;
        
        // 创建目录（如果不存在）
        if (!CreateDirectoryIfNotExists(m_filePath)) {
            return false;
        }
        
        // 打开文件流
        return OpenFileStream();
    }
    catch (const std::exception&) {
        return false;
    }
}

void FileLogOutput::ShutdownInternal() {
    CloseFileStream();
}

std::wstring FileLogOutput::GetConfigStringInternal() const {
    return L"File:{path:" + m_filePath + L",autoFlush:" + (m_autoFlush ? L"true" : L"false") + L"}";
}

bool FileLogOutput::CreateDirectoryIfNotExists(const std::wstring& filePath) {
    try {
        std::filesystem::path path(filePath);
        std::filesystem::path dirPath = path.parent_path();
        
        if (!dirPath.empty() && !std::filesystem::exists(dirPath)) {
            return std::filesystem::create_directories(dirPath);
        }
        
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool FileLogOutput::OpenFileStream() {
    try {
        CloseFileStream();  // 确保先关闭已有的流
        
        // 打开文件流（追加模式）
        m_fileStream.open(m_filePath, std::ios::out | std::ios::app);
        
        if (!m_fileStream.is_open()) {
            return false;
        }
        
        // 设置缓冲区
        if (m_bufferSize == 0) {
            m_fileStream.rdbuf()->pubsetbuf(nullptr, 0);  // 无缓冲
        }
        
        // 设置UTF-8编码（如果需要）
        m_fileStream.imbue(std::locale(""));
        
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

void FileLogOutput::CloseFileStream() {
    std::lock_guard<std::mutex> lock(m_fileMutex);
    
    if (m_fileStream.is_open()) {
        m_fileStream.flush();
        m_fileStream.close();
    }
}