#pragma once

#include "BaseLogOutput.h"
#include <fstream>
#include <string>
#include <mutex>

/**
 * @brief 文件日志输出实现
 * @details 将日志消息写入到指定的文件中，支持文件路径配置、自动创建目录等功能
 */
class FileLogOutput : public BaseLogOutput {
private:
    std::wstring m_filePath;           // 文件路径
    std::wofstream m_fileStream;       // 文件输出流
    std::mutex m_fileMutex;            // 文件写入互斥锁
    bool m_autoFlush;                  // 是否自动刷新
    size_t m_bufferSize;               // 缓冲区大小

public:
    /**
     * @brief 构造函数
     * @param outputName 输出名称
     * @param autoFlush 是否自动刷新，默认为true
     */
    explicit FileLogOutput(const std::wstring& outputName, bool autoFlush = true);
    
    /**
     * @brief 析构函数
     */
    virtual ~FileLogOutput();

    /**
     * @brief 设置缓冲区大小
     * @param bufferSize 缓冲区大小，0表示无缓冲
     */
    void SetBufferSize(size_t bufferSize);

    /**
     * @brief 获取当前文件路径
     * @return 文件路径
     */
    std::wstring GetFilePath() const;

protected:
    /**
     * @brief 内部写入日志实现
     * @param formattedLog 格式化后的日志消息
     * @param originalInfo 原始日志信息
     * @return 写入结果
     */
    LogOutputResult WriteLogInternal(const std::wstring& formattedLog, const LogCallbackInfo& originalInfo) override;

    /**
     * @brief 内部刷新实现
     */
    void FlushInternal() override;

    /**
     * @brief 检查输出是否可用
     * @return 是否可用
     */
    bool IsAvailableInternal() const override;

    /**
     * @brief 内部初始化实现
     * @param config 配置字符串（文件路径）
     * @return 是否初始化成功
     */
    bool InitializeInternal(const std::wstring& config) override;

    /**
     * @brief 内部关闭实现
     */
    void ShutdownInternal() override;

    /**
     * @brief 获取配置字符串
     * @return 配置字符串
     */
    std::wstring GetConfigStringInternal() const override;

private:
    /**
     * @brief 创建目录（如果不存在）
     * @param filePath 文件路径
     * @return 是否成功
     */
    bool CreateDirectoryIfNotExists(const std::wstring& filePath);

    /**
     * @brief 打开文件流
     * @return 是否成功
     */
    bool OpenFileStream();

    /**
     * @brief 关闭文件流
     */
    void CloseFileStream();
};