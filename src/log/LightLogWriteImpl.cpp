#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define NOMINMAX
#undef max
#undef min
#include <iostream>
#include <thread>
#include <chrono>
#include "UniConv/UniConv.h"
#include "log/UniConvAdapter.h"
#include "log/LightLogWriteImpl.h"
#include "log/LogCompressor.h"
#include "log/LogOutputManager.h"
#include "log/MultiOutputLogConfig.h"
#include "log/ConsoleLogOutput.h"
#include "log/BasicLogFormatter.h"
#include "log/RotationManagerFactory.h"
#include "log/DebugUtils.h"

const wchar_t* LightLogWrite_Impl::LOG_LEVEL_STRINGS_W[] = {
	L"[TRACE     ]",   // Trace
	L"[DEBUG     ]",   // Debug
	L"[INFO      ]",   // Info
	L"[NOTICE    ]",   // Notice
	L"[WARNING   ]",   // Warning
	L"[ERROR     ]",   // Error
	L"[CRITICAL  ]",   // Critical
	L"[ALERT     ]",   // Alert
	L"[EMERGENCY ]",   // Emergency
	L"[FATAL     ]"    // Fatal
};

const char* LightLogWrite_Impl::LOG_LEVEL_STRINGS_A[] = {
	"[TRACE     ]",   // Trace
	"[DEBUG     ]",   // Debug
	"[INFO      ]",   // Info
	"[NOTICE    ]",   // Notice
	"[WARNING   ]",   // Warning
	"[ERROR     ]",   // Error
	"[CRITICAL  ]",   // Critical
	"[ALERT     ]",   // Alert
	"[EMERGENCY ]",   // Emergency
	"[FATAL     ]"    // Fatal
};

LightLogWrite_Impl::LightLogWrite_Impl(size_t maxQueueSize, LogQueueOverflowStrategy strategy, size_t reportInterval, std::shared_ptr<ILogCompressor> compressor)
	: kMaxQueueSize(maxQueueSize),
	discardCount(0),
	lastReportedDiscardCount(0),
	bIsStopLogging{ false },
	queueFullStrategy(strategy),
	reportInterval(reportInterval),
	bHasLogLasting{ false },
	eMinLogLevel{ LogLevel::Trace },
	pNextCallbackHandle(1),
	multiOutputEnabled(false),
	logCompressor_(compressor),
	logFilter_(nullptr)
{
	// Initialize new rotation system with default async configuration
	LogRotationConfig defaultConfig;
	defaultConfig.strategy 				= LogRotationStrategy::None; // 日志轮转策略枚举
	defaultConfig.enableAsync 			= true; // 启用异步轮转
	defaultConfig.asyncWorkerCount 		= 2; // 异步工作线程
	defaultConfig.enablePreCheck 		= true;  // 启用预检
	defaultConfig.enableTransaction 	= true;  // 启用事务
	defaultConfig.enableStateMachine 	= true;  // 启用状态机
	rotationManager_ = RotationManagerFactory::CreateAsyncRotationManager(defaultConfig, logCompressor_);

	// Start the rotation manager
	if (rotationManager_) {
		rotationManager_->Start();
	}

	sWrittenThreads = std::thread(&LightLogWrite_Impl::RunWriteThread, this);

	// Initialize multi-output system
	multiOutputManager = std::make_shared<LogOutputManager>();
	multiOutputConfig  = std::make_unique<MultiOutputLogConfig>();
}

LightLogWrite_Impl::~LightLogWrite_Impl()
{
	CloseLogStream();

	// Cleanup rotation system
	if (rotationManager_) {
		try {
			// Stop the rotation manager
			rotationManager_->Stop();
			// Wait for any pending rotation tasks to complete
			rotationManager_->WaitForAllTasks(std::chrono::milliseconds(5000));
		}
		catch (const std::exception& e) {
			std::cerr << "[LogRotation] Exception during cleanup: " << e.what() << "\n";
		}
		rotationManager_.reset();
	}

	// Cleanup multi-output system
	if (multiOutputManager) {
		multiOutputManager->ShutdownAll();
	}
}

void LightLogWrite_Impl::SetLogsFileName(const std::wstring& sFilename)
{
	std::lock_guard<std::mutex> sWriteLock(pLogWriteMutex);
	std::lock_guard<std::mutex> streamLock(pFileStreamMutex);
	if (pLogFileStream.is_open())
		pLogFileStream.close();
	ChecksDirectory(sFilename);
	pLogFileStream.open(sFilename, std::ios::app);
}

void LightLogWrite_Impl::SetLogsFileName(const std::string& sFilename)
{
	std::wstring wFilename = UniConvAdapter::LocaleToWideString(sFilename);
	//Utf8ConvertsToUcs4(sFilename)
	SetLogsFileName(wFilename);
}

