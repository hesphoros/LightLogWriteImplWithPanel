#pragma once

#include "LightLogWriteImpl.h"
#include <memory>
#include <string>

enum class FilterOperation {
    Allow,
    Block,
    Transform
};

class ILogFilter {
public:
    virtual ~ILogFilter() = default;
    virtual FilterOperation ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo = nullptr) = 0;
    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;
    virtual std::wstring GetFilterName() const = 0;
};

using LogFilterPtr = std::shared_ptr<ILogFilter>;
