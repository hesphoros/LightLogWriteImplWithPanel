# Enhanced Log Filter System

## 概述

本项目已成功扩展了 ILogFilter 接口，实现了一个功能强大、高性能的日志过滤系统。新系统提供了以下主要功能：

- **基础过滤器**: 级别、关键词、正则表达式、频率限制、线程过滤
- **组合过滤器**: 支持多种组合策略（AND、OR、多数决等）
- **过滤器管理**: 工厂模式、配置管理、模板系统
- **性能统计**: 详细的处理时间和操作统计
- **动态配置**: 运行时配置和优先级调整

## 新增接口

### 1. 扩展的 ILogFilter 接口

```cpp
class ILogFilter {
public:
    // 核心过滤功能
    virtual FilterOperation ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo = nullptr) = 0;
    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;
    virtual std::wstring GetFilterName() const = 0;
    
    // 配置管理
    virtual bool SetConfiguration(const std::wstring& config) = 0;
    virtual std::wstring GetConfiguration() const = 0;
    virtual bool ValidateConfiguration(const std::wstring& config) const = 0;
    
    // 优先级控制
    virtual int GetPriority() const = 0;
    virtual void SetPriority(int priority) = 0;
    
    // 性能统计
    virtual FilterStatistics GetStatistics() const = 0;
    virtual void ResetStatistics() = 0;
    
    // 性能优化提示
    virtual bool CanQuickReject(LogLevel level) const = 0;
    virtual bool IsExpensive() const = 0;
    
    // 上下文管理
    virtual void SetContext(const FilterContext& context) = 0;
    virtual FilterContext GetContext() const = 0;
    
    // 生命周期管理
    virtual std::unique_ptr<ILogFilter> Clone() const = 0;
    virtual void Reset() = 0;
    
    // 描述和元数据
    virtual std::wstring GetDescription() const = 0;
    virtual std::wstring GetVersion() const = 0;
};
```

### 2. ICompositeFilter 组合过滤器接口

```cpp
class ICompositeFilter : public ILogFilter {
public:
    // 过滤器链管理
    virtual void AddFilter(std::shared_ptr<ILogFilter> filter) = 0;
    virtual void RemoveFilter(const std::wstring& filterName) = 0;
    virtual void InsertFilter(size_t position, std::shared_ptr<ILogFilter> filter) = 0;
    virtual void ClearFilters() = 0;
    
    // 查询和遍历
    virtual size_t GetFilterCount() const = 0;
    virtual std::shared_ptr<ILogFilter> GetFilter(size_t index) const = 0;
    virtual std::shared_ptr<ILogFilter> GetFilter(const std::wstring& name) const = 0;
    virtual std::vector<std::shared_ptr<ILogFilter>> GetAllFilters() const = 0;
    
    // 组合策略
    virtual void SetCompositionStrategy(CompositionStrategy strategy) = 0;
    virtual CompositionStrategy GetCompositionStrategy() const = 0;
    
    // 自定义组合逻辑
    virtual void SetCustomCompositionLogic(
        std::function<FilterOperation(const std::vector<FilterOperation>&)> logic) = 0;
};
```

### 3. IFilterManager 过滤器管理接口

```cpp
class IFilterManager {
public:
    // 过滤器注册和工厂
    virtual void RegisterFilterType(const std::wstring& typeName, FilterFactory factory) = 0;
    virtual std::unique_ptr<ILogFilter> CreateFilter(const std::wstring& typeName) = 0;
    virtual std::unique_ptr<ILogFilter> CreateFilter(const std::wstring& typeName, const std::wstring& config) = 0;
    virtual std::vector<std::wstring> GetAvailableFilterTypes() const = 0;
    
    // 配置管理
    virtual void SaveFilterConfiguration(const std::wstring& name, const std::shared_ptr<ILogFilter>& filter) = 0;
    virtual std::shared_ptr<ILogFilter> LoadFilterConfiguration(const std::wstring& name) = 0;
    virtual void DeleteFilterConfiguration(const std::wstring& name) = 0;
    virtual std::vector<std::wstring> GetSavedConfigurations() const = 0;
    
    // 过滤器验证
    virtual FilterValidationResult ValidateFilter(const std::shared_ptr<ILogFilter>& filter) = 0;
    virtual FilterValidationResult ValidateConfiguration(const std::wstring& filterType, const std::wstring& configuration) = 0;
    
    // 模板管理
    virtual void CreateFilterTemplate(const std::wstring& templateName, const std::wstring& filterType, const std::wstring& defaultConfig) = 0;
    virtual std::unique_ptr<ILogFilter> CreateFromTemplate(const std::wstring& templateName) = 0;
    virtual std::vector<std::wstring> GetAvailableTemplates() const = 0;
    
    // 组合过滤器帮助器
    virtual std::unique_ptr<ICompositeFilter> CreateCompositeFilter(CompositionStrategy strategy) = 0;
    virtual std::unique_ptr<ICompositeFilter> CreateCompositeFilterFromConfig(const std::wstring& config) = 0;
};
```

