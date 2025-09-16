#include "../../include/log/LogFilters.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <codecvt>

using namespace std::chrono;

// BaseLogFilter implementation
BaseLogFilter::BaseLogFilter(const std::wstring& name, const std::wstring& description, const std::wstring& version)
    : m_filterName(name)
    , m_description(description)
    , m_version(version)
    , m_enabled(true)
    , m_priority(static_cast<int>(FilterPriority::Normal))
{
    m_statistics.lastResetTime = system_clock::now();
}

bool BaseLogFilter::IsEnabled() const {
    return m_enabled.load();
}

void BaseLogFilter::SetEnabled(bool enabled) {
    m_enabled.store(enabled);
}

std::wstring BaseLogFilter::GetFilterName() const {
    return m_filterName;
}

bool BaseLogFilter::SetConfiguration(const std::wstring& config) {
    if (!ValidateConfiguration(config)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_configuration = config;
    return true;
}

std::wstring BaseLogFilter::GetConfiguration() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_configuration;
}

bool BaseLogFilter::ValidateConfiguration(const std::wstring& config) const {
    return DoValidateConfiguration(config);
}

int BaseLogFilter::GetPriority() const {
    return m_priority.load();
}

void BaseLogFilter::SetPriority(int priority) {
    m_priority.store(priority);
}

FilterStatistics BaseLogFilter::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_statistics;
}

void BaseLogFilter::ResetStatistics() {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_statistics = FilterStatistics{};
    m_statistics.lastResetTime = system_clock::now();
}

bool BaseLogFilter::CanQuickReject(LogLevel level) const {
    return false; // Base implementation doesn't optimize
}

bool BaseLogFilter::IsExpensive() const {
    return false; // Base implementation is not expensive
}

void BaseLogFilter::SetContext(const FilterContext& context) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_context = context;
}

FilterContext BaseLogFilter::GetContext() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_context;
}

void BaseLogFilter::Reset() {
    SetEnabled(true);
    SetPriority(static_cast<int>(FilterPriority::Normal));
    ResetStatistics();
    DoReset();
}

std::wstring BaseLogFilter::GetDescription() const {
    return m_description;
}

std::wstring BaseLogFilter::GetVersion() const {
    return m_version;
}

void BaseLogFilter::UpdateStatistics(FilterOperation operation, milliseconds processingTime) const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    
    m_statistics.totalProcessed++;
    m_statistics.totalProcessingTime += processingTime;
    
    if (m_statistics.totalProcessed > 0) {
        m_statistics.averageProcessingTime = 
            static_cast<double>(m_statistics.totalProcessingTime.count()) / m_statistics.totalProcessed;
    }
    
    switch (operation) {
    case FilterOperation::Allow:
        m_statistics.allowed++;
        break;
    case FilterOperation::Block:
        m_statistics.blocked++;
        break;
    case FilterOperation::Transform:
        m_statistics.transformed++;
        break;
    }
}

bool BaseLogFilter::DoValidateConfiguration(const std::wstring& config) const {
    return true; // Base implementation accepts any configuration
}

void BaseLogFilter::DoReset() {
    // Base implementation does nothing
}

// LevelFilter implementation
LevelFilter::LevelFilter(LogLevel minLevel, LogLevel maxLevel)
    : BaseLogFilter(L"LevelFilter", L"Filters logs based on log level", L"1.0.0")
    , m_minLevel(minLevel)
    , m_maxLevel(maxLevel)
    , m_hasMaxLevel(maxLevel != LogLevel::Fatal)
{
}

FilterOperation LevelFilter::ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo) {
    auto start = steady_clock::now();
    
    FilterOperation result = FilterOperation::Allow;
    
    LogLevel currentLevel = logInfo.level;
    LogLevel minLevel = m_minLevel.load();
    
    if (currentLevel < minLevel) {
        result = FilterOperation::Block;
    }
    else if (m_hasMaxLevel) {
        LogLevel maxLevel = m_maxLevel.load();
        if (currentLevel > maxLevel) {
            result = FilterOperation::Block;
        }
    }
    
    auto end = steady_clock::now();
    UpdateStatistics(result, duration_cast<milliseconds>(end - start));
    
    return result;
}

