#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <mutex>
#include <map>
#include <filesystem>

/*****************************************************************************
 *  TransactionalRotation
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     TransactionalRotation.h
 *  @brief    事务性轮转机制
 *  @details  提供原子性轮转操作和回滚机制，确保轮转操作的一致性
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/15
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

/**
 * @brief 轮转操作类型
 */
enum class RotationOperationType {
    FileMove,           /*!< 文件移动 */
    FileRename,         /*!< 文件重命名 */
    FileDelete,         /*!< 文件删除 */
    DirectoryCreate,    /*!< 目录创建 */
    Compression,        /*!< 压缩操作 */
    Custom              /*!< 自定义操作 */
};

/**
 * @brief 轮转操作记录
 */
struct RotationOperation {
    RotationOperationType type;                         /*!< 操作类型 */
    std::wstring sourceFile;                            /*!< 源文件路径 */
    std::wstring targetFile;                            /*!< 目标文件路径 */
    std::wstring backupFile;                            /*!< 备份文件路径 */
    std::function<bool()> operation;                    /*!< 执行操作的函数 */
    std::function<bool()> rollbackOperation;           /*!< 回滚操作的函数 */
    std::wstring description;                           /*!< 操作描述 */
    std::chrono::system_clock::time_point timestamp;   /*!< 操作时间戳 */
    bool executed = false;                              /*!< 是否已执行 */
    bool success = false;                               /*!< 执行是否成功 */
    std::wstring errorMessage;                          /*!< 错误消息 */
    
    RotationOperation() : timestamp(std::chrono::system_clock::now()) {}
    
    RotationOperation(RotationOperationType t, const std::wstring& desc)
        : type(t), description(desc), timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief 轮转事务
 */
class RotationTransaction {
public:
    /**
     * @brief 构造函数
     * @param transactionId 事务ID
     */
    explicit RotationTransaction(const std::wstring& transactionId = L"");
    
    /**
     * @brief 析构函数
     */
    ~RotationTransaction();
    
    /**
     * @brief 获取事务ID
     * @return 事务ID
     */
    std::wstring GetTransactionId() const;
    
    /**
     * @brief 添加文件移动操作
     * @param sourceFile 源文件
     * @param targetFile 目标文件
     * @param createBackup 是否创建备份
     * @return 操作是否添加成功
     */
    bool AddFileMoveOperation(const std::wstring& sourceFile, 
                             const std::wstring& targetFile,
                             bool createBackup = true);
    
    /**
     * @brief 添加文件重命名操作
     * @param oldName 旧文件名
     * @param newName 新文件名
     * @param createBackup 是否创建备份
     * @return 操作是否添加成功
     */
    bool AddFileRenameOperation(const std::wstring& oldName,
                               const std::wstring& newName,
                               bool createBackup = true);
    
    /**
     * @brief 添加文件删除操作
     * @param filePath 文件路径
     * @param createBackup 是否创建备份
     * @return 操作是否添加成功
     */
    bool AddFileDeleteOperation(const std::wstring& filePath,
                               bool createBackup = true);
    
    /**
     * @brief 添加目录创建操作
     * @param dirPath 目录路径
     * @return 操作是否添加成功
     */
    bool AddDirectoryCreateOperation(const std::wstring& dirPath);
    
    /**
     * @brief 添加压缩操作
     * @param sourceFile 源文件
     * @param targetFile 目标压缩文件
     * @return 操作是否添加成功
     */
    bool AddCompressionOperation(const std::wstring& sourceFile,
                                const std::wstring& targetFile);
    
    /**
     * @brief 添加自定义操作
     * @param description 操作描述
     * @param operation 执行函数
     * @param rollbackOperation 回滚函数
     * @return 操作是否添加成功
     */
    bool AddCustomOperation(const std::wstring& description,
                           std::function<bool()> operation,
                           std::function<bool()> rollbackOperation);
    
    /**
     * @brief 执行事务
     * @return 事务是否成功
     */
    bool Execute();
    
    /**
     * @brief 回滚事务
     * @return 回滚是否成功
     */
    bool Rollback();
    
    /**
     * @brief 提交事务（删除备份文件）
     * @return 提交是否成功
     */
    bool Commit();
    
    /**
     * @brief 获取操作数量
     * @return 操作数量
     */
    size_t GetOperationCount() const;
    
    /**
     * @brief 获取执行结果
     * @return 执行是否成功
     */
    bool IsExecuted() const;
    
    /**
     * @brief 获取执行成功的操作数量
     * @return 成功操作数量
     */
    size_t GetSuccessfulOperationCount() const;
    
    /**
     * @brief 获取失败的操作数量
     * @return 失败操作数量
     */
    size_t GetFailedOperationCount() const;
    
    /**
     * @brief 获取操作历史
     * @return 操作历史列表
     */
    std::vector<RotationOperation> GetOperationHistory() const;
    
    /**
     * @brief 获取错误消息
     * @return 错误消息列表
     */
    std::vector<std::wstring> GetErrorMessages() const;
    
    /**
     * @brief 获取事务执行时间
     * @return 执行时间（毫秒）
     */
    std::chrono::milliseconds GetExecutionTime() const;
    
    /**
     * @brief 清空所有操作
     */
    void Clear();
    
    /**
     * @brief 设置事务超时时间
     * @param timeout 超时时间
     */
    void SetTimeout(std::chrono::milliseconds timeout);

private:
    std::wstring transactionId_;                        /*!< 事务ID */
    std::vector<RotationOperation> operations_;         /*!< 操作列表 */
    std::wstring backupDirectory_;                      /*!< 备份目录 */
    bool executed_ = false;                             /*!< 是否已执行 */
    bool committed_ = false;                            /*!< 是否已提交 */
    std::chrono::milliseconds timeout_{30000};         /*!< 超时时间 */
    
