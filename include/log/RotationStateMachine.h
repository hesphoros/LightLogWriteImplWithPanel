#pragma once

#include <string>
#include <chrono>
#include <functional>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>

/*****************************************************************************
 *  RotationStateMachine
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     RotationStateMachine.h
 *  @brief    轮转状态机
 *  @details  管理轮转过程的状态转换，确保轮转操作的原子性和可控性
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/15
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

/**
 * @brief 轮转状态枚举
 */
enum class RotationState {
    Idle,               /*!< 空闲状态 - 等待轮转触发 */
    Checking,           /*!< 检查状态 - 检查轮转条件 */
    Preparing,          /*!< 准备状态 - 准备轮转操作 */
    PreCheck,           /*!< 预检查状态 - 验证轮转前置条件 */
    Rotating,           /*!< 轮转状态 - 执行文件轮转 */
    Compressing,        /*!< 压缩状态 - 压缩旧文件 */
    Cleaning,           /*!< 清理状态 - 清理旧归档文件 */
    Completing,         /*!< 完成状态 - 轮转后处理 */
    Completed,          /*!< 已完成 - 轮转成功完成 */
    Failed,             /*!< 失败状态 - 轮转失败 */
    Recovering,         /*!< 恢复状态 - 从失败中恢复 */
    Rollback            /*!< 回滚状态 - 撤销操作 */
};

/**
 * @brief 轮转事件枚举
 */
enum class RotationEvent {
    Start,              /*!< 开始轮转 */
    CheckPassed,        /*!< 检查通过 */
    CheckFailed,        /*!< 检查失败 */
    PrepareDone,        /*!< 准备完成 */
    PreCheckPassed,     /*!< 预检查通过 */
    PreCheckFailed,     /*!< 预检查失败 */
    RotationSuccess,    /*!< 轮转成功 */
    RotationFailed,     /*!< 轮转失败 */
    CompressionSuccess, /*!< 压缩成功 */
    CompressionFailed,  /*!< 压缩失败 */
    CleanupDone,        /*!< 清理完成 */
    Complete,           /*!< 完成事件 */
    Fail,               /*!< 失败事件 */
    Recover,            /*!< 恢复事件 */
    Rollback,           /*!< 回滚事件 */
    Reset               /*!< 重置事件 */
};

/**
 * @brief 状态转换结果
 */
struct StateTransitionResult {
    bool success = false;                               /*!< 转换是否成功 */
    RotationState fromState = RotationState::Idle;     /*!< 源状态 */
    RotationState toState = RotationState::Idle;       /*!< 目标状态 */
    RotationEvent event = RotationEvent::Start;        /*!< 触发事件 */
    std::wstring message;                               /*!< 转换消息 */
    std::chrono::system_clock::time_point timestamp;   /*!< 转换时间 */
    
    StateTransitionResult() : timestamp(std::chrono::system_clock::now()) {}
    
    StateTransitionResult(bool succ, RotationState from, RotationState to, 
                         RotationEvent evt, const std::wstring& msg)
        : success(succ), fromState(from), toState(to), event(evt), message(msg)
        , timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief 状态回调函数类型
 */
using StateCallback = std::function<void(const StateTransitionResult&)>;

/**
 * @brief 轮转状态机上下文
 */
struct RotationStateMachineContext {
    std::wstring fileName;                              /*!< 轮转文件名 */
    std::wstring transactionId;                         /*!< 事务ID */
    std::chrono::system_clock::time_point startTime;   /*!< 开始时间 */
    std::map<std::wstring, std::wstring> metadata;     /*!< 元数据 */
    
    RotationStateMachineContext() : startTime(std::chrono::system_clock::now()) {}
};

/**
 * @brief 轮转状态机
 * @details 管理轮转过程的状态转换，确保操作的原子性和可追踪性
 */
class RotationStateMachine {
public:
    /**
     * @brief 构造函数
     */
    RotationStateMachine();
    
    /**
     * @brief 析构函数
     */
    ~RotationStateMachine();
    
    /**
     * @brief 获取当前状态
     * @return 当前状态
     */
    RotationState GetCurrentState() const;
    
    /**
     * @brief 触发状态转换
     * @param event 触发事件
     * @param context 状态机上下文
     * @return 转换结果
     */
    StateTransitionResult TriggerEvent(RotationEvent event, 
                                      const RotationStateMachineContext& context = {});
    