## 已实现的过滤器类型

### 1. BaseLogFilter - 基础过滤器类
提供所有过滤器的通用功能：
- 统计跟踪
- 配置管理
- 上下文支持
- 线程安全

### 2. LevelFilter - 级别过滤器
```cpp
auto levelFilter = std::make_unique<LevelFilter>(LogLevel::Warning, LogLevel::Error);
levelFilter->SetMinLevel(LogLevel::Error);  // 只允许 Error 及以上级别
levelFilter->SetLevelRange(LogLevel::Warning, LogLevel::Critical);  // 设置级别范围
```

### 3. KeywordFilter - 关键词过滤器
```cpp
auto keywordFilter = std::make_unique<KeywordFilter>(false);  // 不区分大小写
keywordFilter->AddIncludeKeyword(L"important");  // 包含关键词
keywordFilter->AddExcludeKeyword(L"debug");      // 排除关键词
```

### 4. RegexFilter - 正则表达式过滤器
```cpp
auto regexFilter = std::make_unique<RegexFilter>(L"error|warning|critical");
regexFilter->SetPattern(L"\\b(error|fail|exception)\\b");  // 匹配错误相关词汇
```

### 5. RateLimitFilter - 频率限制过滤器
```cpp
auto rateLimitFilter = std::make_unique<RateLimitFilter>(100, 10);  // 100/秒，突发10
rateLimitFilter->SetRateLimit(50, 5);  // 调整频率限制
```

### 6. ThreadFilter - 线程过滤器
```cpp
auto threadFilter = std::make_unique<ThreadFilter>(true);  // 使用允许列表
threadFilter->AddAllowedThread(std::this_thread::get_id());  // 添加允许的线程
```

### 7. CompositeFilter - 组合过滤器
```cpp
auto compositeFilter = std::make_unique<CompositeFilter>(L"MyComposite", CompositionStrategy::AllMustPass);

// 添加子过滤器
compositeFilter->AddFilter(levelFilter);
compositeFilter->AddFilter(keywordFilter);

// 设置组合策略
compositeFilter->SetCompositionStrategy(CompositionStrategy::AnyCanPass);

// 按优先级排序
compositeFilter->SortFiltersByPriority();
```

## 组合策略

### AllMustPass (AND 逻辑)
所有过滤器都必须通过，日志才会被允许。

### AnyCanPass (OR 逻辑)
任何一个过滤器通过，日志就会被允许。

### MajorityRule (多数决)
大多数过滤器通过，日志才会被允许。

### FirstMatch (首次匹配)
第一个非允许结果决定最终结果。

### Custom (自定义逻辑)
```cpp
compositeFilter->SetCustomCompositionLogic([](const std::vector<FilterOperation>& results) {
    // 自定义逻辑：如果有任何 Transform，返回 Transform
    for (auto result : results) {
        if (result == FilterOperation::Transform) {
            return FilterOperation::Transform;
        }
    }
    // 否则使用 AND 逻辑
    for (auto result : results) {
        if (result == FilterOperation::Block) {
            return FilterOperation::Block;
        }
    }
    return FilterOperation::Allow;
});
```

## 使用示例

### 基本使用
```cpp
#include "include/log/FilterManager.h"
#include "include/log/LogFilters.h"

// 创建过滤器管理器
auto filterManager = std::make_unique<FilterManager>();

// 创建级别过滤器
auto levelFilter = filterManager->CreateFilter(L"LevelFilter");
levelFilter->SetConfiguration(L"minLevel=Warning");

// 创建关键词过滤器
auto keywordFilter = filterManager->CreateFilter(L"KeywordFilter");
keywordFilter->SetConfiguration(L"exclude=test,debug;include=important");

// 使用过滤器
LogCallbackInfo logInfo;
logInfo.level = LogLevel::Error;
logInfo.message = L"This is an important error message";

FilterOperation result = levelFilter->ApplyFilter(logInfo);
// result == FilterOperation::Allow
```

