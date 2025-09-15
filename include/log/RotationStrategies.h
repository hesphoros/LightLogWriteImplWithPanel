#pragma once

#include "IRotationStrategy.h"
#include <cmath>
#include <vector>
#include <algorithm>

/*****************************************************************************
 *  Concrete Rotation Strategies
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     RotationStrategies.h
 *  @brief    具体轮转策略实现
 *  @details  实现各种轮转策略：大小策略、时间策略、组合策略等
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/15
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

/**
 * @brief 大小基础轮转策略
 * @details 基于文件大小进行轮转判断
 */
class SizeBasedRotationStrategy : public IRotationStrategy {
public:
    /**
     * @brief 构造函数
     * @param maxFileSizeBytes 最大文件大小(字节)
     */
    explicit SizeBasedRotationStrategy(size_t maxFileSizeBytes = 100 * 1024 * 1024)
        : maxFileSize_(maxFileSizeBytes) {}
    
    RotationDecision ShouldRotate(const RotationContext& context) const override {
        if (context.currentFileSize >= maxFileSize_) {
            double overageRatio = static_cast<double>(context.currentFileSize) / maxFileSize_;
            int priority = static_cast<int>(std::min(10.0, overageRatio * 5.0)); // 越超限优先级越高
            
            std::wstring reason = L"File size (" + 
                std::to_wstring(context.currentFileSize) + 
                L" bytes) exceeds limit (" + 
                std::to_wstring(maxFileSize_) + L" bytes)";
            
            return RotationDecision(true, reason, priority);
        }
        return RotationDecision(false, L"File size within limit");
    }
    
    std::wstring GetStrategyName() const override {
        return L"SizeBased";
    }
    
    std::wstring GetStrategyDescription() const override {
        return L"Rotates log files when they exceed " + 
               std::to_wstring(maxFileSize_ / (1024 * 1024)) + L"MB";
    }
    
    bool ValidateConfiguration() const override {
        return maxFileSize_ > 0 && maxFileSize_ <= SIZE_MAX;
    }
    
    bool GetNextRotationTime(const RotationContext& context,
                           std::chrono::system_clock::time_point& nextTime) const override {
        // 大小策略无法预测下次轮转时间
        return false;
    }
    
    /**
     * @brief 设置最大文件大小
     * @param sizeBytes 大小(字节)
     */
    void SetMaxFileSize(size_t sizeBytes) {
        maxFileSize_ = sizeBytes;
    }
    
    /**
     * @brief 获取最大文件大小
     * @return 大小(字节)
     */
    size_t GetMaxFileSize() const {
        return maxFileSize_;
    }

private:
    size_t maxFileSize_;    /*!< 最大文件大小(字节) */
};

/**
 * @brief 时间基础轮转策略
 * @details 基于时间间隔进行轮转判断
 */
class TimeBasedRotationStrategy : public IRotationStrategy {
public:
    /**
     * @brief 时间间隔类型
     */
    enum class TimeInterval {
        Hourly,     /*!< 每小时 */
        Daily,      /*!< 每天 */
        Weekly,     /*!< 每周 */
        Monthly     /*!< 每月 */
    };
    
    /**
     * @brief 构造函数
     * @param interval 时间间隔
     */
    explicit TimeBasedRotationStrategy(TimeInterval interval = TimeInterval::Daily)
        : interval_(interval) {}
    
    RotationDecision ShouldRotate(const RotationContext& context) const override {
        auto duration = context.currentTime - context.lastRotationTime;
        auto requiredDuration = GetIntervalDuration();
        
        if (duration >= requiredDuration) {
            auto overageDuration = duration - requiredDuration;
            int priority = static_cast<int>(
                std::min(10.0, std::chrono::duration_cast<std::chrono::hours>(overageDuration).count() * 0.5));
            
            std::wstring reason = L"Time interval (" + GetIntervalDescription() + L") reached";
            
            return RotationDecision(true, reason, priority);
        }
        return RotationDecision(false, L"Time interval not reached");
    }
    
    std::wstring GetStrategyName() const override {
        return L"TimeBased";
    }
    
    std::wstring GetStrategyDescription() const override {
        return L"Rotates log files every " + GetIntervalDescription();
    }
    
    bool ValidateConfiguration() const override {
        return true; // 所有时间间隔都是有效的
    }
    
    bool GetNextRotationTime(const RotationContext& context,
                           std::chrono::system_clock::time_point& nextTime) const override {
        nextTime = context.lastRotationTime + GetIntervalDuration();
        return true;
    }
    
    /**
     * @brief 设置时间间隔
     * @param interval 时间间隔
     */
    void SetTimeInterval(TimeInterval interval) {
        interval_ = interval;
    }
    
    /**
     * @brief 获取时间间隔
     * @return 时间间隔
     */
    TimeInterval GetTimeInterval() const {
        return interval_;
    }

private:
    TimeInterval interval_; /*!< 时间间隔 */
    
    /**
     * @brief 获取间隔持续时间
     * @return 持续时间
     */
    std::chrono::hours GetIntervalDuration() const {
        switch (interval_) {
            case TimeInterval::Hourly:  return std::chrono::hours(1);
            case TimeInterval::Daily:   return std::chrono::hours(24);
            case TimeInterval::Weekly:  return std::chrono::hours(24 * 7);
            case TimeInterval::Monthly: return std::chrono::hours(24 * 30); // 近似值
            default: return std::chrono::hours(24);
        }
    }
    
    /**
     * @brief 获取间隔描述
     * @return 描述字符串
     */
    std::wstring GetIntervalDescription() const {
        switch (interval_) {
            case TimeInterval::Hourly:  return L"hour";
            case TimeInterval::Daily:   return L"day";
            case TimeInterval::Weekly:  return L"week";
            case TimeInterval::Monthly: return L"month";
            default: return L"day";
        }
    }
};

