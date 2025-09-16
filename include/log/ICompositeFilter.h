#pragma once

#include "ILogFilter.h"
#include <vector>
#include <functional>

/**
 * @brief Composition strategy for combining multiple filters
 */
enum class CompositionStrategy {
	AllMustPass,     // All filters must pass (AND logic)
	AnyCanPass,      // Any filter can pass (OR logic)
	MajorityRule,    // Majority of filters must pass
	FirstMatch,      // First matching filter determines result
	Custom           // Custom composition logic
};

/**
 * @brief Composite filter interface for combining multiple filters
 * @details This interface allows combining multiple filters with different strategies
 */
class ICompositeFilter : public virtual ILogFilter {
public:
	virtual ~ICompositeFilter() = default;
	
	// Filter chain management
	virtual void AddFilter(std::shared_ptr<ILogFilter> filter) = 0;
	virtual void RemoveFilter(const std::wstring& filterName) = 0;
	virtual void InsertFilter(size_t position, std::shared_ptr<ILogFilter> filter) = 0;
	virtual void ClearFilters() = 0;
	
	// Query and iteration
	virtual size_t GetFilterCount() const = 0;
	virtual std::shared_ptr<ILogFilter> GetFilter(size_t index) const = 0;
	virtual std::shared_ptr<ILogFilter> GetFilter(const std::wstring& name) const = 0;
	virtual std::vector<std::shared_ptr<ILogFilter>> GetAllFilters() const = 0;
	
	// Composition strategy
	virtual void SetCompositionStrategy(CompositionStrategy strategy) = 0;
	virtual CompositionStrategy GetCompositionStrategy() const = 0;
	
	// Custom composition logic
	virtual void SetCustomCompositionLogic(
		std::function<FilterOperation(const std::vector<FilterOperation>&)> logic) = 0;
	
	// Batch operations
	virtual void EnableFilter(const std::wstring& filterName, bool enabled) = 0;
	virtual void SetFilterPriority(const std::wstring& filterName, int priority) = 0;
	
	// Performance optimization
	virtual bool GetShortCircuitEnabled() const = 0;
	virtual void SetShortCircuitEnabled(bool enabled) = 0;
	
	// Ordering and sorting
	virtual void SortFiltersByPriority() = 0;
	virtual void MoveFilter(const std::wstring& filterName, size_t newPosition) = 0;
};

using CompositeFilterPtr = std::shared_ptr<ICompositeFilter>;