void LightLogWrite_Impl::SetLogsFileName(const std::u16string& sFilename)
{
	std::wstring wFilename = UniConvAdapter::U16StringToWString(sFilename);
	SetLogsFileName(wFilename);
}

void LightLogWrite_Impl::SetLastingsLogs(const std::wstring& sFilePath, const std::wstring& sBaseName)
{
	sLogLastingDir = sFilePath;
	sLogsBasedName = sBaseName;
	bHasLogLasting = true;
	CreateLogsFile();
}

void LightLogWrite_Impl::SetLastingsLogs(const std::u16string& sFilePath, const std::u16string& sBaseName)
{
	SetLastingsLogs(UniConvAdapter::U16StringToWString(sFilePath), UniConvAdapter::U16StringToWString(sBaseName));
}

void LightLogWrite_Impl::SetLastingsLogs(const std::string& sFilePath, const std::string& sBaseName)
{
	SetLastingsLogs(UniConvAdapter::LocaleToWideString(sFilePath), UniConvAdapter::LocaleToWideString(sBaseName));
}

void LightLogWrite_Impl::WriteLogContent(const std::wstring& sTypeVal, const std::wstring& sMessage)
{
	bool bNeedReport = false;
	size_t currentDiscard = 0;
	static std::atomic<bool> inErrorReport = false;

	if (queueFullStrategy == LogQueueOverflowStrategy::Block) {
		std::unique_lock<std::mutex> sWriteLock(pLogWriteMutex);
		pWrittenCondVar.wait(sWriteLock, [this] { return pLogWriteQueue.size() < kMaxQueueSize; });
		pLogWriteQueue.emplace(sTypeVal, sMessage);
	}
	else if (queueFullStrategy == LogQueueOverflowStrategy::DropOldest) {
		std::lock_guard<std::mutex> sWriteLock(pLogWriteMutex);
		if (pLogWriteQueue.size() >= kMaxQueueSize) {
			pLogWriteQueue.pop();
			++discardCount;
			if (discardCount - lastReportedDiscardCount >= reportInterval) {
				bNeedReport = true;
				currentDiscard = discardCount;
				lastReportedDiscardCount.store(discardCount.load());
			}
		}
		pLogWriteQueue.emplace(sTypeVal, sMessage);
	}
	pWrittenCondVar.notify_one();
	if (bNeedReport && !inErrorReport.exchange(true)) {
		std::wstring overflowMsg = L"The log queue overflows and has been discarded " + std::to_wstring(currentDiscard) + L" logs";
		// std::wcerr << L"[WriteLogContent] Report overflow: " << overflowMsg << std::endl;
		WriteLogContent(L"LOG_OVERFLOW", std::move(overflowMsg));
		inErrorReport = false;
	}
}

void LightLogWrite_Impl::WriteLogContent(std::wstring&& sTypeVal, std::wstring&& sMessage)
{
	bool bNeedReport = false;
	size_t currentDiscard = 0;
	static std::atomic<bool> inErrorReport = false;

	if (queueFullStrategy == LogQueueOverflowStrategy::Block) {
		std::unique_lock<std::mutex> sWriteLock(pLogWriteMutex);
		pWrittenCondVar.wait(sWriteLock, [this] { return pLogWriteQueue.size() < kMaxQueueSize; });
		pLogWriteQueue.emplace(std::move(sTypeVal), std::move(sMessage));
	}
	else if (queueFullStrategy == LogQueueOverflowStrategy::DropOldest) {
		std::lock_guard<std::mutex> sWriteLock(pLogWriteMutex);
		if (pLogWriteQueue.size() >= kMaxQueueSize) {
			pLogWriteQueue.pop();
			++discardCount;
			if (discardCount - lastReportedDiscardCount >= reportInterval) {
				bNeedReport = true;
				currentDiscard = discardCount;
				lastReportedDiscardCount.store(discardCount.load());
			}
		}
		pLogWriteQueue.emplace(std::move(sTypeVal), std::move(sMessage));
	}
	pWrittenCondVar.notify_one();
	if (bNeedReport && !inErrorReport.exchange(true)) {
		std::wstring overflowMsg = L"The log queue overflows and has been discarded " + std::to_wstring(currentDiscard) + L" logs";
		WriteLogContent(L"LOG_OVERFLOW", std::move(overflowMsg));  // Use move for the overflow message too
		inErrorReport = false;
	}
}

