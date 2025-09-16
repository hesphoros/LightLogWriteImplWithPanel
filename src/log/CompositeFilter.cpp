#include "../../include/log/CompositeFilter.h"
#include <numeric>
#include <memory>

using namespace std::chrono;

CompositeFilter::CompositeFilter(const std::wstring& name, CompositionStrategy strategy)
    : BaseLogFilter(name, L"Composite filter combining multiple filters", L"1.0.0")
    , m_strategy(strategy)
    , m_shortCircuitEnabled(true)
{
}

FilterOperation CompositeFilter::ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo) {
    auto start = steady_clock::now();
    
    FilterOperation result = FilterOperation::Allow;
    
    switch (m_strategy) {
    case CompositionStrategy::AllMustPass:
        result = ApplyAllMustPass(logInfo, transformedInfo);
        break;
    case CompositionStrategy::AnyCanPass:
        result = ApplyAnyCanPass(logInfo, transformedInfo);
        break;
    case CompositionStrategy::MajorityRule:
        result = ApplyMajorityRule(logInfo, transformedInfo);
        break;
    case CompositionStrategy::FirstMatch:
        result = ApplyFirstMatch(logInfo, transformedInfo);
        break;
    case CompositionStrategy::Custom:
        result = ApplyCustomLogic(logInfo, transformedInfo);
        break;
    }
    
    auto end = steady_clock::now();
    UpdateStatistics(result, duration_cast<milliseconds>(end - start));
    
    return result;
}

std::unique_ptr<ILogFilter> CompositeFilter::Clone() const {
    auto clone = std::make_unique<CompositeFilter>(BaseLogFilter::GetFilterName(), m_strategy);
    
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    for (const auto& filter : m_filters) {
        if (filter) {
            clone->AddFilter(filter->Clone());
        }
    }
    
    clone->BaseLogFilter::SetEnabled(BaseLogFilter::IsEnabled());
    clone->BaseLogFilter::SetPriority(BaseLogFilter::GetPriority());
    clone->BaseLogFilter::SetConfiguration(BaseLogFilter::GetConfiguration());
    clone->BaseLogFilter::SetContext(BaseLogFilter::GetContext());
    clone->SetShortCircuitEnabled(m_shortCircuitEnabled);
    clone->SetCustomCompositionLogic(m_customLogic);
    
    return std::unique_ptr<ILogFilter>(clone.release());
}

void CompositeFilter::AddFilter(std::shared_ptr<ILogFilter> filter) {
    if (!filter) return;
    
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    m_filters.push_back(filter);
}

void CompositeFilter::RemoveFilter(const std::wstring& filterName) {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    
    m_filters.erase(
        std::remove_if(m_filters.begin(), m_filters.end(),
            [&filterName](const std::shared_ptr<ILogFilter>& filter) {
                return filter && filter->GetFilterName() == filterName;
            }),
        m_filters.end());
}

void CompositeFilter::InsertFilter(size_t position, std::shared_ptr<ILogFilter> filter) {
    if (!filter) return;
    
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    
    if (position >= m_filters.size()) {
        m_filters.push_back(filter);
    }
    else {
        m_filters.insert(m_filters.begin() + position, filter);
    }
}

void CompositeFilter::ClearFilters() {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    m_filters.clear();
}

size_t CompositeFilter::GetFilterCount() const {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    return m_filters.size();
}

std::shared_ptr<ILogFilter> CompositeFilter::GetFilter(size_t index) const {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    
    if (index >= m_filters.size()) {
        return nullptr;
    }
    
    return m_filters[index];
}

std::shared_ptr<ILogFilter> CompositeFilter::GetFilter(const std::wstring& name) const {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    
    auto it = std::find_if(m_filters.begin(), m_filters.end(),
        [&name](const std::shared_ptr<ILogFilter>& filter) {
            return filter && filter->GetFilterName() == name;
        });
    
    return (it != m_filters.end()) ? *it : nullptr;
}

std::vector<std::shared_ptr<ILogFilter>> CompositeFilter::GetAllFilters() const {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    return m_filters;
}

