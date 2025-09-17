/*****************************************************************************
 *  RotationStrategies Implementation
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  @file     RotationStrategies.cpp
 *  @brief    轮转策略的额外实用函数
 *  @details  为头文件中的策略提供额外的工具函数和辅助方法
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *****************************************************************************/

#include "log/RotationStrategies.h"
#include <iostream>
#include <sstream>
#include <iomanip>

// ==================== 辅助函数实现 ====================

namespace RotationUtils {
    
    /**
     * @brief 格式化文件大小为可读字符串
     * @param sizeBytes 文件大小（字节）
     * @return 格式化的大小字符串
     */
    std::wstring FormatFileSize(size_t sizeBytes) {
        std::wostringstream oss;
        if (sizeBytes >= 1024 * 1024 * 1024) {
            oss << std::fixed << std::setprecision(2) 
                << (static_cast<double>(sizeBytes) / (1024.0 * 1024.0 * 1024.0)) << L" GB";
        } else if (sizeBytes >= 1024 * 1024) {
            oss << std::fixed << std::setprecision(2) 
                << (static_cast<double>(sizeBytes) / (1024.0 * 1024.0)) << L" MB";
        } else if (sizeBytes >= 1024) {
            oss << std::fixed << std::setprecision(2) 
                << (static_cast<double>(sizeBytes) / 1024.0) << L" KB";
        } else {
            oss << sizeBytes << L" bytes";
        }
        return oss.str();
    }
    
    /**
     * @brief 格式化时间间隔为可读字符串
     * @param duration 时间间隔
     * @return 格式化的时间字符串
     */
    std::wstring FormatDuration(std::chrono::milliseconds duration) {
        std::wostringstream oss;
        
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration - hours);
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration - hours - minutes);
        
        bool hasHours = hours.count() > 0;
        bool hasMinutes = minutes.count() > 0;
        bool hasSeconds = seconds.count() > 0;
        
        if (hasHours) {
            oss << hours.count() << L"h";
            if (hasMinutes || hasSeconds) oss << L" ";
        }
        if (hasMinutes) {
            oss << minutes.count() << L"m";
            if (hasSeconds) oss << L" ";
        }
        if (hasSeconds || (!hasHours && !hasMinutes)) {
            oss << seconds.count() << L"s";
        }
        
        return oss.str();
    }
    
    /**
     * @brief 创建标准的轮转策略组合（推荐配置）
     * @param maxSizeMB 最大文件大小（MB）
     * @param timeInterval 时间间隔类型
     * @return 配置好的复合策略
     */
    std::shared_ptr<CompositeRotationStrategy> CreateStandardStrategy(
        size_t maxSizeMB = 100,
        TimeBasedRotationStrategy::TimeInterval timeInterval = TimeBasedRotationStrategy::TimeInterval::Daily) {
        
        auto composite = std::make_shared<CompositeRotationStrategy>();
        composite->AddStrategy(RotationStrategyFactory::CreateSizeStrategy(maxSizeMB));
        composite->AddStrategy(RotationStrategyFactory::CreateTimeStrategy(timeInterval));
        return composite;
    }
    
    /**
     * @brief 验证轮转配置的合理性
     * @param strategy 要验证的策略
     * @param context 测试上下文
     * @return 验证结果和建议
     */
    std::wstring ValidateStrategy(const IRotationStrategy& strategy, const RotationContext& context) {
        std::wostringstream result;
        
        // 基本配置验证
        if (!strategy.ValidateConfiguration()) {
            result << L"ERROR: Invalid strategy configuration. ";
        } else {
            result << L"OK: Configuration is valid. ";
        }
        
        // 测试策略行为
        auto decision = strategy.ShouldRotate(context);
        result << L"Test rotation decision: " << (decision.shouldRotate ? L"ROTATE" : L"NO ROTATION");
        if (decision.shouldRotate) {
            result << L" (Priority: " << decision.priority << L", Reason: " << decision.reason << L")";
        }
        
        return result.str();
    }
}