    /**
     * @brief 检查是否可以触发事件
     * @param event 要检查的事件
     * @return 是否可以触发
     */
    bool CanTriggerEvent(RotationEvent event) const;
    
    /**
     * @brief 重置状态机到空闲状态
     */
    void Reset();
    
    /**
     * @brief 设置状态回调
     * @param callback 状态变化时的回调函数
     */
    void SetStateCallback(StateCallback callback);
    
    /**
     * @brief 获取状态历史
     * @return 状态转换历史
     */
    std::vector<StateTransitionResult> GetStateHistory() const;
    
    /**
     * @brief 清空状态历史
     */
    void ClearStateHistory();
    
    /**
     * @brief 获取在当前状态的持续时间
     * @return 持续时间
     */
    std::chrono::milliseconds GetCurrentStateDuration() const;
    
    /**
     * @brief 获取总处理时间
     * @return 从第一次状态转换到现在的总时间
     */
    std::chrono::milliseconds GetTotalProcessingTime() const;
    
    /**
     * @brief 检查状态机是否在最终状态
     * @return 是否在最终状态(Completed/Failed)
     */
    bool IsInFinalState() const;
    
    /**
     * @brief 检查状态机是否在错误状态
     * @return 是否在错误状态
     */
    bool IsInErrorState() const;
    
    /**
     * @brief 获取状态名称
     * @param state 状态
     * @return 状态名称
     */
    static std::wstring GetStateName(RotationState state);
    
    /**
     * @brief 获取事件名称
     * @param event 事件
     * @return 事件名称
     */
    static std::wstring GetEventName(RotationEvent event);
    
    /**
     * @brief 获取状态机的图形化表示（DOT格式）
     * @return DOT格式的状态机图
     */
    std::string GetStateMachineDiagram() const;
    
    /**
     * @brief 验证状态机配置
     * @return 配置是否有效
     */
    bool ValidateStateMachine() const;

private:
    // 状态管理
    std::atomic<RotationState> currentState_;
    mutable std::mutex stateMutex_;
    
    // 状态转换表
    std::map<std::pair<RotationState, RotationEvent>, RotationState> transitionTable_;
    
    // 回调管理
    mutable std::mutex callbackMutex_;
    StateCallback stateCallback_;
    
    // 历史记录
    mutable std::mutex historyMutex_;
    std::vector<StateTransitionResult> stateHistory_;
    static const size_t MAX_HISTORY_SIZE = 1000;  /*!< 最大历史记录数量 */
    
    // 时间跟踪
    std::chrono::system_clock::time_point currentStateStartTime_;
    std::chrono::system_clock::time_point processingStartTime_;
    
    /**
     * @brief 初始化状态转换表
     */
    void InitializeTransitionTable();
    
    /**
     * @brief 执行状态转换
     * @param newState 新状态
     * @param event 触发事件
     * @param context 上下文
     * @return 转换结果
     */
    StateTransitionResult TransitionToState(RotationState newState, 
                                           RotationEvent event,
                                           const RotationStateMachineContext& context);
    
    /**
     * @brief 触发状态回调
     * @param result 状态转换结果
     */
    void TriggerStateCallback(const StateTransitionResult& result);
    
    /**
     * @brief 添加状态转换记录
     * @param result 转换结果
     */
    void AddStateHistoryRecord(const StateTransitionResult& result);
    
    /**
     * @brief 清理历史记录（保持在限制内）
     */
    void TrimStateHistory();
    
    /**
     * @brief 生成转换消息
     * @param fromState 源状态
     * @param toState 目标状态
     * @param event 事件
     * @param success 是否成功
     * @return 消息字符串
     */
    std::wstring GenerateTransitionMessage(RotationState fromState,
                                          RotationState toState,
                                          RotationEvent event,
                                          bool success) const;
};

/**
 * @brief 状态机工厂类
 */
class RotationStateMachineFactory {
public:
    /**
     * @brief 创建标准状态机
     * @return 状态机实例
     */
    static std::unique_ptr<RotationStateMachine> CreateStandard();
    
    /**
     * @brief 创建带回调的状态机
     * @param callback 状态回调函数
     * @return 状态机实例
     */
    static std::unique_ptr<RotationStateMachine> CreateWithCallback(StateCallback callback);
};