void LightLogWrite_Impl::WriteLogContent(const std::string& sTypeVal, const std::string& sMessage)
{
	// Convert once and reuse, then use move semantics to avoid additional copies
	std::wstring wTypeVal = UniConvAdapter::LocaleToWideString(sTypeVal);
	std::wstring wMessage = UniConvAdapter::LocaleToWideString(sMessage);
	WriteLogContent(std::move(wTypeVal), std::move(wMessage));
}

void LightLogWrite_Impl::WriteLogContent(const std::u16string& sTypeVal, const std::u16string& sMessage)
{
	// Convert once and reuse, then use move semantics to avoid additional copies
	std::wstring wTypeVal = UniConvAdapter::U16StringToWString(sTypeVal);
	std::wstring wMessage = UniConvAdapter::U16StringToWString(sMessage);
	WriteLogContent(std::move(wTypeVal), std::move(wMessage));
}

void LightLogWrite_Impl::WriteLogContent(std::string&& sTypeVal, std::string&& sMessage)
{
	// Convert once and reuse, then use move semantics to avoid additional copies
	std::wstring wTypeVal = UniConvAdapter::LocaleToWideString(sTypeVal);
	std::wstring wMessage = UniConvAdapter::LocaleToWideString(sMessage);
	WriteLogContent(std::move(wTypeVal), std::move(wMessage));
}

void LightLogWrite_Impl::WriteLogContent(std::u16string&& sTypeVal, std::u16string&& sMessage)
{
	// Convert once and reuse, then use move semantics to avoid additional copies
	std::wstring wTypeVal = UniConvAdapter::U16StringToWString(sTypeVal);
	std::wstring wMessage = UniConvAdapter::U16StringToWString(sMessage);
	WriteLogContent(std::move(wTypeVal), std::move(wMessage));
}

size_t LightLogWrite_Impl::GetDiscardCount() const
{
	return discardCount;
}

void LightLogWrite_Impl::ResetDiscardCount()
{
	discardCount = 0;
}

std::wstring LightLogWrite_Impl::BuildLogFileOut()
{
	std::tm sTmPartsInfo = GetCurrsTimerTm();
	std::wostringstream sWosStrStream;

	sWosStrStream << std::put_time(&sTmPartsInfo, L"%Y_%m_%d")
		<< (sTmPartsInfo.tm_hour < 12 ? L"_AM" : L"_PM") << L".log";

	bLastingTmTags = (sTmPartsInfo.tm_hour >= 12);

	std::filesystem::path sLotOutPaths = sLogLastingDir;
	std::filesystem::path sLogOutFiles = sLotOutPaths / (sLogsBasedName + sWosStrStream.str());

	return sLogOutFiles.wstring();
}

void LightLogWrite_Impl::CloseLogStream()
{
	WriteLogContent(L"<================================              Stop log write thread    ", L"================================>");
	bIsStopLogging = true;
	pWrittenCondVar.notify_all();
	if (sWrittenThreads.joinable())
		sWrittenThreads.join();
}

void LightLogWrite_Impl::CreateLogsFile()
{
	std::lock_guard<std::mutex> sLock(pLogWriteMutex);
	CreateLogsFileUnlocked();
}

void LightLogWrite_Impl::CreateLogsFileUnlocked()
{
	std::wstring sOutFileName = BuildLogFileOut();
	ChecksDirectory(sOutFileName);
	pLogFileStream.close();
	pLogFileStream.open(sOutFileName, std::ios::app);
	pLogFileStream.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));

	// 更新当前日志文件名以便轮转器使用
	currentLogFileName = sOutFileName;
}

void LightLogWrite_Impl::RunWriteThread()
{
	while (true) {
		LightLogWriteInfo sLogMessageInf;

		{
			auto sLock = std::unique_lock<std::mutex>(pLogWriteMutex);
			pWrittenCondVar.wait(sLock, [this]
				{ return !pLogWriteQueue.empty() || bIsStopLogging; });

			if (bIsStopLogging && pLogWriteQueue.empty())
				break;
			if (!pLogWriteQueue.empty()) {
				sLogMessageInf = pLogWriteQueue.front();
				pLogWriteQueue.pop();
				pWrittenCondVar.notify_one();
			}
		}

		{
			std::lock_guard<std::mutex> streamLock(pFileStreamMutex);

			if (bHasLogLasting)
				if (bLastingTmTags != (GetCurrsTimerTm().tm_hour > 12))
					CreateLogsFileUnlocked();

			CheckAndPerformRotation();
			if (!sLogMessageInf.sLogContentVal.empty() && pLogFileStream.is_open()) {
				pLogFileStream << sLogMessageInf.sLogTagNameVal << L"-//>>>" << GetCurrentTimer() << " : " << sLogMessageInf.sLogContentVal << "\n";
			}
		}
	}
	{
		std::lock_guard<std::mutex> streamLock(pFileStreamMutex);
		pLogFileStream.close();
	}
}

