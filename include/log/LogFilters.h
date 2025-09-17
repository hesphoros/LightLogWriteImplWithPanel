#pragma once

#include "ILogFilter.h"
#include <mutex>
#include <atomic>
#include <chrono>
#include <regex>
#include <set>
#include <vector>
#include <thread>

/**
 * @brief Base implementation for log filters providing common functionality
 */
class BaseLogFilter : public virtual ILogFilter {
protected:
	std::wstring                m_filterName;
	std::wstring                m_description;
	std::wstring                m_version;
	std::atomic<bool>           m_enabled;
	std::atomic<int>            m_priority;
	mutable std::mutex          m_statsMutex;
	mutable FilterStatistics    m_statistics;
	mutable std::mutex          m_configMutex;
	std::wstring                m_configuration;
	FilterContext               m_context;

public:
	explicit BaseLogFilter(const std::wstring& name, const std::wstring& description = L"", const std::wstring& version = L"1.0.0");
	virtual ~BaseLogFilter() = default;
	
	// ILogFilter implementation
	bool         IsEnabled() const override;
	void         SetEnabled(bool enabled) override;
	std::wstring GetFilterName() const override;
	
	bool         SetConfiguration(const std::wstring& config) override;
	std::wstring GetConfiguration() const override;
	bool         ValidateConfiguration(const std::wstring& config) const override;
	
	int          GetPriority() const override;
	void         SetPriority(int priority) override;
	
	FilterStatistics GetStatistics() const override;
	void             ResetStatistics() override;
	
	bool CanQuickReject(LogLevel level) const override;
	bool IsExpensive() const override;
	
	void SetContext(const FilterContext& context) override;
	FilterContext GetContext() const override;
	
	void Reset() override;
	
	std::wstring GetDescription() const override;
	std::wstring GetVersion() const override;
	
protected:
	// Helper methods for derived classes
	void         UpdateStatistics(FilterOperation operation, std::chrono::milliseconds processingTime) const;
	virtual bool DoValidateConfiguration(const std::wstring& config) const;
	virtual void DoReset();
};

/**
 * @brief Level-based log filter implementation
 */
class LevelFilter : public BaseLogFilter {
private:
	std::atomic<LogLevel> m_minLevel;
	std::atomic<LogLevel> m_maxLevel;
	bool                  m_hasMaxLevel;
	
public:
	explicit LevelFilter(LogLevel minLevel = LogLevel::Trace, LogLevel maxLevel = LogLevel::Fatal);
	
	FilterOperation ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo = nullptr) override;
	std::unique_ptr<ILogFilter> Clone() const override;
	bool CanQuickReject(LogLevel level) const override;
	
	// Level-specific methods
	void SetMinLevel(LogLevel level);
	LogLevel GetMinLevel() const;
	void SetMaxLevel(LogLevel level);
	LogLevel GetMaxLevel() const;
	void SetLevelRange(LogLevel minLevel, LogLevel maxLevel);
	void ClearMaxLevel();
	bool HasMaxLevel() const;
	
protected:
	bool DoValidateConfiguration(const std::wstring& config) const override;
	void DoReset() override;
};

/**
 * @brief Keyword-based log filter implementation
 */
class KeywordFilter : public BaseLogFilter {
private:
	std::vector<std::wstring>   m_includeKeywords;
	std::vector<std::wstring>   m_excludeKeywords;
	mutable std::mutex          m_keywordsMutex;
	bool                        m_caseSensitive;
	
public:
	explicit KeywordFilter(bool caseSensitive = false);
	
	FilterOperation ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo = nullptr) override;
	std::unique_ptr<ILogFilter> Clone() const override;
	
	// Keyword-specific methods
	void AddIncludeKeyword(const std::wstring& keyword);
	void AddExcludeKeyword(const std::wstring& keyword);
	void RemoveIncludeKeyword(const std::wstring& keyword);
	void RemoveExcludeKeyword(const std::wstring& keyword);
	void ClearIncludeKeywords();
	void ClearExcludeKeywords();
	void ClearAllKeywords();
	
	std::vector<std::wstring> GetIncludeKeywords() const;
	std::vector<std::wstring> GetExcludeKeywords() const;
	
	void SetCaseSensitive(bool sensitive);
	bool IsCaseSensitive() const;
	
protected:
	bool DoValidateConfiguration(const std::wstring& config) const override;
	void DoReset() override;
	
private:
	bool ContainsKeyword(const std::wstring& text, const std::wstring& keyword) const;
};

/**
 * @brief Regular expression-based log filter implementation
 */
class RegexFilter : public BaseLogFilter {
private:
	std::wregex          m_pattern;
	std::wstring         m_patternString;
	mutable std::mutex   m_regexMutex;
	bool                 m_patternValid;
	
public:
	explicit RegexFilter(const std::wstring& pattern = L"");
	
	FilterOperation ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo = nullptr) override;
	std::unique_ptr<ILogFilter> Clone() const override;
	bool IsExpensive() const override;
	
	// Regex-specific methods
	bool SetPattern(const std::wstring& pattern);
	std::wstring GetPattern() const;
	bool IsPatternValid() const;
	
protected:
	bool DoValidateConfiguration(const std::wstring& config) const override;
	void DoReset() override;
	
private:
	bool CompilePattern(const std::wstring& pattern);
};

/**
 * @brief Rate limiting filter implementation
 */
class RateLimitFilter : public BaseLogFilter {
private:
	std::atomic<size_t>                             m_maxPerSecond;
	std::atomic<size_t>                             m_maxBurst;
	mutable std::mutex                              m_rateMutex;
	mutable std::chrono::steady_clock::time_point   m_lastRefill;
	mutable size_t                                  m_tokens;
	
public:
	explicit RateLimitFilter(size_t maxPerSecond = 100, size_t maxBurst = 10);
	
	FilterOperation ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo = nullptr) override;
	std::unique_ptr<ILogFilter> Clone() const override;
	
	// Rate limit specific methods
	void    SetRateLimit(size_t maxPerSecond, size_t maxBurst);
	size_t  GetMaxPerSecond() const;
	size_t  GetMaxBurst() const;
	size_t  GetAvailableTokens() const;

protected:
	bool DoValidateConfiguration(const std::wstring& config) const override;
	void DoReset() override;
	
private:
	void RefillTokens() const;
};

/**
 * @brief Thread-based log filter implementation
 */
class ThreadFilter : public BaseLogFilter {
private:
	std::set<std::thread::id> m_allowedThreads;
	std::set<std::thread::id> m_blockedThreads;
	mutable std::mutex        m_threadsMutex;
	bool                      m_useAllowList;
	
public:
	explicit ThreadFilter(bool useAllowList = true);
	
	FilterOperation ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo = nullptr) override;
	std::unique_ptr<ILogFilter> Clone() const override;
	
	// Thread-specific methods
	void AddAllowedThread(std::thread::id threadId);
	void AddBlockedThread(std::thread::id threadId);
	void RemoveAllowedThread(std::thread::id threadId);
	void RemoveBlockedThread(std::thread::id threadId);
	void ClearAllowedThreads();
	void ClearBlockedThreads();
	
	void SetUseAllowList(bool useAllowList);
	bool IsUsingAllowList() const;
	
protected:
	bool DoValidateConfiguration(const std::wstring& config) const override;
	void DoReset() override;
};