    mutable std::mutex operationsMutex_;               /*!< 操作列表互斥锁 */
    
    std::chrono::system_clock::time_point startTime_;  /*!< 开始时间 */
    std::chrono::system_clock::time_point endTime_;    /*!< 结束时间 */
    
    /**
     * @brief 生成唯一的事务ID
     * @return 事务ID
     */
    static std::wstring GenerateTransactionId();
    
    /**
     * @brief 创建备份目录
     * @return 创建是否成功
     */
    bool CreateBackupDirectory();
    
    /**
     * @brief 生成备份文件路径
     * @param originalFile 原文件路径
     * @return 备份文件路径
     */
    std::wstring GenerateBackupPath(const std::wstring& originalFile) const;
    
    /**
     * @brief 创建文件备份
     * @param filePath 文件路径
     * @param backupPath 备份路径
     * @return 备份是否成功
     */
    bool CreateFileBackup(const std::wstring& filePath, const std::wstring& backupPath);
    
    /**
     * @brief 执行单个操作
     * @param operation 要执行的操作
     * @return 执行是否成功
     */
    bool ExecuteOperation(RotationOperation& operation);
    
    /**
     * @brief 回滚单个操作
     * @param operation 要回滚的操作
     * @return 回滚是否成功
     */
    bool RollbackOperation(const RotationOperation& operation);
    
    /**
     * @brief 清理备份文件
     * @return 清理是否成功
     */
    bool CleanupBackupFiles();
    
    /**
     * @brief 验证文件路径
     * @param filePath 文件路径
     * @return 路径是否有效
     */
    bool ValidateFilePath(const std::wstring& filePath) const;
};

/**
 * @brief 事务性轮转管理器
 * @details 管理多个轮转事务，提供事务池功能
 */
class TransactionalRotationManager {
public:
    /**
     * @brief 构造函数
     */
    TransactionalRotationManager();
    
    /**
     * @brief 析构函数
     */
    ~TransactionalRotationManager();
    
    /**
     * @brief 创建新事务
     * @param transactionId 事务ID（可选）
     * @return 事务智能指针
     */
    std::shared_ptr<RotationTransaction> CreateTransaction(const std::wstring& transactionId = L"");
    
    /**
     * @brief 获取事务
     * @param transactionId 事务ID
     * @return 事务智能指针，如果不存在返回nullptr
     */
    std::shared_ptr<RotationTransaction> GetTransaction(const std::wstring& transactionId);
    
    /**
     * @brief 执行事务
     * @param transactionId 事务ID
     * @return 执行是否成功
     */
    bool ExecuteTransaction(const std::wstring& transactionId);
    
    /**
     * @brief 回滚事务
     * @param transactionId 事务ID
     * @return 回滚是否成功
     */
    bool RollbackTransaction(const std::wstring& transactionId);
    
    /**
     * @brief 提交事务
     * @param transactionId 事务ID
     * @return 提交是否成功
     */
    bool CommitTransaction(const std::wstring& transactionId);
    
    /**
     * @brief 删除事务
     * @param transactionId 事务ID
     * @return 删除是否成功
     */
    bool RemoveTransaction(const std::wstring& transactionId);
    
    /**
     * @brief 获取活跃事务数量
     * @return 活跃事务数量
     */
    size_t GetActiveTransactionCount() const;
    
    /**
     * @brief 获取所有事务ID
     * @return 事务ID列表
     */
    std::vector<std::wstring> GetAllTransactionIds() const;
    
    /**
     * @brief 清理已完成的事务
     * @return 清理的事务数量
     */
    size_t CleanupCompletedTransactions();
    
    /**
     * @brief 设置最大事务数量
     * @param maxTransactions 最大事务数量
     */
    void SetMaxTransactions(size_t maxTransactions);
    
    /**
     * @brief 获取事务统计信息
     * @return 统计信息字符串
     */
    std::wstring GetTransactionStatistics() const;

private:
    mutable std::mutex transactionsMutex_;             /*!< 事务映射互斥锁 */
    std::map<std::wstring, std::shared_ptr<RotationTransaction>> transactions_; /*!< 事务映射 */
    size_t maxTransactions_ = 100;                     /*!< 最大事务数量 */
    
    // 统计信息
    std::atomic<size_t> totalTransactions_{0};        /*!< 总事务数量 */
    std::atomic<size_t> successfulTransactions_{0};   /*!< 成功事务数量 */
    std::atomic<size_t> failedTransactions_{0};       /*!< 失败事务数量 */
    
    /**
     * @brief 检查事务数量限制
     * @return 是否可以创建新事务
     */
    bool CheckTransactionLimit() const;
};

/**
 * @brief 事务工厂类
 */
class TransactionFactory {
public:
    /**
     * @brief 创建文件轮转事务
     * @param sourceFile 源文件
     * @param archiveFile 归档文件
     * @param compressionEnabled 是否启用压缩
     * @return 事务智能指针
     */
    static std::shared_ptr<RotationTransaction> CreateFileRotationTransaction(
        const std::wstring& sourceFile,
        const std::wstring& archiveFile,
        bool compressionEnabled = false);
    
    /**
     * @brief 创建批量文件清理事务
     * @param filesToDelete 要删除的文件列表
     * @param createBackup 是否创建备份
     * @return 事务智能指针
     */
    static std::shared_ptr<RotationTransaction> CreateCleanupTransaction(
        const std::vector<std::wstring>& filesToDelete,
        bool createBackup = true);
};