void LightLogWrite_Impl::ChecksDirectory(const std::wstring& sFilename)
{
	std::filesystem::path sFullFileName(sFilename);
	std::filesystem::path sOutFilesPath = sFullFileName.parent_path();
	if (!sOutFilesPath.empty() && !std::filesystem::exists(sOutFilesPath))
	{
		std::filesystem::create_directories(sOutFilesPath);
	}
}

std::wstring LightLogWrite_Impl::GetCurrentTimer() const
{
	std::tm sTmPartsInfo = GetCurrsTimerTm();
	std::wostringstream sWosStrStream;
	sWosStrStream << std::put_time(&sTmPartsInfo, L"%Y-%m-%d %H:%M:%S");
	return sWosStrStream.str();
}

std::tm LightLogWrite_Impl::GetCurrsTimerTm() const
{
	auto sCurrentTime = std::chrono::system_clock::now();
	std::time_t sCurrTimerTm = std::chrono::system_clock::to_time_t(sCurrentTime);
	std::tm sCurrTmDatas;
#ifdef _WIN32
	localtime_s(&sCurrTmDatas, &sCurrTimerTm);
#else
	localtime_r(&sCurrTmDatas, &sCurrTimerTm);
#endif
	return sCurrTmDatas;
}

std::string LightLogWrite_Impl::LogLevelToString(LogLevel level) const
{
	int index = static_cast<int>(level);
	if (index >= 0 && index < sizeof(LOG_LEVEL_STRINGS_A) / sizeof(LOG_LEVEL_STRINGS_A[0])) {
		return LOG_LEVEL_STRINGS_A[index];
	}
	return "[UNKNOWN   ]";
}

std::wstring LightLogWrite_Impl::LogLevelToWString(LogLevel level) const
{
	int index = static_cast<int>(level);
	if (index >= 0 && index < sizeof(LOG_LEVEL_STRINGS_W) / sizeof(LOG_LEVEL_STRINGS_W[0])) {
		return LOG_LEVEL_STRINGS_W[index];
	}
	return L"[UNKNOWN   ]";
}

void LightLogWrite_Impl::WriteLogContent(LogLevel level, const std::wstring& sMessage)
{
	if (level < eMinLogLevel) {
		return;
	}

	std::wstring levelStr = LogLevelToWString(level);

	// 应用过滤器检查（在触发回调之前）
	{
		std::lock_guard<std::mutex> filterLock(filterMutex_);
		if (logFilter_) {
			LogCallbackInfo filterInfo;
			filterInfo.level = level;
			filterInfo.levelString = levelStr;
			filterInfo.message = sMessage;
			filterInfo.timestamp = std::chrono::system_clock::now();
			filterInfo.threadId = std::this_thread::get_id();
			filterInfo.formattedTime = GetCurrentTimer();

			FilterOperation result = logFilter_->ApplyFilter(filterInfo, nullptr);
			if (result == FilterOperation::Block) {
				return; // 消息被过滤器阻止，直接返回
			}
		}
	}

	// 触发回调（在写入队列之前）
	TriggerLogCallbacks(level, levelStr, sMessage);

	// Write to multi-output system if enabled
	if (multiOutputEnabled.load() && multiOutputManager) {
		LIGHTLOG_DEBUG_MULTIOUTPUT_INFO(L"Multi-output enabled - routing to output manager");
		LogCallbackInfo logInfo;
		logInfo.level = level;
		logInfo.levelString = levelStr;
		logInfo.message = sMessage;
		logInfo.timestamp = std::chrono::system_clock::now();
		logInfo.threadId = std::this_thread::get_id();

		try {
			multiOutputManager->WriteLog(logInfo);
			LIGHTLOG_DEBUG_MULTIOUTPUT_VERBOSE(L"Multi-output WriteLog completed");
		}
		catch (...) {
			LIGHTLOG_DEBUG_MULTIOUTPUT_ERROR(L"Multi-output WriteLog failed with exception");
			// Continue with normal logging even if multi-output fails
		}
	} else {
		LIGHTLOG_DEBUG_STREAM(DEBUG_LEVEL_VERBOSE, MultiOutput) 
			<< L"Multi-output not enabled or manager is null - multiOutputEnabled=" 
			<< (multiOutputEnabled.load() ? L"true" : L"false") 
			<< L", multiOutputManager=" << (multiOutputManager ? L"valid" : L"null") 
			LIGHTLOG_DEBUG_STREAM_END(VERBOSE, MultiOutput);
	}

	bool bNeedReport = false;
	size_t currentDiscard = 0;
	static std::atomic<bool> inErrorReport = false;

	if (queueFullStrategy == LogQueueOverflowStrategy::Block) {
		std::unique_lock<std::mutex> sWriteLock(pLogWriteMutex);
		pWrittenCondVar.wait(sWriteLock, [this] { return pLogWriteQueue.size() < kMaxQueueSize; });
		pLogWriteQueue.emplace(std::move(levelStr), sMessage);
	}
	else if (queueFullStrategy == LogQueueOverflowStrategy::DropOldest) {
		std::lock_guard<std::mutex> sWriteLock(pLogWriteMutex);
		if (pLogWriteQueue.size() >= kMaxQueueSize) {
			pLogWriteQueue.pop();
			++discardCount;
			if (discardCount - lastReportedDiscardCount >= reportInterval) {
				bNeedReport = true;
				currentDiscard = discardCount;
				lastReportedDiscardCount.store(discardCount.load());
			}
		}
		pLogWriteQueue.emplace(std::move(levelStr), sMessage);
	}
	pWrittenCondVar.notify_one();

	if (bNeedReport && !inErrorReport.exchange(true)) {
		std::wstring overflowMsg = L"The log queue overflows and has been discarded " + std::to_wstring(currentDiscard) + L" logs";
		WriteLogContent(LogLevel::Warning, std::move(overflowMsg));
		inErrorReport = false;
	}
}