void CompositeFilter::SetCompositionStrategy(CompositionStrategy strategy) {
    m_strategy = strategy;
}

CompositionStrategy CompositeFilter::GetCompositionStrategy() const {
    return m_strategy;
}

void CompositeFilter::SetCustomCompositionLogic(
    std::function<FilterOperation(const std::vector<FilterOperation>&)> logic) {
    m_customLogic = logic;
}

void CompositeFilter::EnableFilter(const std::wstring& filterName, bool enabled) {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    
    for (auto& filter : m_filters) {
        if (filter && filter->GetFilterName() == filterName) {
            filter->SetEnabled(enabled);
            break;
        }
    }
}

void CompositeFilter::SetFilterPriority(const std::wstring& filterName, int priority) {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    
    for (auto& filter : m_filters) {
        if (filter && filter->GetFilterName() == filterName) {
            filter->SetPriority(priority);
            break;
        }
    }
}

bool CompositeFilter::GetShortCircuitEnabled() const {
    return m_shortCircuitEnabled;
}

void CompositeFilter::SetShortCircuitEnabled(bool enabled) {
    m_shortCircuitEnabled = enabled;
}

void CompositeFilter::SortFiltersByPriority() {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    SortFiltersByPriorityInternal(m_filters);
}

void CompositeFilter::MoveFilter(const std::wstring& filterName, size_t newPosition) {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    
    auto it = std::find_if(m_filters.begin(), m_filters.end(),
        [&filterName](const std::shared_ptr<ILogFilter>& filter) {
            return filter && filter->GetFilterName() == filterName;
        });
    
    if (it != m_filters.end() && newPosition < m_filters.size()) {
        auto filter = *it;
        m_filters.erase(it);
        
        if (newPosition >= m_filters.size()) {
            m_filters.push_back(filter);
        }
        else {
            m_filters.insert(m_filters.begin() + newPosition, filter);
        }
    }
}

bool CompositeFilter::CanQuickReject(LogLevel level) const {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    
    // For AllMustPass, if any filter can quick reject, we can quick reject
    if (m_strategy == CompositionStrategy::AllMustPass) {
        for (const auto& filter : m_filters) {
            if (filter && filter->IsEnabled() && filter->CanQuickReject(level)) {
                return true;
            }
        }
    }
    
    return false;
}

bool CompositeFilter::IsExpensive() const {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    
    for (const auto& filter : m_filters) {
        if (filter && filter->IsEnabled() && filter->IsExpensive()) {
            return true;
        }
    }
    
    return false;
}

bool CompositeFilter::DoValidateConfiguration(const std::wstring& config) const {
    return true; // Accept any configuration for composite filters
}

void CompositeFilter::DoReset() {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    
    for (auto& filter : m_filters) {
        if (filter) {
            filter->Reset();
        }
    }
    
    m_strategy = CompositionStrategy::AllMustPass;
    m_shortCircuitEnabled = true;
    m_customLogic = nullptr;
}

FilterOperation CompositeFilter::ApplyAllMustPass(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo) {
    auto filters = GetEnabledFilters();
    
    LogCallbackInfo currentInfo = logInfo;
    LogCallbackInfo tempInfo;
    
    for (const auto& filter : filters) {
        FilterOperation result = filter->ApplyFilter(currentInfo, &tempInfo);
        
        if (result == FilterOperation::Block) {
            if (m_shortCircuitEnabled) {
                return FilterOperation::Block;
            }
        }
        else if (result == FilterOperation::Transform) {
            currentInfo = tempInfo;
        }
    }
    
    if (transformedInfo && currentInfo.message != logInfo.message) {
        *transformedInfo = currentInfo;
        return FilterOperation::Transform;
    }
    
    return FilterOperation::Allow;
}

