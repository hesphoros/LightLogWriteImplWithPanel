#pragma once

#include "ILogFilter.h"
#include "LogFilters.h"
#include "../nlohmann/json.hpp"
#include <memory>
#include <string>
#include <functional>
#include <map>

/*****************************************************************************
 *  LogFilterFactory
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     LogFilterFactory.h
 *  @brief    日志过滤器工厂和序列化系统
 *  @details  提供过滤器的创建、序列化和反序列化功能
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/17
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

/**
 * @brief 过滤器类型创建函数
 */
using FilterCreatorFunc = std::function<std::unique_ptr<ILogFilter>()>;

/**
 * @brief 过滤器序列化函数
 */
using FilterSerializerFunc = std::function<nlohmann::json(const ILogFilter*)>;

/**
 * @brief 过滤器反序列化函数
 */
using FilterDeserializerFunc = std::function<void(ILogFilter*, const nlohmann::json&)>;

/**
 * @brief 过滤器类型信息
 */
struct FilterTypeInfo {
    std::wstring typeName;              // 类型名称
    std::wstring description;           // 类型描述
    FilterCreatorFunc creator;          // 创建函数
    FilterSerializerFunc serializer;    // 序列化函数
    FilterDeserializerFunc deserializer; // 反序列化函数
    bool isBuiltin;                     // 是否是内置类型
};

/**
 * @brief 日志过滤器工厂
 * @details 管理过滤器的创建、注册和序列化
 */
class LogFilterFactory {
private:
    static std::map<std::wstring, FilterTypeInfo> filterTypes_;
    static bool initialized_;
    
public:
    /**
     * @brief 初始化工厂，注册内置过滤器类型
     */
    static void Initialize();
    
    /**
     * @brief 注册过滤器类型
     * @param typeName 类型名称
     * @param info 类型信息
     * @return 是否注册成功
     */
    static bool RegisterFilterType(const std::wstring& typeName, const FilterTypeInfo& info);
    
    /**
     * @brief 创建过滤器实例
     * @param typeName 过滤器类型名称
     * @return 过滤器实例，失败返回nullptr
     */
    static std::unique_ptr<ILogFilter> CreateFilter(const std::wstring& typeName);
    
    /**
     * @brief 从配置创建过滤器
     * @param typeName 过滤器类型名称
     * @param config 配置JSON对象
     * @return 过滤器实例，失败返回nullptr
     */
    static std::unique_ptr<ILogFilter> CreateFilterFromConfig(const std::wstring& typeName, const nlohmann::json& config);
    
    /**
     * @brief 序列化过滤器
     * @param filter 过滤器实例
     * @return JSON对象
     */
    static nlohmann::json SerializeFilter(const ILogFilter* filter);
    
    /**
     * @brief 反序列化过滤器
     * @param json JSON对象
     * @return 过滤器实例，失败返回nullptr
     */
    static std::unique_ptr<ILogFilter> DeserializeFilter(const nlohmann::json& json);
    
    /**
     * @brief 获取所有注册的过滤器类型
     * @return 类型名称列表
     */
    static std::vector<std::wstring> GetRegisteredTypes();
    
    /**
     * @brief 获取过滤器类型信息
     * @param typeName 类型名称
     * @return 类型信息，不存在返回nullptr
     */
    static const FilterTypeInfo* GetTypeInfo(const std::wstring& typeName);
    
    /**
     * @brief 检查类型是否已注册
     * @param typeName 类型名称
     * @return 是否已注册
     */
    static bool IsTypeRegistered(const std::wstring& typeName);

private:
    // 内置过滤器的创建和序列化函数
    static void RegisterBuiltinFilters();
    static FilterTypeInfo CreateLevelFilterInfo();
    static FilterTypeInfo CreateKeywordFilterInfo();
    static FilterTypeInfo CreateRegexFilterInfo();
    static FilterTypeInfo CreateFrequencyFilterInfo();
    static FilterTypeInfo CreateThreadFilterInfo();
    
    // 序列化辅助函数
    static std::string WStringToString(const std::wstring& wstr);
    static std::wstring StringToWString(const std::string& str);
    static std::string LogLevelToString(LogLevel level);
    static LogLevel StringToLogLevel(const std::string& str);
};