void LightLogWrite_Impl::WriteLogContent(LogLevel level, std::wstring&& sMessage)
{
	if (level < eMinLogLevel) {
		return;
	}

	std::wstring levelStr = LogLevelToWString(level);

	// 应用过滤器检查（在触发回调之前）
	{
		std::lock_guard<std::mutex> filterLock(filterMutex_);
		if (logFilter_) {
			LogCallbackInfo filterInfo;
			filterInfo.level = level;
			filterInfo.levelString = levelStr;
			filterInfo.message = sMessage; // 这里我们需要复制消息用于过滤器检�?
			filterInfo.timestamp = std::chrono::system_clock::now();
			filterInfo.threadId = std::this_thread::get_id();
			filterInfo.formattedTime = GetCurrentTimer();

			FilterOperation result = logFilter_->ApplyFilter(filterInfo, nullptr);
			if (result == FilterOperation::Block) {
				return; // 消息被过滤器阻止，直接返回
			}
		}
	}

	// 触发回调（在写入队列之前）
	TriggerLogCallbacks(level, levelStr, sMessage);

	// Write to multi-output system if enabled
	if (multiOutputEnabled.load() && multiOutputManager) {
		LogCallbackInfo logInfo;
		logInfo.level = level;
		logInfo.levelString = levelStr;
		logInfo.message = sMessage;  // Copy for callback, original will be moved later
		logInfo.timestamp = std::chrono::system_clock::now();
		logInfo.threadId = std::this_thread::get_id();

		try {
			multiOutputManager->WriteLog(logInfo);
		}
		catch (...) {
			// Continue with normal logging even if multi-output fails
		}
	}

	bool bNeedReport = false;
	size_t currentDiscard = 0;
	static std::atomic<bool> inErrorReport = false;

	if (queueFullStrategy == LogQueueOverflowStrategy::Block) {
		std::unique_lock<std::mutex> sWriteLock(pLogWriteMutex);
		pWrittenCondVar.wait(sWriteLock, [this] { return pLogWriteQueue.size() < kMaxQueueSize; });
		pLogWriteQueue.emplace(std::move(levelStr), std::move(sMessage));
	}
	else if (queueFullStrategy == LogQueueOverflowStrategy::DropOldest) {
		std::lock_guard<std::mutex> sWriteLock(pLogWriteMutex);
		if (pLogWriteQueue.size() >= kMaxQueueSize) {
			pLogWriteQueue.pop();
			++discardCount;
			if (discardCount - lastReportedDiscardCount >= reportInterval) {
				bNeedReport = true;
				currentDiscard = discardCount;
				lastReportedDiscardCount.store(discardCount.load());
			}
		}
		pLogWriteQueue.emplace(std::move(levelStr), std::move(sMessage));
	}
	pWrittenCondVar.notify_one();

	if (bNeedReport && !inErrorReport.exchange(true)) {
		std::wstring overflowMsg = L"The log queue overflows and has been discarded " + std::to_wstring(currentDiscard) + L" logs";
		WriteLogContent(LogLevel::Warning, std::move(overflowMsg));
		inErrorReport = false;
	}
}

void LightLogWrite_Impl::WriteLogContent(LogLevel level, const std::string& sMessage)
{
	// Convert to wstring and use move version for efficiency
	std::wstring wMessage = UniConvAdapter::LocaleToWideString(sMessage);
	WriteLogContent(level, std::move(wMessage));
}