### 组合过滤器使用
```cpp
// 创建组合过滤器
auto compositeFilter = filterManager->CreateCompositeFilter(CompositionStrategy::AllMustPass);

// 添加多个过滤器
compositeFilter->AddFilter(levelFilter);
compositeFilter->AddFilter(keywordFilter);

// 设置优先级
compositeFilter->SetFilterPriority(L"LevelFilter", static_cast<int>(FilterPriority::High));
compositeFilter->SetFilterPriority(L"KeywordFilter", static_cast<int>(FilterPriority::Normal));

// 应用过滤器
FilterOperation result = compositeFilter->ApplyFilter(logInfo);
```

### 配置管理
```cpp
// 保存配置
filterManager->SaveFilterConfiguration(L"MyErrorFilter", levelFilter);

// 加载配置
auto loadedFilter = filterManager->LoadFilterConfiguration(L"MyErrorFilter");

// 从模板创建
auto templateFilter = filterManager->CreateFromTemplate(L"ErrorOnly");

// 导出配置
std::wstring configData = filterManager->ExportConfiguration(L"MyErrorFilter");

// 导入配置
filterManager->ImportConfiguration(L"ImportedFilter", configData);
```

### 性能统计
```cpp
// 获取统计信息
FilterStatistics stats = filter->GetStatistics();
std::wcout << L"Total processed: " << stats.totalProcessed << std::endl;
std::wcout << L"Average time: " << stats.averageProcessingTime << L"ms" << std::endl;

// 重置统计
filter->ResetStatistics();

// 获取所有过滤器统计
auto allStats = filterManager->GetAllFilterStatistics();
for (const auto& pair : allStats) {
    std::wcout << L"Filter: " << pair.first 
               << L", Processed: " << pair.second.totalProcessed << std::endl;
}
```

## 性能优化

### 1. 快速拒绝
```cpp
// 过滤器可以基于日志级别快速拒绝
if (filter->CanQuickReject(LogLevel::Debug)) {
    // 快速路径，无需完整处理
    return FilterOperation::Block;
}
```

### 2. 短路优化
```cpp
// 组合过滤器支持短路优化
compositeFilter->SetShortCircuitEnabled(true);  // 默认启用
```

### 3. 优先级排序
```cpp
// 高优先级过滤器先执行
filter->SetPriority(static_cast<int>(FilterPriority::High));
compositeFilter->SortFiltersByPriority();
```

### 4. 昂贵操作标记
```cpp
// 正则表达式过滤器标记为昂贵操作
if (filter->IsExpensive()) {
    // 可以延迟执行或使用缓存
}
```

## 与现有系统集成

新的过滤器系统完全兼容现有的 BaseLogOutput 实现：

```cpp
// BaseLogOutput 中的过滤器应用
FilterOperation BaseLogOutput::ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo) {
    if (!m_filter || !m_filter->IsEnabled()) {
        return FilterOperation::Allow;
    }
    
    return m_filter->ApplyFilter(logInfo, transformedInfo);
}

// 设置新的组合过滤器
auto compositeFilter = filterManager->CreateCompositeFilter(CompositionStrategy::AllMustPass);
// ... 配置过滤器 ...
baseLogOutput->SetFilter(compositeFilter);
```

## 线程安全

所有过滤器实现都是线程安全的：
- 使用 `std::atomic` 对于简单状态
- 使用 `std::mutex` 保护复杂数据结构
- 统计信息更新使用专用互斥锁

## 配置格式

过滤器支持简单的配置字符串格式：

```cpp
// 级别过滤器
filter->SetConfiguration(L"minLevel=Warning;maxLevel=Error");

// 关键词过滤器
filter->SetConfiguration(L"include=important,critical;exclude=test,debug;caseSensitive=false");

// 频率限制过滤器
filter->SetConfiguration(L"maxPerSecond=100;maxBurst=10");

// 正则表达式过滤器
filter->SetConfiguration(L"pattern=\\b(error|warning)\\b");
```

## 扩展指南

要添加新的过滤器类型：

1. 继承 `BaseLogFilter`
2. 实现 `ApplyFilter` 方法
3. 实现 `Clone` 方法
4. 在 FilterManager 中注册：

```cpp
filterManager->RegisterFilterType(L"MyCustomFilter", []() {
    return std::make_unique<MyCustomFilter>();
});
```

## 总结

新的过滤器系统提供了：

✅ **高性能**: 优化的短路逻辑和快速路径  
✅ **高灵活性**: 多种组合策略和自定义逻辑  
✅ **易扩展**: 简单的插件架构和工厂模式  
✅ **易使用**: 丰富的模板和配置管理  
✅ **高可靠**: 全面的错误处理和验证  
✅ **线程安全**: 完整的并发支持  
✅ **统计监控**: 详细的性能和使用统计  

这个增强的过滤器系统大大提升了日志系统的功能性和可用性，为复杂的日志处理需求提供了强大的支持。