FilterOperation CompositeFilter::ApplyAnyCanPass(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo) {
    auto filters = GetEnabledFilters();
    
    if (filters.empty()) {
        return FilterOperation::Allow;
    }
    
    LogCallbackInfo bestTransform = logInfo;
    bool hasTransform = false;
    
    for (const auto& filter : filters) {
        LogCallbackInfo tempInfo;
        FilterOperation result = filter->ApplyFilter(logInfo, &tempInfo);
        
        if (result == FilterOperation::Allow) {
            if (m_shortCircuitEnabled) {
                return FilterOperation::Allow;
            }
        }
        else if (result == FilterOperation::Transform) {
            bestTransform = tempInfo;
            hasTransform = true;
            if (m_shortCircuitEnabled) {
                if (transformedInfo) {
                    *transformedInfo = bestTransform;
                }
                return FilterOperation::Transform;
            }
        }
    }
    
    if (hasTransform) {
        if (transformedInfo) {
            *transformedInfo = bestTransform;
        }
        return FilterOperation::Transform;
    }
    
    return FilterOperation::Block;
}

FilterOperation CompositeFilter::ApplyMajorityRule(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo) {
    auto filters = GetEnabledFilters();
    
    if (filters.empty()) {
        return FilterOperation::Allow;
    }
    
    size_t allowCount = 0;
    size_t blockCount = 0;
    size_t transformCount = 0;
    
    LogCallbackInfo bestTransform = logInfo;
    bool hasTransform = false;
    
    for (const auto& filter : filters) {
        LogCallbackInfo tempInfo;
        FilterOperation result = filter->ApplyFilter(logInfo, &tempInfo);
        
        switch (result) {
        case FilterOperation::Allow:
            allowCount++;
            break;
        case FilterOperation::Block:
            blockCount++;
            break;
        case FilterOperation::Transform:
            transformCount++;
            bestTransform = tempInfo;
            hasTransform = true;
            break;
        }
    }
    
    size_t totalCount = filters.size();
    size_t majority = totalCount / 2 + 1;
    
    if (blockCount >= majority) {
        return FilterOperation::Block;
    }
    else if (allowCount >= majority) {
        return FilterOperation::Allow;
    }
    else if (hasTransform && transformCount > 0) {
        if (transformedInfo) {
            *transformedInfo = bestTransform;
        }
        return FilterOperation::Transform;
    }
    
    return FilterOperation::Allow; // Default to allow in case of tie
}

FilterOperation CompositeFilter::ApplyFirstMatch(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo) {
    auto filters = GetEnabledFilters();
    
    for (const auto& filter : filters) {
        FilterOperation result = filter->ApplyFilter(logInfo, transformedInfo);
        
        if (result != FilterOperation::Allow) {
            return result;
        }
    }
    
    return FilterOperation::Allow;
}

FilterOperation CompositeFilter::ApplyCustomLogic(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo) {
    if (!m_customLogic) {
        return ApplyAllMustPass(logInfo, transformedInfo);
    }
    
    auto filters = GetEnabledFilters();
    std::vector<FilterOperation> results;
    
    LogCallbackInfo currentInfo = logInfo;
    LogCallbackInfo tempInfo;
    
    for (const auto& filter : filters) {
        FilterOperation result = filter->ApplyFilter(currentInfo, &tempInfo);
        results.push_back(result);
        
        if (result == FilterOperation::Transform) {
            currentInfo = tempInfo;
        }
    }
    
    FilterOperation finalResult = m_customLogic(results);
    
    if (finalResult == FilterOperation::Transform && transformedInfo) {
        *transformedInfo = currentInfo;
    }
    
    return finalResult;
}

std::vector<std::shared_ptr<ILogFilter>> CompositeFilter::GetEnabledFilters() const {
    std::lock_guard<std::mutex> lock(m_filtersMutex);
    
    std::vector<std::shared_ptr<ILogFilter>> enabledFilters;
    
    for (const auto& filter : m_filters) {
        if (filter && filter->IsEnabled()) {
            enabledFilters.push_back(filter);
        }
    }
    
    SortFiltersByPriorityInternal(enabledFilters);
    
    return enabledFilters;
}

void CompositeFilter::SortFiltersByPriorityInternal(std::vector<std::shared_ptr<ILogFilter>>& filters) const {
    std::sort(filters.begin(), filters.end(),
        [](const std::shared_ptr<ILogFilter>& a, const std::shared_ptr<ILogFilter>& b) {
            return a->GetPriority() > b->GetPriority(); // Higher priority first
        });
}