/**
 * @brief 组合轮转策略
 * @details 组合多个策略，满足任一条件即轮转
 */
class CompositeRotationStrategy : public IRotationStrategy {
public:
    /**
     * @brief 构造函数
     */
    CompositeRotationStrategy() = default;
    
    /**
     * @brief 添加策略
     * @param strategy 要添加的策略
     */
    void AddStrategy(RotationStrategySharedPtr strategy) {
        if (strategy) {
            strategies_.push_back(strategy);
        }
    }
    
    /**
     * @brief 移除所有策略
     */
    void ClearStrategies() {
        strategies_.clear();
    }
    
    /**
     * @brief 获取策略数量
     * @return 策略数量
     */
    size_t GetStrategyCount() const {
        return strategies_.size();
    }
    
    RotationDecision ShouldRotate(const RotationContext& context) const override {
        if (strategies_.empty()) {
            return RotationDecision(false, L"No strategies configured");
        }
        
        RotationDecision bestDecision(false, L"No strategy triggered");
        int maxPriority = -1;
        
        for (const auto& strategy : strategies_) {
            auto decision = strategy->ShouldRotate(context);
            if (decision.shouldRotate && decision.priority > maxPriority) {
                bestDecision = decision;
                maxPriority = decision.priority;
                
                // 添加策略名称到原因中
                bestDecision.reason = strategy->GetStrategyName() + L": " + decision.reason;
            }
        }
        
        return bestDecision;
    }
    
    std::wstring GetStrategyName() const override {
        return L"Composite";
    }
    
    std::wstring GetStrategyDescription() const override {
        if (strategies_.empty()) {
            return L"Empty composite strategy";
        }
        
        std::wstring desc = L"Composite of: ";
        for (size_t i = 0; i < strategies_.size(); ++i) {
            if (i > 0) desc += L", ";
            desc += strategies_[i]->GetStrategyName();
        }
        return desc;
    }
    
    bool ValidateConfiguration() const override {
        if (strategies_.empty()) {
            return false;
        }
        
        for (const auto& strategy : strategies_) {
            if (!strategy || !strategy->ValidateConfiguration()) {
                return false;
            }
        }
        return true;
    }
    
    bool GetNextRotationTime(const RotationContext& context,
                           std::chrono::system_clock::time_point& nextTime) const override {
        bool hasValidTime = false;
        std::chrono::system_clock::time_point earliestTime = 
            std::chrono::system_clock::time_point::max();
        
        for (const auto& strategy : strategies_) {
            std::chrono::system_clock::time_point strategyTime;
            if (strategy->GetNextRotationTime(context, strategyTime)) {
                if (!hasValidTime || strategyTime < earliestTime) {
                    earliestTime = strategyTime;
                    hasValidTime = true;
                }
            }
        }
        
        if (hasValidTime) {
            nextTime = earliestTime;
        }
        return hasValidTime;
    }

private:
    std::vector<RotationStrategySharedPtr> strategies_;    /*!< 策略列表 */
};

/**
 * @brief 手动轮转策略
 * @details 仅响应手动触发的轮转
 */
class ManualRotationStrategy : public IRotationStrategy {
public:
    RotationDecision ShouldRotate(const RotationContext& context) const override {
        if (context.manualTrigger) {
            return RotationDecision(true, L"Manual rotation requested", 10);
        }
        return RotationDecision(false, L"No manual trigger");
    }
    
    std::wstring GetStrategyName() const override {
        return L"Manual";
    }
    
    std::wstring GetStrategyDescription() const override {
        return L"Rotates only when manually triggered";
    }
    
    bool ValidateConfiguration() const override {
        return true;
    }
    
    bool GetNextRotationTime(const RotationContext& context,
                           std::chrono::system_clock::time_point& nextTime) const override {
        return false; // 手动策略无法预测下次轮转时间
    }
};

/**
 * @brief 策略工厂类
 */
class RotationStrategyFactory {
public:
    /**
     * @brief 创建大小策略
     * @param maxSizeMB 最大大小(MB)
     * @return 策略实例
     */
    static RotationStrategySharedPtr CreateSizeStrategy(size_t maxSizeMB = 100) {
        return std::make_shared<SizeBasedRotationStrategy>(maxSizeMB * 1024 * 1024);
    }
    
    /**
     * @brief 创建时间策略
     * @param interval 时间间隔
     * @return 策略实例
     */
    static RotationStrategySharedPtr CreateTimeStrategy(
        TimeBasedRotationStrategy::TimeInterval interval = TimeBasedRotationStrategy::TimeInterval::Daily) {
        return std::make_shared<TimeBasedRotationStrategy>(interval);
    }
    
    /**
     * @brief 创建组合策略
     * @param maxSizeMB 最大大小(MB)
     * @param interval 时间间隔
     * @return 策略实例
     */
    static RotationStrategySharedPtr CreateCompositeStrategy(
        size_t maxSizeMB = 100,
        TimeBasedRotationStrategy::TimeInterval interval = TimeBasedRotationStrategy::TimeInterval::Daily) {
        
        auto composite = std::make_shared<CompositeRotationStrategy>();
        composite->AddStrategy(CreateSizeStrategy(maxSizeMB));
        composite->AddStrategy(CreateTimeStrategy(interval));
        return std::static_pointer_cast<IRotationStrategy>(composite);
    }
    
    /**
     * @brief 创建手动策略
     * @return 策略实例
     */
    static RotationStrategySharedPtr CreateManualStrategy() {
        return std::make_shared<ManualRotationStrategy>();
    }
};