std::unique_ptr<ILogFilter> LevelFilter::Clone() const {
    auto clone = std::make_unique<LevelFilter>(m_minLevel.load(), m_maxLevel.load());
    clone->SetEnabled(IsEnabled());
    clone->SetPriority(GetPriority());
    clone->SetConfiguration(GetConfiguration());
    clone->SetContext(GetContext());
    return std::move(clone);
}

void LevelFilter::SetMinLevel(LogLevel level) {
    m_minLevel.store(level);
}

LogLevel LevelFilter::GetMinLevel() const {
    return m_minLevel.load();
}

void LevelFilter::SetMaxLevel(LogLevel level) {
    m_maxLevel.store(level);
    m_hasMaxLevel = true;
}

LogLevel LevelFilter::GetMaxLevel() const {
    return m_maxLevel.load();
}

void LevelFilter::SetLevelRange(LogLevel minLevel, LogLevel maxLevel) {
    m_minLevel.store(minLevel);
    m_maxLevel.store(maxLevel);
    m_hasMaxLevel = true;
}

void LevelFilter::ClearMaxLevel() {
    m_hasMaxLevel = false;
}

bool LevelFilter::HasMaxLevel() const {
    return m_hasMaxLevel;
}

bool LevelFilter::CanQuickReject(LogLevel level) const {
    return level < m_minLevel.load();
}

bool LevelFilter::DoValidateConfiguration(const std::wstring& config) const {
    // Simple validation: config should contain min level and optionally max level
    return !config.empty();
}

void LevelFilter::DoReset() {
    m_minLevel.store(LogLevel::Trace);
    m_maxLevel.store(LogLevel::Fatal);
    m_hasMaxLevel = false;
}

// KeywordFilter implementation
KeywordFilter::KeywordFilter(bool caseSensitive)
    : BaseLogFilter(L"KeywordFilter", L"Filters logs based on keywords", L"1.0.0")
    , m_caseSensitive(caseSensitive)
{
}

FilterOperation KeywordFilter::ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo) {
    auto start = steady_clock::now();
    
    FilterOperation result = FilterOperation::Allow;
    
    std::lock_guard<std::mutex> lock(m_keywordsMutex);
    
    const std::wstring& message = logInfo.message;
    
    // Check exclude keywords first (higher priority)
    for (const auto& keyword : m_excludeKeywords) {
        if (ContainsKeyword(message, keyword)) {
            result = FilterOperation::Block;
            break;
        }
    }
    
    // If not blocked and we have include keywords, check them
    if (result == FilterOperation::Allow && !m_includeKeywords.empty()) {
        bool found = false;
        for (const auto& keyword : m_includeKeywords) {
            if (ContainsKeyword(message, keyword)) {
                found = true;
                break;
            }
        }
        if (!found) {
            result = FilterOperation::Block;
        }
    }
    
    auto end = steady_clock::now();
    UpdateStatistics(result, duration_cast<milliseconds>(end - start));
    
    return result;
}

std::unique_ptr<ILogFilter> KeywordFilter::Clone() const {
    auto clone = std::make_unique<KeywordFilter>(m_caseSensitive);
    
    std::lock_guard<std::mutex> lock(m_keywordsMutex);
    clone->m_includeKeywords = m_includeKeywords;
    clone->m_excludeKeywords = m_excludeKeywords;
    
    clone->SetEnabled(IsEnabled());
    clone->SetPriority(GetPriority());
    clone->SetConfiguration(GetConfiguration());
    clone->SetContext(GetContext());
    
    return std::move(clone);
}

void KeywordFilter::AddIncludeKeyword(const std::wstring& keyword) {
    std::lock_guard<std::mutex> lock(m_keywordsMutex);
    m_includeKeywords.push_back(keyword);
}

