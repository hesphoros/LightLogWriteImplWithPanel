#include "../include/log/FilterManager.h"
#include "../include/log/LogFilters.h"
#include "../include/log/CompositeFilter.h"
#include <iostream>
#include <memory>

void DemonstrateFilterSystem() {
    std::wcout << L"=== Filter System Demonstration ===" << std::endl;
    
    // Create filter manager
    auto filterManager = std::make_unique<FilterManager>();
    
    // 1. Create and test basic filters
    std::wcout << L"\n1. Creating basic filters..." << std::endl;
    
    // Level filter: only allow Error and above
    auto levelFilter = filterManager->CreateFilter(L"LevelFilter");
    if (auto lf = std::dynamic_pointer_cast<LevelFilter>(std::shared_ptr<ILogFilter>(levelFilter.release()))) {
        lf->SetMinLevel(LogLevel::Error);
        std::wcout << L"Created Level Filter (Error+): " << lf->GetDescription() << std::endl;
    }
    
    // Keyword filter: block messages containing "test"
    auto keywordFilter = filterManager->CreateFilter(L"KeywordFilter");
    if (auto kf = std::dynamic_pointer_cast<KeywordFilter>(std::shared_ptr<ILogFilter>(keywordFilter.release()))) {
        kf->AddExcludeKeyword(L"test");
        kf->AddExcludeKeyword(L"debug");
        std::wcout << L"Created Keyword Filter (exclude test/debug): " << kf->GetDescription() << std::endl;
    }
    
    // Rate limit filter: 10 per second, burst of 5
    auto rateLimitFilter = filterManager->CreateFilter(L"RateLimitFilter");
    if (auto rlf = std::dynamic_pointer_cast<RateLimitFilter>(std::shared_ptr<ILogFilter>(rateLimitFilter.release()))) {
        rlf->SetRateLimit(10, 5);
        std::wcout << L"Created Rate Limit Filter (10/sec, burst 5): " << rlf->GetDescription() << std::endl;
    }
    
    // 2. Create composite filter
    std::wcout << L"\n2. Creating composite filter..." << std::endl;
    
    auto compositeFilter = filterManager->CreateCompositeFilter(CompositionStrategy::AllMustPass);
    
    // Add filters to composite
    auto levelFilter2 = filterManager->CreateFilter(L"LevelFilter");
    if (auto lf = std::dynamic_pointer_cast<LevelFilter>(std::shared_ptr<ILogFilter>(levelFilter2.release()))) {
        lf->SetMinLevel(LogLevel::Warning);
        lf->SetPriority(static_cast<int>(FilterPriority::High));
        compositeFilter->AddFilter(std::shared_ptr<ILogFilter>(lf));
    }
    
    auto keywordFilter2 = filterManager->CreateFilter(L"KeywordFilter");
    if (auto kf = std::dynamic_pointer_cast<KeywordFilter>(std::shared_ptr<ILogFilter>(keywordFilter2.release()))) {
        kf->AddIncludeKeyword(L"important");
        kf->AddIncludeKeyword(L"critical");
        kf->SetPriority(static_cast<int>(FilterPriority::Normal));
        compositeFilter->AddFilter(std::shared_ptr<ILogFilter>(kf));
    }
    
    compositeFilter->SortFiltersByPriority();
    
    std::wcout << L"Created Composite Filter with " << compositeFilter->GetFilterCount() 
               << L" sub-filters using AllMustPass strategy" << std::endl;
    
    // 3. Test filter performance
    std::wcout << L"\n3. Testing filter performance..." << std::endl;
    
    // Create test log info
    LogCallbackInfo testInfo;
    testInfo.level = LogLevel::Error;
    testInfo.message = L"This is an important error message";
    testInfo.timestamp = std::chrono::system_clock::now();
    testInfo.threadId = std::this_thread::get_id();
    
    // Test composite filter
    auto result = compositeFilter->ApplyFilter(testInfo);
    std::wcout << L"Filter result for error message: " << 
        (result == FilterOperation::Allow ? L"ALLOW" : 
         result == FilterOperation::Block ? L"BLOCK" : L"TRANSFORM") << std::endl;
    
    // Show statistics
    auto stats = compositeFilter->GetStatistics();
    std::wcout << L"Filter Statistics:" << std::endl;
    std::wcout << L"  Total processed: " << stats.totalProcessed << std::endl;
    std::wcout << L"  Allowed: " << stats.allowed << std::endl;
    std::wcout << L"  Blocked: " << stats.blocked << std::endl;
    std::wcout << L"  Transformed: " << stats.transformed << std::endl;
    std::wcout << L"  Average processing time: " << stats.averageProcessingTime << L"ms" << std::endl;
    
    // 4. Demonstrate configuration management
    std::wcout << L"\n4. Testing configuration management..." << std::endl;
    
    filterManager->SaveFilterConfiguration(L"MyCompositeFilter", compositeFilter);
    auto loadedFilter = filterManager->LoadFilterConfiguration(L"MyCompositeFilter");
    
    if (loadedFilter) {
        std::wcout << L"Successfully saved and loaded filter configuration: " 
                   << loadedFilter->GetFilterName() << std::endl;
    }
    
    // Show available filter types
    auto availableTypes = filterManager->GetAvailableFilterTypes();
    std::wcout << L"\nAvailable filter types: ";
    for (const auto& type : availableTypes) {
        std::wcout << type << L" ";
    }
    std::wcout << std::endl;
    
    // Show available templates
    auto templates = filterManager->GetAvailableTemplates();
    std::wcout << L"Available templates: ";
    for (const auto& tmpl : templates) {
        std::wcout << tmpl << L" ";
    }
    std::wcout << std::endl;
    
    // 5. Test template creation
    std::wcout << L"\n5. Testing template creation..." << std::endl;
    
    auto templateFilter = filterManager->CreateFromTemplate(L"ErrorOnly");
    if (templateFilter) {
        std::wcout << L"Created filter from ErrorOnly template: " 
                   << templateFilter->GetDescription() << std::endl;
    }
    
    std::wcout << L"\n=== Filter System Demonstration Complete ===" << std::endl;
}

int main() {
    try {
        DemonstrateFilterSystem();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}