// 宏定义用于简化重复的日志函数实现
#define IMPLEMENT_LOG_FUNCTIONS(FuncName, Level) \
	void LightLogWrite_Impl::WriteLog##FuncName(const std::wstring& sMessage) \
	{ \
		WriteLogContent(LogLevel::Level, sMessage); \
	} \
	void LightLogWrite_Impl::WriteLog##FuncName(const std::string& sMessage) \
	{ \
		WriteLogContent(LogLevel::Level, sMessage); \
	}

IMPLEMENT_LOG_FUNCTIONS(Trace, Trace)
IMPLEMENT_LOG_FUNCTIONS(Debug, Debug)
IMPLEMENT_LOG_FUNCTIONS(Info, Info)
IMPLEMENT_LOG_FUNCTIONS(Notice, Notice)
IMPLEMENT_LOG_FUNCTIONS(Warning, Warning)
IMPLEMENT_LOG_FUNCTIONS(Error, Error)
IMPLEMENT_LOG_FUNCTIONS(Critical, Critical)
IMPLEMENT_LOG_FUNCTIONS(Alert, Alert)
IMPLEMENT_LOG_FUNCTIONS(Emergency, Emergency)
IMPLEMENT_LOG_FUNCTIONS(Fatal, Fatal)

#undef IMPLEMENT_LOG_FUNCTIONS

// Callback management implementation

CallbackHandle LightLogWrite_Impl::SubscribeToLogEvents(const LogCallback& callback, LogLevel minLevel)
{
	std::lock_guard<std::mutex> lock(pCallbackMutex);
	CallbackHandle handle = pNextCallbackHandle++;
	pLogCallbacks.push_back({ handle, callback, minLevel });
	return handle;
}

bool LightLogWrite_Impl::UnsubscribeFromLogEvents(CallbackHandle handle)
{
	std::lock_guard<std::mutex> lock(pCallbackMutex);
	auto it = std::find_if(pLogCallbacks.begin(), pLogCallbacks.end(),
		[handle](const CallbackEntry& entry) { return entry.handle == handle; });

	if (it != pLogCallbacks.end()) {
		pLogCallbacks.erase(it);
		return true;
	}
	return false;
}

void LightLogWrite_Impl::ClearAllLogCallbacks()
{
	std::lock_guard<std::mutex> lock(pCallbackMutex);
	pLogCallbacks.clear();
}

size_t LightLogWrite_Impl::GetCallbackCount() const
{
	std::lock_guard<std::mutex> lock(pCallbackMutex);
	return pLogCallbacks.size();
}

void LightLogWrite_Impl::TriggerLogCallbacks(LogLevel level, const std::wstring& levelString, const std::wstring& message)
{
	std::lock_guard<std::mutex> lock(pCallbackMutex);

	if (pLogCallbacks.empty()) {
		return;
	}

	// 创建回调信息
	LogCallbackInfo callbackInfo;
	callbackInfo.level = level;
	callbackInfo.levelString = levelString;
	callbackInfo.message = message;
	callbackInfo.timestamp = std::chrono::system_clock::now();
	callbackInfo.formattedTime = GetCurrentTimer();

	// 调用所有符合条件的回调
	for (const auto& entry : pLogCallbacks) {
		if (level >= entry.minLevel) {
			try {
				entry.callback(callbackInfo);
			}
			catch (...) {
				// 忽略回调函数中的异常，避免影响日志系统的正常工作
				// 在实际应用中，可以考虑记录这些异常到错误日志
			}
		}
	}
}

// Log rotation implementation

void LightLogWrite_Impl::SetLogRotationConfig(const LogRotationConfig& config)
{
	std::lock_guard<std::mutex> lock(rotationMutex);

	if (rotationManager_) {
		// Update the rotation manager with new configuration
		LogRotationConfig updatedConfig = config;

		// 如果设置了归档目录但为空，则使用日志目录
		if (updatedConfig.archiveDirectory.empty()) {
			updatedConfig.archiveDirectory = sLogLastingDir;
		}

		rotationManager_->SetConfig(updatedConfig);
	}
}

LogRotationConfig LightLogWrite_Impl::GetLogRotationConfig() const
{
	std::lock_guard<std::mutex> lock(rotationMutex);

	if (rotationManager_) {
		return rotationManager_->GetConfig();
	}

	// Return default config if no rotation manager
	LogRotationConfig defaultConfig;
	defaultConfig.strategy = LogRotationStrategy::None;
	return defaultConfig;
}