void KeywordFilter::AddExcludeKeyword(const std::wstring& keyword) {
    std::lock_guard<std::mutex> lock(m_keywordsMutex);
    m_excludeKeywords.push_back(keyword);
}

void KeywordFilter::RemoveIncludeKeyword(const std::wstring& keyword) {
    std::lock_guard<std::mutex> lock(m_keywordsMutex);
    m_includeKeywords.erase(
        std::remove(m_includeKeywords.begin(), m_includeKeywords.end(), keyword),
        m_includeKeywords.end());
}

void KeywordFilter::RemoveExcludeKeyword(const std::wstring& keyword) {
    std::lock_guard<std::mutex> lock(m_keywordsMutex);
    m_excludeKeywords.erase(
        std::remove(m_excludeKeywords.begin(), m_excludeKeywords.end(), keyword),
        m_excludeKeywords.end());
}

void KeywordFilter::ClearIncludeKeywords() {
    std::lock_guard<std::mutex> lock(m_keywordsMutex);
    m_includeKeywords.clear();
}

void KeywordFilter::ClearExcludeKeywords() {
    std::lock_guard<std::mutex> lock(m_keywordsMutex);
    m_excludeKeywords.clear();
}

void KeywordFilter::ClearAllKeywords() {
    std::lock_guard<std::mutex> lock(m_keywordsMutex);
    m_includeKeywords.clear();
    m_excludeKeywords.clear();
}

std::vector<std::wstring> KeywordFilter::GetIncludeKeywords() const {
    std::lock_guard<std::mutex> lock(m_keywordsMutex);
    return m_includeKeywords;
}

std::vector<std::wstring> KeywordFilter::GetExcludeKeywords() const {
    std::lock_guard<std::mutex> lock(m_keywordsMutex);
    return m_excludeKeywords;
}

void KeywordFilter::SetCaseSensitive(bool sensitive) {
    m_caseSensitive = sensitive;
}

bool KeywordFilter::IsCaseSensitive() const {
    return m_caseSensitive;
}

bool KeywordFilter::DoValidateConfiguration(const std::wstring& config) const {
    return true; // Accept any configuration for now
}

void KeywordFilter::DoReset() {
    std::lock_guard<std::mutex> lock(m_keywordsMutex);
    m_includeKeywords.clear();
    m_excludeKeywords.clear();
    m_caseSensitive = false;
}

bool KeywordFilter::ContainsKeyword(const std::wstring& text, const std::wstring& keyword) const {
    if (m_caseSensitive) {
        return text.find(keyword) != std::wstring::npos;
    }
    else {
        std::wstring lowerText = text;
        std::wstring lowerKeyword = keyword;
        
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::towlower);
        std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::towlower);
        
        return lowerText.find(lowerKeyword) != std::wstring::npos;
    }
}

// RegexFilter implementation
RegexFilter::RegexFilter(const std::wstring& pattern)
    : BaseLogFilter(L"RegexFilter", L"Filters logs using regular expressions", L"1.0.0")
    , m_patternString(pattern)
    , m_patternValid(false)
{
    if (!pattern.empty()) {
        CompilePattern(pattern);
    }
}

FilterOperation RegexFilter::ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo) {
    auto start = steady_clock::now();
    
    FilterOperation result = FilterOperation::Allow;
    
    std::lock_guard<std::mutex> lock(m_regexMutex);
    
    if (m_patternValid) {
        try {
            if (!std::regex_search(logInfo.message, m_pattern)) {
                result = FilterOperation::Block;
            }
        }
        catch (const std::regex_error& e) {
            // On regex error, allow the message through
            result = FilterOperation::Allow;
        }
    }
    
    auto end = steady_clock::now();
    UpdateStatistics(result, duration_cast<milliseconds>(end - start));
    
    return result;
}

