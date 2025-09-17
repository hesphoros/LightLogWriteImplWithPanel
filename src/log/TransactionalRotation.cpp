#include "log/TransactionalRotation.h"
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

/*****************************************************************************
 *  TransactionalRotation
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     TransactionalRotation.cpp
 *  @brief    事务性轮转机制实现
 *  @details  提供原子性轮转操作和回滚机制，确保轮转操作的一致性
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/16
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

// RotationTransaction 实现

RotationTransaction::RotationTransaction(const std::wstring& transactionId)
    : transactionId_(transactionId.empty() ? GenerateTransactionId() : transactionId)
    , backupDirectory_(L"./logs/backup/tx_" + transactionId_)
{
}

RotationTransaction::~RotationTransaction()
{
    if (executed_ && !committed_) {
        // 如果事务已执行但未提交，尝试清理备份文件
        CleanupBackupFiles();
    }
}

std::wstring RotationTransaction::GetTransactionId() const
{
    return transactionId_;
}

bool RotationTransaction::AddFileMoveOperation(const std::wstring& sourceFile,
                                             const std::wstring& targetFile,
                                             bool createBackup)
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    
    if (executed_) {
        return false; // 事务已执行，不能添加新操作
    }

    if (!ValidateFilePath(sourceFile) || !ValidateFilePath(targetFile)) {
        return false;
    }

    RotationOperation operation(RotationOperationType::FileMove, L"Move file from " + sourceFile + L" to " + targetFile);
    operation.sourceFile = sourceFile;
    operation.targetFile = targetFile;
    
    if (createBackup) {
        operation.backupFile = GenerateBackupPath(sourceFile);
    }

    // 定义执行操作
    operation.operation = [this, sourceFile, targetFile]() -> bool {
        try {
            namespace fs = std::filesystem;
            
            // 确保目标目录存在
            fs::path targetPath(targetFile);
            if (targetPath.has_parent_path()) {
                fs::create_directories(targetPath.parent_path());
            }
            
            // 移动文件
            fs::rename(sourceFile, targetFile);
            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    };

    // 定义回滚操作
    operation.rollbackOperation = [this, sourceFile, targetFile, &operation]() -> bool {
        try {
            namespace fs = std::filesystem;
            
            if (fs::exists(targetFile)) {
                fs::rename(targetFile, sourceFile);
                return true;
            }
            
            // 如果目标文件不存在但有备份，从备份恢复
            if (!operation.backupFile.empty() && fs::exists(operation.backupFile)) {
                fs::copy_file(operation.backupFile, sourceFile);
                return true;
            }
            
            return false;
        }
        catch (const std::exception&) {
            return false;
        }
    };

    operations_.push_back(std::move(operation));
    return true;
}

bool RotationTransaction::AddFileRenameOperation(const std::wstring& oldName,
                                                const std::wstring& newName,
                                                bool createBackup)
{
    return AddFileMoveOperation(oldName, newName, createBackup);
}

bool RotationTransaction::AddFileDeleteOperation(const std::wstring& filePath,
                                               bool createBackup)
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    
    if (executed_) {
        return false;
    }

    if (!ValidateFilePath(filePath)) {
        return false;
    }

    RotationOperation operation(RotationOperationType::FileDelete, L"Delete file " + filePath);
    operation.sourceFile = filePath;
    
    if (createBackup) {
        operation.backupFile = GenerateBackupPath(filePath);
    }

    // 保存备份路径到局部变量
    std::wstring backupFile = operation.backupFile;
    
    // 定义执行操作
    operation.operation = [this, filePath, backupFile]() -> bool {
        try {
            namespace fs = std::filesystem;
            
            if (!fs::exists(filePath)) {
                return true; // 文件不存在，视为成功
            }
            
            // 创建备份（如果需要）
            if (!backupFile.empty()) {
                if (!CreateFileBackup(filePath, backupFile)) {
                    return false;
                }
            }
            
            fs::remove(filePath);
            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    };

    // 定义回滚操作
    operation.rollbackOperation = [this, filePath, backupFile]() -> bool {
        try {
            namespace fs = std::filesystem;
            
            // 从备份恢复文件
            if (!backupFile.empty() && fs::exists(backupFile)) {
                fs::copy_file(backupFile, filePath);
                return true;
            }
            
            return false;
        }
        catch (const std::exception&) {
            return false;
        }
    };

    operations_.push_back(std::move(operation));
    return true;
}

bool RotationTransaction::AddDirectoryCreateOperation(const std::wstring& dirPath)
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    
    if (executed_) {
        return false;
    }

    RotationOperation operation(RotationOperationType::DirectoryCreate, L"Create directory " + dirPath);
    operation.targetFile = dirPath;

    // 定义执行操作
    operation.operation = [dirPath]() -> bool {
        try {
            namespace fs = std::filesystem;
            return fs::create_directories(dirPath);
        }
        catch (const std::exception&) {
            return false;
        }
    };

    // 定义回滚操作（删除创建的目录）
    operation.rollbackOperation = [dirPath]() -> bool {
        try {
            namespace fs = std::filesystem;
            
            if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
                // 只删除空目录，避免意外删除有内容的目录
                if (fs::is_empty(dirPath)) {
                    fs::remove(dirPath);
                }
            }
            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    };

    operations_.push_back(std::move(operation));
    return true;
}

bool RotationTransaction::AddCompressionOperation(const std::wstring& sourceFile,
                                                 const std::wstring& targetFile)
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    
    if (executed_) {
        return false;
    }

    RotationOperation operation(RotationOperationType::Compression, L"Compress " + sourceFile + L" to " + targetFile);
    operation.sourceFile = sourceFile;
    operation.targetFile = targetFile;

    // 定义执行操作（这里简化实现，实际应该调用压缩库）
    operation.operation = [sourceFile, targetFile]() -> bool {
        try {
            namespace fs = std::filesystem;
            
            if (!fs::exists(sourceFile)) {
                return false;
            }
            
            // 这里应该调用实际的压缩功能
            // 简化实现：直接复制文件（实际项目中应该使用压缩库）
            fs::copy_file(sourceFile, targetFile);
            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    };

    // 定义回滚操作
    operation.rollbackOperation = [targetFile]() -> bool {
        try {
            namespace fs = std::filesystem;
            
            if (fs::exists(targetFile)) {
                fs::remove(targetFile);
            }
            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    };

    operations_.push_back(std::move(operation));
    return true;
}

bool RotationTransaction::AddCustomOperation(const std::wstring& description,
                                            std::function<bool()> operation,
                                            std::function<bool()> rollbackOperation)
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    
    if (executed_) {
        return false;
    }

    RotationOperation op(RotationOperationType::Custom, description);
    op.operation = operation;
    op.rollbackOperation = rollbackOperation;

    operations_.push_back(std::move(op));
    return true;
}

bool RotationTransaction::Execute()
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    
    if (executed_) {
        return true; // 已执行
    }

    startTime_ = std::chrono::system_clock::now();
    
    // 创建备份目录
    if (!CreateBackupDirectory()) {
        return false;
    }

    bool allSuccessful = true;
    size_t executedCount = 0;

    // 按顺序执行所有操作
    for (auto& operation : operations_) {
        if (!ExecuteOperation(operation)) {
            allSuccessful = false;
            break;
        }
        executedCount++;
        
        // 检查超时
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - startTime_);
        if (elapsed > timeout_) {
            allSuccessful = false;
            break;
        }
    }

    endTime_ = std::chrono::system_clock::now();
    executed_ = true;

    if (!allSuccessful) {
        // 如果有操作失败，自动回滚已执行的操作
        Rollback();
        return false;
    }

    return true;
}

bool RotationTransaction::Rollback()
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    
    if (!executed_) {
        return true; // 未执行，无需回滚
    }

    bool allSuccessful = true;

    // 按逆序回滚所有已执行的操作
    for (auto it = operations_.rbegin(); it != operations_.rend(); ++it) {
        if (it->executed && it->success) {
            if (!RollbackOperation(*it)) {
                allSuccessful = false;
            }
        }
    }

    return allSuccessful;
}

bool RotationTransaction::Commit()
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    
    if (!executed_) {
        return false;
    }

    if (committed_) {
        return true; // 已提交
    }

    // 清理备份文件
    bool cleanupSuccess = CleanupBackupFiles();
    committed_ = true;
    
    return cleanupSuccess;
}

size_t RotationTransaction::GetOperationCount() const
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    return operations_.size();
}

bool RotationTransaction::IsExecuted() const
{
    return executed_;
}

size_t RotationTransaction::GetSuccessfulOperationCount() const
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    
    return std::count_if(operations_.begin(), operations_.end(),
        [](const RotationOperation& op) { return op.success; });
}

size_t RotationTransaction::GetFailedOperationCount() const
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    
    return std::count_if(operations_.begin(), operations_.end(),
        [](const RotationOperation& op) { return op.executed && !op.success; });
}

std::vector<RotationOperation> RotationTransaction::GetOperationHistory() const
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    return operations_;
}

std::vector<std::wstring> RotationTransaction::GetErrorMessages() const
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    
    std::vector<std::wstring> errors;
    for (const auto& operation : operations_) {
        if (operation.executed && !operation.success && !operation.errorMessage.empty()) {
            errors.push_back(operation.errorMessage);
        }
    }
    
    return errors;
}

std::chrono::milliseconds RotationTransaction::GetExecutionTime() const
{
    if (!executed_) {
        return std::chrono::milliseconds(0);
    }
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime_ - startTime_);
}

void RotationTransaction::Clear()
{
    std::lock_guard<std::mutex> lock(operationsMutex_);
    
    if (executed_) {
        return; // 已执行的事务不能清空
    }
    
    operations_.clear();
}

void RotationTransaction::SetTimeout(std::chrono::milliseconds timeout)
{
    timeout_ = timeout;
}

// 私有方法实现

std::wstring RotationTransaction::GenerateTransactionId()
{
    static std::atomic<uint64_t> counter{0};
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    uint64_t id = counter.fetch_add(1);
    
    std::wostringstream oss;
    oss << L"TX_" << timestamp << L"_" << id;
    return oss.str();
}

bool RotationTransaction::CreateBackupDirectory()
{
    try {
        namespace fs = std::filesystem;
        return fs::create_directories(backupDirectory_);
    }
    catch (const std::exception&) {
        return false;
    }
}

std::wstring RotationTransaction::GenerateBackupPath(const std::wstring& originalFile) const
{
    namespace fs = std::filesystem;
    
    fs::path originalPath(originalFile);
    std::wstring fileName = originalPath.filename().wstring();
    
    fs::path backupPath(backupDirectory_);
    backupPath /= fileName;
    
    return backupPath.wstring();
}

bool RotationTransaction::CreateFileBackup(const std::wstring& filePath, const std::wstring& backupPath)
{
    try {
        namespace fs = std::filesystem;
        
        if (!fs::exists(filePath)) {
            return true; // 原文件不存在，无需备份
        }
        
        // 确保备份目录存在
        fs::path backupDir = fs::path(backupPath).parent_path();
        fs::create_directories(backupDir);
        
        // 复制文件到备份位置
        fs::copy_file(filePath, backupPath, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool RotationTransaction::ExecuteOperation(RotationOperation& operation)
{
    try {
        operation.executed = true;
        
        // 如果需要备份，先创建备份
        if (!operation.backupFile.empty() && !operation.sourceFile.empty()) {
            if (!CreateFileBackup(operation.sourceFile, operation.backupFile)) {
                operation.success = false;
                operation.errorMessage = L"Failed to create backup";
                return false;
            }
        }
        
        // 执行操作
        if (operation.operation && operation.operation()) {
            operation.success = true;
            return true;
        } else {
            operation.success = false;
            operation.errorMessage = L"Operation execution failed";
            return false;
        }
    }
    catch (const std::exception& ex) {
        operation.success = false;
        operation.errorMessage = std::wstring(ex.what(), ex.what() + strlen(ex.what()));
        return false;
    }
}

bool RotationTransaction::RollbackOperation(const RotationOperation& operation)
{
    try {
        if (operation.rollbackOperation) {
            return operation.rollbackOperation();
        }
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool RotationTransaction::CleanupBackupFiles()
{
    try {
        namespace fs = std::filesystem;
        
        if (fs::exists(backupDirectory_)) {
            fs::remove_all(backupDirectory_);
        }
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool RotationTransaction::ValidateFilePath(const std::wstring& filePath) const
{
    if (filePath.empty()) {
        return false;
    }
    
    // 检查路径中是否包含非法字符
    const std::wstring invalidChars = L"<>:\"|?*";
    for (wchar_t ch : invalidChars) {
        if (filePath.find(ch) != std::wstring::npos) {
            return false;
        }
    }
    
    return true;
}

// TransactionalRotationManager 实现

TransactionalRotationManager::TransactionalRotationManager() = default;

TransactionalRotationManager::~TransactionalRotationManager()
{
    // 清理所有未完成的事务
    std::lock_guard<std::mutex> lock(transactionsMutex_);
    transactions_.clear();
}

std::shared_ptr<RotationTransaction> TransactionalRotationManager::CreateTransaction(const std::wstring& transactionId)
{
    std::lock_guard<std::mutex> lock(transactionsMutex_);
    
    if (!CheckTransactionLimit()) {
        return nullptr;
    }
    
    std::wstring id = transactionId.empty() ? RotationTransaction::GenerateTransactionId() : transactionId;
    
    // 检查ID是否已存在
    if (transactions_.find(id) != transactions_.end()) {
        return nullptr;
    }
    
    auto transaction = std::make_shared<RotationTransaction>(id);
    transactions_[id] = transaction;
    totalTransactions_.fetch_add(1);
    
    return transaction;
}

std::shared_ptr<RotationTransaction> TransactionalRotationManager::GetTransaction(const std::wstring& transactionId)
{
    std::lock_guard<std::mutex> lock(transactionsMutex_);
    
    auto it = transactions_.find(transactionId);
    if (it != transactions_.end()) {
        return it->second;
    }
    return nullptr;
}

bool TransactionalRotationManager::ExecuteTransaction(const std::wstring& transactionId)
{
    auto transaction = GetTransaction(transactionId);
    if (!transaction) {
        return false;
    }
    
    bool success = transaction->Execute();
    
    if (success) {
        successfulTransactions_.fetch_add(1);
    } else {
        failedTransactions_.fetch_add(1);
    }
    
    return success;
}

bool TransactionalRotationManager::RollbackTransaction(const std::wstring& transactionId)
{
    auto transaction = GetTransaction(transactionId);
    if (!transaction) {
        return false;
    }
    
    return transaction->Rollback();
}

bool TransactionalRotationManager::CommitTransaction(const std::wstring& transactionId)
{
    auto transaction = GetTransaction(transactionId);
    if (!transaction) {
        return false;
    }
    
    return transaction->Commit();
}

bool TransactionalRotationManager::RemoveTransaction(const std::wstring& transactionId)
{
    std::lock_guard<std::mutex> lock(transactionsMutex_);
    
    auto it = transactions_.find(transactionId);
    if (it != transactions_.end()) {
        transactions_.erase(it);
        return true;
    }
    return false;
}

size_t TransactionalRotationManager::GetActiveTransactionCount() const
{
    std::lock_guard<std::mutex> lock(transactionsMutex_);
    return transactions_.size();
}

std::vector<std::wstring> TransactionalRotationManager::GetAllTransactionIds() const
{
    std::lock_guard<std::mutex> lock(transactionsMutex_);
    
    std::vector<std::wstring> ids;
    ids.reserve(transactions_.size());
    
    for (const auto& pair : transactions_) {
        ids.push_back(pair.first);
    }
    
    return ids;
}

size_t TransactionalRotationManager::CleanupCompletedTransactions()
{
    std::lock_guard<std::mutex> lock(transactionsMutex_);
    
    size_t cleanupCount = 0;
    auto it = transactions_.begin();
    
    while (it != transactions_.end()) {
        auto& transaction = it->second;
        
        // 清理已执行且已提交的事务
        if (transaction->IsExecuted()) {
            it = transactions_.erase(it);
            cleanupCount++;
        } else {
            ++it;
        }
    }
    
    return cleanupCount;
}

void TransactionalRotationManager::SetMaxTransactions(size_t maxTransactions)
{
    maxTransactions_ = maxTransactions;
}

std::wstring TransactionalRotationManager::GetTransactionStatistics() const
{
    std::wostringstream stats;
    
    stats << L"Transaction Manager Statistics:\n";
    stats << L"  Active Transactions: " << GetActiveTransactionCount() << L"\n";
    stats << L"  Total Transactions: " << totalTransactions_.load() << L"\n";
    stats << L"  Successful Transactions: " << successfulTransactions_.load() << L"\n";
    stats << L"  Failed Transactions: " << failedTransactions_.load() << L"\n";
    stats << L"  Max Transactions: " << maxTransactions_ << L"\n";
    
    if (totalTransactions_.load() > 0) {
        double successRate = static_cast<double>(successfulTransactions_.load()) / totalTransactions_.load() * 100.0;
        stats << L"  Success Rate: " << std::fixed << std::setprecision(2) << successRate << L"%\n";
    }
    
    return stats.str();
}

bool TransactionalRotationManager::CheckTransactionLimit() const
{
    return transactions_.size() < maxTransactions_;
}

// TransactionFactory 实现

std::shared_ptr<RotationTransaction> TransactionFactory::CreateFileRotationTransaction(
    const std::wstring& sourceFile,
    const std::wstring& archiveFile,
    bool compressionEnabled)
{
    auto transaction = std::make_shared<RotationTransaction>();
    
    // 添加文件移动操作
    transaction->AddFileMoveOperation(sourceFile, archiveFile, true);
    
    // 如果启用压缩，添加压缩操作
    if (compressionEnabled) {
        std::wstring compressedFile = archiveFile + L".zip";
        transaction->AddCompressionOperation(archiveFile, compressedFile);
        
        // 压缩后删除原归档文件
        transaction->AddFileDeleteOperation(archiveFile, false);
    }
    
    return transaction;
}

std::shared_ptr<RotationTransaction> TransactionFactory::CreateCleanupTransaction(
    const std::vector<std::wstring>& filesToDelete,
    bool createBackup)
{
    auto transaction = std::make_shared<RotationTransaction>();
    
    for (const auto& filePath : filesToDelete) {
        transaction->AddFileDeleteOperation(filePath, createBackup);
    }
    
    return transaction;
}