#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "ICompositeFilter.h"
#include "LogFilters.h"
#include <algorithm>

/**
 * @brief Concrete implementation of composite filter
 */
class CompositeFilter : public BaseLogFilter, public virtual ICompositeFilter {
private:
    std::vector<std::shared_ptr<ILogFilter>> m_filters;
    mutable std::mutex m_filtersMutex;
    
    CompositionStrategy m_strategy;
    std::function<FilterOperation(const std::vector<FilterOperation>&)> m_customLogic;
    
    bool m_shortCircuitEnabled;
    
public:
    explicit CompositeFilter(const std::wstring& name = L"CompositeFilter",
                           CompositionStrategy strategy = CompositionStrategy::AllMustPass);
    
    // ILogFilter implementation
    FilterOperation ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo = nullptr) override;
    std::unique_ptr<ILogFilter> Clone() const override;
    
    // ICompositeFilter implementation
    void AddFilter(std::shared_ptr<ILogFilter> filter) override;
    void RemoveFilter(const std::wstring& filterName) override;
    void InsertFilter(size_t position, std::shared_ptr<ILogFilter> filter) override;
    void ClearFilters() override;
    
    size_t GetFilterCount() const override;
    std::shared_ptr<ILogFilter> GetFilter(size_t index) const override;
    std::shared_ptr<ILogFilter> GetFilter(const std::wstring& name) const override;
    std::vector<std::shared_ptr<ILogFilter>> GetAllFilters() const override;
    
    void SetCompositionStrategy(CompositionStrategy strategy) override;
    CompositionStrategy GetCompositionStrategy() const override;
    
    void SetCustomCompositionLogic(
        std::function<FilterOperation(const std::vector<FilterOperation>&)> logic) override;
    
    void EnableFilter(const std::wstring& filterName, bool enabled) override;
    void SetFilterPriority(const std::wstring& filterName, int priority) override;
    
    bool GetShortCircuitEnabled() const override;
    void SetShortCircuitEnabled(bool enabled) override;
    
    void SortFiltersByPriority() override;
    void MoveFilter(const std::wstring& filterName, size_t newPosition) override;
    
    // Performance optimization
    bool CanQuickReject(LogLevel level) const override;
    bool IsExpensive() const override;
    
protected:
    bool DoValidateConfiguration(const std::wstring& config) const override;
    void DoReset() override;
    
private:
    FilterOperation ApplyAllMustPass(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo);
    FilterOperation ApplyAnyCanPass(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo);
    FilterOperation ApplyMajorityRule(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo);
    FilterOperation ApplyFirstMatch(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo);
    FilterOperation ApplyCustomLogic(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo);
    
    std::vector<std::shared_ptr<ILogFilter>> GetEnabledFilters() const;
    void SortFiltersByPriorityInternal(std::vector<std::shared_ptr<ILogFilter>>& filters) const;
};