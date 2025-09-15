#pragma once

#include <string>
#include <chrono>
#include <memory>

/*****************************************************************************
 *  IRotationStrategy Interface
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     IRotationStrategy.h
 *  @brief    日志轮转策略抽象接口
 *  @details  定义轮转策略的统一接口，支持策略模式扩展
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/15
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

/**
 * @brief 轮转上下文信息
 */
struct RotationContext {
    std::wstring currentFileName;                                   /*!< 当前文件名 */
    size_t currentFileSize = 0;                                     /*!< 当前文件大小(字节) */
    std::chrono::system_clock::time_point lastRotationTime;        /*!< 上次轮转时间 */
    std::chrono::system_clock::time_point currentTime;             /*!< 当前时间 */
    std::chrono::system_clock::time_point fileCreationTime;        /*!< 文件创建时间 */
    bool manualTrigger = false;                                     /*!< 是否手动触发 */
    
    RotationContext() : 
        lastRotationTime(std::chrono::system_clock::now()),
        currentTime(std::chrono::system_clock::now()),
        fileCreationTime(std::chrono::system_clock::now()) {}
};

/**
 * @brief 轮转决策结果
 */
struct RotationDecision {
    bool shouldRotate = false;                                      /*!< 是否应该轮转 */
    std::wstring reason;                                            /*!< 轮转原因 */
    int priority = 0;                                               /*!< 轮转优先级(越高越紧急) */
    std::chrono::milliseconds estimatedDuration{0};                /*!< 预估轮转耗时 */
    
    RotationDecision() = default;
    RotationDecision(bool rotate, const std::wstring& r, int p = 0)
        : shouldRotate(rotate), reason(r), priority(p) {}
};

/**
 * @brief 轮转策略抽象接口
 * @details 定义轮转策略的统一接口，实现策略模式
 */
class IRotationStrategy {
public:
    virtual ~IRotationStrategy() = default;
    
    /**
     * @brief 判断是否需要轮转
     * @param context 轮转上下文
     * @return 轮转决策结果
     */
    virtual RotationDecision ShouldRotate(const RotationContext& context) const = 0;
    
    /**
     * @brief 获取策略名称
     * @return 策略名称
     */
    virtual std::wstring GetStrategyName() const = 0;
    
    /**
     * @brief 获取策略描述
     * @return 策略描述
     */
    virtual std::wstring GetStrategyDescription() const = 0;
    
    /**
     * @brief 验证策略配置
     * @return 配置是否有效
     */
    virtual bool ValidateConfiguration() const = 0;
    
    /**
     * @brief 获取下次预计轮转时间
     * @param context 轮转上下文
     * @param nextTime 输出参数，下次轮转时间
     * @return 是否有预计时间
     */
    virtual bool GetNextRotationTime(const RotationContext& context,
                                    std::chrono::system_clock::time_point& nextTime) const = 0;
};

/**
 * @brief 智能指针类型定义
 */
using RotationStrategyPtr = std::unique_ptr<IRotationStrategy>;
using RotationStrategySharedPtr = std::shared_ptr<IRotationStrategy>;