std::unique_ptr<ILogFilter> RegexFilter::Clone() const {
    std::lock_guard<std::mutex> lock(m_regexMutex);
    auto clone = std::make_unique<RegexFilter>(m_patternString);
    
    clone->SetEnabled(IsEnabled());
    clone->SetPriority(GetPriority());
    clone->SetConfiguration(GetConfiguration());
    clone->SetContext(GetContext());
    
    return std::move(clone);
}

bool RegexFilter::IsExpensive() const {
    return true; // Regex operations can be expensive
}

bool RegexFilter::SetPattern(const std::wstring& pattern) {
    return CompilePattern(pattern);
}

std::wstring RegexFilter::GetPattern() const {
    std::lock_guard<std::mutex> lock(m_regexMutex);
    return m_patternString;
}

bool RegexFilter::IsPatternValid() const {
    std::lock_guard<std::mutex> lock(m_regexMutex);
    return m_patternValid;
}

bool RegexFilter::DoValidateConfiguration(const std::wstring& config) const {
    if (config.empty()) return false;
    
    try {
        std::wregex test(config);
        return true;
    }
    catch (const std::regex_error&) {
        return false;
    }
}

void RegexFilter::DoReset() {
    std::lock_guard<std::mutex> lock(m_regexMutex);
    m_patternString.clear();
    m_patternValid = false;
}

bool RegexFilter::CompilePattern(const std::wstring& pattern) {
    std::lock_guard<std::mutex> lock(m_regexMutex);
    
    try {
        m_pattern = std::wregex(pattern, std::regex_constants::icase);
        m_patternString = pattern;
        m_patternValid = true;
        return true;
    }
    catch (const std::regex_error&) {
        m_patternValid = false;
        return false;
    }
}

// RateLimitFilter implementation
RateLimitFilter::RateLimitFilter(size_t maxPerSecond, size_t maxBurst)
    : BaseLogFilter(L"RateLimitFilter", L"Rate limiting filter using token bucket algorithm", L"1.0.0")
    , m_maxPerSecond(maxPerSecond)
    , m_maxBurst(maxBurst)
    , m_lastRefill(steady_clock::now())
    , m_tokens(maxBurst)
{
}

FilterOperation RateLimitFilter::ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo) {
    auto start = steady_clock::now();
    
    std::lock_guard<std::mutex> lock(m_rateMutex);
    
    RefillTokens();
    
    FilterOperation result = FilterOperation::Block;
    if (m_tokens > 0) {
        m_tokens--;
        result = FilterOperation::Allow;
    }
    
    auto end = steady_clock::now();
    UpdateStatistics(result, duration_cast<milliseconds>(end - start));
    
    return result;
}

std::unique_ptr<ILogFilter> RateLimitFilter::Clone() const {
    auto clone = std::make_unique<RateLimitFilter>(m_maxPerSecond.load(), m_maxBurst.load());
    
    clone->SetEnabled(IsEnabled());
    clone->SetPriority(GetPriority());
    clone->SetConfiguration(GetConfiguration());
    clone->SetContext(GetContext());
    
    return std::move(clone);
}

void RateLimitFilter::SetRateLimit(size_t maxPerSecond, size_t maxBurst) {
    std::lock_guard<std::mutex> lock(m_rateMutex);
    m_maxPerSecond.store(maxPerSecond);
    m_maxBurst.store(maxBurst);
    m_tokens = std::min(m_tokens, maxBurst);
}

size_t RateLimitFilter::GetMaxPerSecond() const {
    return m_maxPerSecond.load();
}

size_t RateLimitFilter::GetMaxBurst() const {
    return m_maxBurst.load();
}

size_t RateLimitFilter::GetAvailableTokens() const {
    std::lock_guard<std::mutex> lock(m_rateMutex);
    return m_tokens;
}

bool RateLimitFilter::DoValidateConfiguration(const std::wstring& config) const {
    return !config.empty();
}