void LightLogWrite_Impl::ForceLogRotation()
{
	if (!rotationManager_) {
		return;
	}

	try {
		std::lock_guard<std::mutex> streamLock(pFileStreamMutex);

		if (pLogFileStream.is_open()) {
			pLogFileStream.flush();
			pLogFileStream.close();
		}
		
		RotationTrigger trigger;
		trigger.manualRequested = true;
		trigger.reason = L"Manual rotation requested";
		
		auto rotationFuture = rotationManager_->PerformRotationAsync(currentLogFileName, trigger);
		
		RotationResult result;
		if (rotationFuture.wait_for(std::chrono::seconds(30)) == std::future_status::ready) {
			result = rotationFuture.get();
			
			if (result.success && !result.newFileName.empty()) {
				currentLogFileName = result.newFileName;
			}
		}
		
		ReopenLogFileStream();
	}
	catch (const std::exception& e) {
		std::cerr << "[LogRotation] Exception in ForceLogRotation: " << e.what() << "\n";
	}
	
	std::cout << "[LogRotation] Force rotation process completed\n";
}

std::future<bool> LightLogWrite_Impl::ForceLogRotationAsync()
{
	if (!rotationManager_) {
		std::promise<bool> promise;
		promise.set_value(false);
		return promise.get_future();
	}

	return std::async(std::launch::async, [this]() -> bool {
		try {
			ForceLogRotation();
			std::lock_guard<std::mutex> streamLock(pFileStreamMutex);
			return pLogFileStream.is_open();
		}
		catch (const std::exception& e) {
			std::cerr << "[LogRotation] Exception in ForceLogRotationAsync: " << e.what() << "\n";
			return false;
		}
		catch (...) {
			return false;
		}
	});
}

size_t LightLogWrite_Impl::GetPendingRotationTasks() const
{
	if (!rotationManager_) {
		return 0;
	}

	try {
		return rotationManager_->GetPendingTaskCount();
	}
	catch (const std::exception& e) {
		std::cerr << "[LogRotation] Exception in GetPendingRotationTasks: " << e.what() << "\n";
		return 0;
	}
}

size_t LightLogWrite_Impl::CancelPendingRotationTasks()
{
	if (!rotationManager_) {
		return 0;
	}

	try {
		size_t cancelled = rotationManager_->CancelPendingTasks();
		return cancelled;
	}
	catch (const std::exception& e) {
		std::cerr << "[LogRotation] Exception in CancelPendingRotationTasks: " << e.what() << "\n";
		return 0;
	}
}

size_t LightLogWrite_Impl::GetCurrentLogFileSize() const
{
	std::lock_guard<std::mutex> lock(pLogWriteMutex);
	return GetCurrentLogFileSizeUnlocked();
}

size_t LightLogWrite_Impl::GetCurrentLogFileSizeUnlocked() const
{
	if (!currentLogFileName.empty() && std::filesystem::exists(currentLogFileName)) {
		try {
			return static_cast<size_t>(std::filesystem::file_size(currentLogFileName));
		}
		catch (...) {
			return 0;
		}
	}
	return 0;
}

void LightLogWrite_Impl::ReopenLogFileStream()
{
	if (pLogFileStream.is_open()) {
		return;
	}

	pLogFileStream.clear();
	pLogFileStream.open(currentLogFileName.c_str(), std::ios::app);
	if (pLogFileStream.is_open()) {
		pLogFileStream.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));
	}
}

void LightLogWrite_Impl::CheckAndPerformRotation()
{
	if (!rotationManager_) {
		return;
	}

	try {
		if (pLogFileStream.is_open()) {
			pLogFileStream.flush();
		}

		size_t currentSize = GetCurrentLogFileSizeUnlocked();
		auto trigger = rotationManager_->CheckRotationNeeded(currentLogFileName, currentSize);

		if (trigger.sizeExceeded || trigger.timeReached || trigger.manualRequested) {
			if (pLogFileStream.is_open()) {
				pLogFileStream.close();
			}
			
			auto rotationFuture = rotationManager_->PerformRotationAsync(currentLogFileName, trigger);

			RotationResult result;
			if (rotationFuture.wait_for(std::chrono::seconds(30)) == std::future_status::ready) {
				result = rotationFuture.get();
			} else {
				result.success = false;
				result.errorMessage = "Rotation operation timed out after 30 seconds";
			}

			if (result.success) {
				if (!result.newFileName.empty()) {
					currentLogFileName = result.newFileName;
				}
			}
			else {
				std::cerr << "[LogRotation] Rotation failed: " << result.errorMessage << "\n";
			}
			
			ReopenLogFileStream();
		}
	}
	catch (const std::exception& e) {
		std::cerr << "[LogRotation] Exception in CheckAndPerformRotation: " << e.what() << "\n";
	}
	catch (...) {
		std::cerr << "[LogRotation] Unknown exception in CheckAndPerformRotation\n";
	}
}



