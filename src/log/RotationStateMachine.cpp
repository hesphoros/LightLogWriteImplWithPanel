#include "log/RotationStateMachine.h"
#include <sstream>
#include <iomanip>
#include <set>

/*****************************************************************************
 *  RotationStateMachine
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     RotationStateMachine.cpp
 *  @brief    轮转状态机实现
 *  @details  管理轮转过程的状态转换，确保轮转操作的原子性和可控性
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/16
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

RotationStateMachine::RotationStateMachine()
    : currentState_(RotationState::Idle)
    , currentStateStartTime_(std::chrono::system_clock::now())
    , processingStartTime_(std::chrono::system_clock::now())
{
    InitializeTransitionTable();
}

RotationStateMachine::~RotationStateMachine() = default;

RotationState RotationStateMachine::GetCurrentState() const
{
    return currentState_.load();
}

StateTransitionResult RotationStateMachine::TriggerEvent(RotationEvent event,
                                                        const RotationStateMachineContext& context)
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    RotationState currentState = currentState_.load();
    
    // 查找状态转换表
    auto key = std::make_pair(currentState, event);
    auto it = transitionTable_.find(key);
    
    if (it == transitionTable_.end()) {
        // 无效的状态转换
        StateTransitionResult result(false, currentState, currentState, event,
                                   L"Invalid state transition");
        
        AddStateHistoryRecord(result);
        return result;
    }
    
    RotationState newState = it->second;
    return TransitionToState(newState, event, context);
}

bool RotationStateMachine::CanTriggerEvent(RotationEvent event) const
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    RotationState currentState = currentState_.load();
    auto key = std::make_pair(currentState, event);
    
    return transitionTable_.find(key) != transitionTable_.end();
}

void RotationStateMachine::Reset()
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    RotationState oldState = currentState_.load();
    currentState_.store(RotationState::Idle);
    currentStateStartTime_ = std::chrono::system_clock::now();
    processingStartTime_ = currentStateStartTime_;
    
    // 记录重置事件
    StateTransitionResult result(true, oldState, RotationState::Idle, RotationEvent::Reset,
                               L"State machine reset to idle");
    
    AddStateHistoryRecord(result);
    TriggerStateCallback(result);
}

void RotationStateMachine::SetStateCallback(StateCallback callback)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    stateCallback_ = callback;
}

std::vector<StateTransitionResult> RotationStateMachine::GetStateHistory() const
{
    std::lock_guard<std::mutex> lock(historyMutex_);
    return stateHistory_;
}

void RotationStateMachine::ClearStateHistory()
{
    std::lock_guard<std::mutex> lock(historyMutex_);
    stateHistory_.clear();
}

std::chrono::milliseconds RotationStateMachine::GetCurrentStateDuration() const
{
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - currentStateStartTime_);
}

std::chrono::milliseconds RotationStateMachine::GetTotalProcessingTime() const
{
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - processingStartTime_);
}

bool RotationStateMachine::IsInFinalState() const
{
    RotationState state = currentState_.load();
    return state == RotationState::Completed || state == RotationState::Failed;
}

bool RotationStateMachine::IsInErrorState() const
{
    RotationState state = currentState_.load();
    return state == RotationState::Failed || state == RotationState::Recovering || 
           state == RotationState::Rollback;
}

std::wstring RotationStateMachine::GetStateName(RotationState state)
{
    switch (state) {
        case RotationState::Idle: return L"Idle";
        case RotationState::Checking: return L"Checking";
        case RotationState::Preparing: return L"Preparing";
        case RotationState::PreCheck: return L"PreCheck";
        case RotationState::Rotating: return L"Rotating";
        case RotationState::Compressing: return L"Compressing";
        case RotationState::Cleaning: return L"Cleaning";
        case RotationState::Completing: return L"Completing";
        case RotationState::Completed: return L"Completed";
        case RotationState::Failed: return L"Failed";
        case RotationState::Recovering: return L"Recovering";
        case RotationState::Rollback: return L"Rollback";
        default: return L"Unknown";
    }
}

std::wstring RotationStateMachine::GetEventName(RotationEvent event)
{
    switch (event) {
        case RotationEvent::Start: return L"Start";
        case RotationEvent::CheckPassed: return L"CheckPassed";
        case RotationEvent::CheckFailed: return L"CheckFailed";
        case RotationEvent::PrepareDone: return L"PrepareDone";
        case RotationEvent::PreCheckPassed: return L"PreCheckPassed";
        case RotationEvent::PreCheckFailed: return L"PreCheckFailed";
        case RotationEvent::RotationSuccess: return L"RotationSuccess";
        case RotationEvent::RotationFailed: return L"RotationFailed";
        case RotationEvent::CompressionSuccess: return L"CompressionSuccess";
        case RotationEvent::CompressionFailed: return L"CompressionFailed";
        case RotationEvent::CleanupDone: return L"CleanupDone";
        case RotationEvent::Complete: return L"Complete";
        case RotationEvent::Fail: return L"Fail";
        case RotationEvent::Recover: return L"Recover";
        case RotationEvent::Rollback: return L"Rollback";
        case RotationEvent::Reset: return L"Reset";
        default: return L"Unknown";
    }
}

std::string RotationStateMachine::GetStateMachineDiagram() const
{
    std::ostringstream dot;
    
    dot << "digraph RotationStateMachine {\n";
    dot << "  rankdir=TD;\n";
    dot << "  node [shape=box, style=rounded];\n\n";
    
    // 定义节点
    dot << "  Idle [color=lightblue];\n";
    dot << "  Checking [color=yellow];\n";
    dot << "  Preparing [color=yellow];\n";
    dot << "  PreCheck [color=yellow];\n";
    dot << "  Rotating [color=orange];\n";
    dot << "  Compressing [color=orange];\n";
    dot << "  Cleaning [color=orange];\n";
    dot << "  Completing [color=orange];\n";
    dot << "  Completed [color=lightgreen];\n";
    dot << "  Failed [color=red];\n";
    dot << "  Recovering [color=pink];\n";
    dot << "  Rollback [color=pink];\n\n";
    
    // 定义状态转换
    dot << "  Idle -> Checking [label=\"Start\"];\n";
    dot << "  Checking -> Preparing [label=\"CheckPassed\"];\n";
    dot << "  Checking -> Failed [label=\"CheckFailed\"];\n";
    dot << "  Preparing -> PreCheck [label=\"PrepareDone\"];\n";
    dot << "  PreCheck -> Rotating [label=\"PreCheckPassed\"];\n";
    dot << "  PreCheck -> Failed [label=\"PreCheckFailed\"];\n";
    dot << "  Rotating -> Compressing [label=\"RotationSuccess\"];\n";
    dot << "  Rotating -> Failed [label=\"RotationFailed\"];\n";
    dot << "  Compressing -> Cleaning [label=\"CompressionSuccess\"];\n";
    dot << "  Compressing -> Cleaning [label=\"CompressionFailed\"];\n";
    dot << "  Cleaning -> Completing [label=\"CleanupDone\"];\n";
    dot << "  Completing -> Completed [label=\"Complete\"];\n";
    dot << "  Failed -> Recovering [label=\"Recover\"];\n";
    dot << "  Failed -> Rollback [label=\"Rollback\"];\n";
    dot << "  Recovering -> Idle [label=\"Complete\"];\n";
    dot << "  Recovering -> Failed [label=\"Fail\"];\n";
    dot << "  Rollback -> Idle [label=\"Complete\"];\n";
    dot << "  Rollback -> Failed [label=\"Fail\"];\n";
    dot << "  \"*\" -> Idle [label=\"Reset\"];\n";
    
    dot << "}\n";
    
    return dot.str();
}

bool RotationStateMachine::ValidateStateMachine() const
{
    // 检查状态转换表的完整性
    std::set<RotationState> allStates = {
        RotationState::Idle, RotationState::Checking, RotationState::Preparing,
        RotationState::PreCheck, RotationState::Rotating, RotationState::Compressing,
        RotationState::Cleaning, RotationState::Completing, RotationState::Completed,
        RotationState::Failed, RotationState::Recovering, RotationState::Rollback
    };
    
    std::set<RotationEvent> allEvents = {
        RotationEvent::Start, RotationEvent::CheckPassed, RotationEvent::CheckFailed,
        RotationEvent::PrepareDone, RotationEvent::PreCheckPassed, RotationEvent::PreCheckFailed,
        RotationEvent::RotationSuccess, RotationEvent::RotationFailed,
        RotationEvent::CompressionSuccess, RotationEvent::CompressionFailed,
        RotationEvent::CleanupDone, RotationEvent::Complete, RotationEvent::Fail,
        RotationEvent::Recover, RotationEvent::Rollback, RotationEvent::Reset
    };
    
    // 检查是否每个状态都有相应的转换定义
    for (RotationState state : allStates) {
        bool hasTransition = false;
        for (RotationEvent event : allEvents) {
            auto key = std::make_pair(state, event);
            if (transitionTable_.find(key) != transitionTable_.end()) {
                hasTransition = true;
                break;
            }
        }
        
        // 终止状态可以没有转换
        if (!hasTransition && state != RotationState::Completed && state != RotationState::Failed) {
            return false;
        }
    }
    
    return true;
}

// 私有方法实现

void RotationStateMachine::InitializeTransitionTable()
{
    // 从 Idle 状态的转换
    transitionTable_[{RotationState::Idle, RotationEvent::Start}] = RotationState::Checking;
    transitionTable_[{RotationState::Idle, RotationEvent::Reset}] = RotationState::Idle;
    
    // 从 Checking 状态的转换
    transitionTable_[{RotationState::Checking, RotationEvent::CheckPassed}] = RotationState::Preparing;
    transitionTable_[{RotationState::Checking, RotationEvent::CheckFailed}] = RotationState::Failed;
    transitionTable_[{RotationState::Checking, RotationEvent::Reset}] = RotationState::Idle;
    
    // 从 Preparing 状态的转换
    transitionTable_[{RotationState::Preparing, RotationEvent::PrepareDone}] = RotationState::PreCheck;
    transitionTable_[{RotationState::Preparing, RotationEvent::Fail}] = RotationState::Failed;
    transitionTable_[{RotationState::Preparing, RotationEvent::Reset}] = RotationState::Idle;
    
    // 从 PreCheck 状态的转换
    transitionTable_[{RotationState::PreCheck, RotationEvent::PreCheckPassed}] = RotationState::Rotating;
    transitionTable_[{RotationState::PreCheck, RotationEvent::PreCheckFailed}] = RotationState::Failed;
    transitionTable_[{RotationState::PreCheck, RotationEvent::Reset}] = RotationState::Idle;
    
    // 从 Rotating 状态的转换
    transitionTable_[{RotationState::Rotating, RotationEvent::RotationSuccess}] = RotationState::Compressing;
    transitionTable_[{RotationState::Rotating, RotationEvent::RotationFailed}] = RotationState::Failed;
    transitionTable_[{RotationState::Rotating, RotationEvent::Reset}] = RotationState::Idle;
    
    // 从 Compressing 状态的转换
    transitionTable_[{RotationState::Compressing, RotationEvent::CompressionSuccess}] = RotationState::Cleaning;
    transitionTable_[{RotationState::Compressing, RotationEvent::CompressionFailed}] = RotationState::Cleaning; // 压缩失败仍继续清理
    transitionTable_[{RotationState::Compressing, RotationEvent::Reset}] = RotationState::Idle;
    
    // 从 Cleaning 状态的转换
    transitionTable_[{RotationState::Cleaning, RotationEvent::CleanupDone}] = RotationState::Completing;
    transitionTable_[{RotationState::Cleaning, RotationEvent::Fail}] = RotationState::Failed;
    transitionTable_[{RotationState::Cleaning, RotationEvent::Reset}] = RotationState::Idle;
    
    // 从 Completing 状态的转换
    transitionTable_[{RotationState::Completing, RotationEvent::Complete}] = RotationState::Completed;
    transitionTable_[{RotationState::Completing, RotationEvent::Fail}] = RotationState::Failed;
    transitionTable_[{RotationState::Completing, RotationEvent::Reset}] = RotationState::Idle;
    
    // 从 Failed 状态的转换
    transitionTable_[{RotationState::Failed, RotationEvent::Recover}] = RotationState::Recovering;
    transitionTable_[{RotationState::Failed, RotationEvent::Rollback}] = RotationState::Rollback;
    transitionTable_[{RotationState::Failed, RotationEvent::Reset}] = RotationState::Idle;
    
    // 从 Recovering 状态的转换
    transitionTable_[{RotationState::Recovering, RotationEvent::Complete}] = RotationState::Idle;
    transitionTable_[{RotationState::Recovering, RotationEvent::Fail}] = RotationState::Failed;
    transitionTable_[{RotationState::Recovering, RotationEvent::Reset}] = RotationState::Idle;
    
    // 从 Rollback 状态的转换
    transitionTable_[{RotationState::Rollback, RotationEvent::Complete}] = RotationState::Idle;
    transitionTable_[{RotationState::Rollback, RotationEvent::Fail}] = RotationState::Failed;
    transitionTable_[{RotationState::Rollback, RotationEvent::Reset}] = RotationState::Idle;
    
    // 从任何状态都可以重置
    for (int state = static_cast<int>(RotationState::Idle); state <= static_cast<int>(RotationState::Rollback); ++state) {
        RotationState rotState = static_cast<RotationState>(state);
        if (rotState != RotationState::Idle) { // 避免重复定义
            transitionTable_[{rotState, RotationEvent::Reset}] = RotationState::Idle;
        }
    }
}

StateTransitionResult RotationStateMachine::TransitionToState(RotationState newState,
                                                             RotationEvent event,
                                                             const RotationStateMachineContext& context)
{
    RotationState oldState = currentState_.load();
    
    // 更新状态
    currentState_.store(newState);
    currentStateStartTime_ = std::chrono::system_clock::now();
    
    // 如果是第一次从 Idle 状态转换，记录处理开始时间
    if (oldState == RotationState::Idle && newState != RotationState::Idle) {
        processingStartTime_ = currentStateStartTime_;
    }
    
    // 生成转换消息
    std::wstring message = GenerateTransitionMessage(oldState, newState, event, true);
    
    StateTransitionResult result(true, oldState, newState, event, message);
    
    // 添加到历史记录
    AddStateHistoryRecord(result);
    
    // 触发回调
    TriggerStateCallback(result);
    
    return result;
}

void RotationStateMachine::TriggerStateCallback(const StateTransitionResult& result)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    
    if (stateCallback_) {
        try {
            stateCallback_(result);
        }
        catch (...) {
            // 忽略回调中的异常
        }
    }
}

void RotationStateMachine::AddStateHistoryRecord(const StateTransitionResult& result)
{
    std::lock_guard<std::mutex> lock(historyMutex_);
    
    stateHistory_.push_back(result);
    
    // 保持历史记录在限制内
    TrimStateHistory();
}

void RotationStateMachine::TrimStateHistory()
{
    if (stateHistory_.size() > MAX_HISTORY_SIZE) {
        size_t removeCount = stateHistory_.size() - MAX_HISTORY_SIZE;
        stateHistory_.erase(stateHistory_.begin(), stateHistory_.begin() + removeCount);
    }
}

std::wstring RotationStateMachine::GenerateTransitionMessage(RotationState fromState,
                                                           RotationState toState,
                                                           RotationEvent event,
                                                           bool success) const
{
    std::wostringstream message;
    
    if (success) {
        message << L"State transition: " << GetStateName(fromState) 
                << L" -> " << GetStateName(toState) 
                << L" (Event: " << GetEventName(event) << L")";
    } else {
        message << L"Failed state transition: " << GetStateName(fromState) 
                << L" -X-> " << GetStateName(toState) 
                << L" (Event: " << GetEventName(event) << L")";
    }
    
    return message.str();
}

// RotationStateMachineFactory 实现

std::unique_ptr<RotationStateMachine> RotationStateMachineFactory::CreateStandard()
{
    return std::make_unique<RotationStateMachine>();
}

std::unique_ptr<RotationStateMachine> RotationStateMachineFactory::CreateWithCallback(StateCallback callback)
{
    auto stateMachine = std::make_unique<RotationStateMachine>();
    stateMachine->SetStateCallback(callback);
    return stateMachine;
}