void RateLimitFilter::DoReset() {
    std::lock_guard<std::mutex> lock(m_rateMutex);
    m_tokens = m_maxBurst.load();
    m_lastRefill = steady_clock::now();
}

void RateLimitFilter::RefillTokens() const {
    auto now = steady_clock::now();
    auto elapsed = duration_cast<milliseconds>(now - m_lastRefill);
    
    if (elapsed.count() >= 1000) { // At least 1 second passed
        size_t tokensToAdd = (elapsed.count() / 1000) * m_maxPerSecond.load();
        m_tokens = std::min(m_tokens + tokensToAdd, m_maxBurst.load());
        m_lastRefill = now;
    }
}

// ThreadFilter implementation
ThreadFilter::ThreadFilter(bool useAllowList)
    : BaseLogFilter(L"ThreadFilter", L"Filters logs based on thread ID", L"1.0.0")
    , m_useAllowList(useAllowList)
{
}

FilterOperation ThreadFilter::ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo) {
    auto start = steady_clock::now();
    
    FilterOperation result = FilterOperation::Allow;
    
    std::lock_guard<std::mutex> lock(m_threadsMutex);
    
    std::thread::id currentThreadId = logInfo.threadId;
    
    if (m_useAllowList) {
        // Allow list mode: only allow threads in the allow list
        if (!m_allowedThreads.empty() && 
            m_allowedThreads.find(currentThreadId) == m_allowedThreads.end()) {
            result = FilterOperation::Block;
        }
    }
    else {
        // Block list mode: block threads in the block list
        if (m_blockedThreads.find(currentThreadId) != m_blockedThreads.end()) {
            result = FilterOperation::Block;
        }
    }
    
    auto end = steady_clock::now();
    UpdateStatistics(result, duration_cast<milliseconds>(end - start));
    
    return result;
}

std::unique_ptr<ILogFilter> ThreadFilter::Clone() const {
    auto clone = std::make_unique<ThreadFilter>(m_useAllowList);
    
    std::lock_guard<std::mutex> lock(m_threadsMutex);
    clone->m_allowedThreads = m_allowedThreads;
    clone->m_blockedThreads = m_blockedThreads;
    
    clone->SetEnabled(IsEnabled());
    clone->SetPriority(GetPriority());
    clone->SetConfiguration(GetConfiguration());
    clone->SetContext(GetContext());
    
    return std::move(clone);
}

void ThreadFilter::AddAllowedThread(std::thread::id threadId) {
    std::lock_guard<std::mutex> lock(m_threadsMutex);
    m_allowedThreads.insert(threadId);
}

void ThreadFilter::AddBlockedThread(std::thread::id threadId) {
    std::lock_guard<std::mutex> lock(m_threadsMutex);
    m_blockedThreads.insert(threadId);
}

void ThreadFilter::RemoveAllowedThread(std::thread::id threadId) {
    std::lock_guard<std::mutex> lock(m_threadsMutex);
    m_allowedThreads.erase(threadId);
}

void ThreadFilter::RemoveBlockedThread(std::thread::id threadId) {
    std::lock_guard<std::mutex> lock(m_threadsMutex);
    m_blockedThreads.erase(threadId);
}

void ThreadFilter::ClearAllowedThreads() {
    std::lock_guard<std::mutex> lock(m_threadsMutex);
    m_allowedThreads.clear();
}

void ThreadFilter::ClearBlockedThreads() {
    std::lock_guard<std::mutex> lock(m_threadsMutex);
    m_blockedThreads.clear();
}

void ThreadFilter::SetUseAllowList(bool useAllowList) {
    m_useAllowList = useAllowList;
}

bool ThreadFilter::IsUsingAllowList() const {
    return m_useAllowList;
}

bool ThreadFilter::DoValidateConfiguration(const std::wstring& config) const {
    return true;
}

void ThreadFilter::DoReset() {
    std::lock_guard<std::mutex> lock(m_threadsMutex);
    m_allowedThreads.clear();
    m_blockedThreads.clear();
    m_useAllowList = true;
}