std::wstring LightLogWrite_Impl::GenerateArchiveFileName(const std::wstring& baseFileName, const std::chrono::system_clock::time_point& timestamp)
{
	std::filesystem::path basePath(baseFileName);
	std::wstring baseName = basePath.stem().wstring();

	// 格式化时间戳
	auto timeT = std::chrono::system_clock::to_time_t(timestamp);
	std::tm tmInfo;
#ifdef _WIN32
	localtime_s(&tmInfo, &timeT);
#else
	localtime_r(&timeT, &tmInfo);
#endif

	std::wostringstream oss;
	oss << std::put_time(&tmInfo, L"%Y%m%d_%H%M%S");

	// Get configuration from rotation manager if available
	std::wstring archiveDir = L"./logs";
	std::wstring extension = L".log";
	bool enableCompression = false;

	if (rotationManager_) {
		auto config = rotationManager_->GetConfig();
		archiveDir = config.archiveDirectory;
		extension = config.enableCompression ? L".zip" : L".log";
		enableCompression = config.enableCompression;
	}

	// 构建归档文件名
	std::filesystem::path archivePath(archiveDir);
	std::wstring archiveFileName = baseName + L"_" + oss.str() + extension;

	return (archivePath / archiveFileName).wstring();
}





// Multi-output system implementation
bool LightLogWrite_Impl::SaveMultiOutputConfigToJson(const std::wstring& configFilePath) {
	std::lock_guard<std::mutex> lock(multiOutputMutex);
	if (multiOutputConfig) {
		return MultiOutputConfigSerializer::SaveToFile(*multiOutputConfig, configFilePath);
	}
	return false;
}

bool LightLogWrite_Impl::LoadMultiOutputConfigFromJson(const std::wstring& configFilePath) {
	std::lock_guard<std::mutex> lock(multiOutputMutex);
	if (multiOutputConfig && MultiOutputConfigSerializer::LoadFromFile(configFilePath, *multiOutputConfig)) {
		// Apply loaded configuration
		if (multiOutputManager) {
			multiOutputManager->SetConfig(multiOutputConfig->managerConfig);
		}
		return true;
	}
	return false;
}

bool LightLogWrite_Impl::AddLogOutput(std::shared_ptr<ILogOutput> output) {
	if (!multiOutputManager || !output) {
		return false;
	}

	std::lock_guard<std::mutex> lock(multiOutputMutex);
	return multiOutputManager->AddOutput(output);
}

bool LightLogWrite_Impl::RemoveLogOutput(const std::wstring& outputName) {
	if (!multiOutputManager) {
		return false;
	}

	std::lock_guard<std::mutex> lock(multiOutputMutex);
	return multiOutputManager->RemoveOutput(outputName);
}

std::shared_ptr<LogOutputManager> LightLogWrite_Impl::GetOutputManager() const {
	return multiOutputManager;
}

void LightLogWrite_Impl::SetMultiOutputEnabled(bool enabled) {
	multiOutputEnabled = enabled;
}

bool LightLogWrite_Impl::IsMultiOutputEnabled() const {
	return multiOutputEnabled.load();
}

// Compression management methods
void LightLogWrite_Impl::SetCompressor(std::shared_ptr<ILogCompressor> compressor) {
	logCompressor_ = compressor;
}

std::shared_ptr<ILogCompressor> LightLogWrite_Impl::GetCompressor() const {
	return logCompressor_;
}

CompressionStatistics LightLogWrite_Impl::GetCompressionStatistics() const {
	if (logCompressor_) {
		auto statisticalCompressor = std::dynamic_pointer_cast<IStatisticalLogCompressor>(logCompressor_);
		if (statisticalCompressor) {
			return statisticalCompressor->GetStatistics();
		}
	}
	// Return empty statistics if no compressor or no statistical interface
	return CompressionStatistics{};
}

// ==================== Filter System Implementation ====================

void LightLogWrite_Impl::SetLogFilter(std::shared_ptr<ILogFilter> filter) {
	std::lock_guard<std::mutex> lock(filterMutex_);
	logFilter_ = filter;
}

std::shared_ptr<ILogFilter> LightLogWrite_Impl::GetLogFilter() const {
	std::lock_guard<std::mutex> lock(filterMutex_);
	return logFilter_;
}

void LightLogWrite_Impl::ClearLogFilter() {
	std::lock_guard<std::mutex> lock(filterMutex_);
	logFilter_.reset();
}

bool LightLogWrite_Impl::HasLogFilter() const {
	std::lock_guard<std::mutex> lock(filterMutex_);
	return logFilter_ != nullptr;
}
