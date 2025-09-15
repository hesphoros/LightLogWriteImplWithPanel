#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define NOMINMAX
#include <iostream>
#include "UniConv.h"
#include "LightLogWriteImpl.h"
#include "../../include/miniz/zip_file.hpp"
#include "../../include/log/LogOutputManager.h"
#include "../../include/log/MultiOutputLogConfig.h"
#include "../../include/log/ConsoleLogOutput.h"
#include "../../include/log/BasicLogFormatter.h"

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

LightLogWrite_Impl::LightLogWrite_Impl(size_t maxQueueSize, LogQueueOverflowStrategy strategy, size_t reportInterval)
  :	kMaxQueueSize(maxQueueSize),
	discardCount(0),
	lastReportedDiscardCount(0),
	bIsStopLogging{ false },
	queueFullStrategy(strategy),
	reportInterval(reportInterval),
	bHasLogLasting{ false },
	eMinLogLevel{ LogLevel::Trace },
	pNextCallbackHandle(1),
	lastRotationTime(std::chrono::system_clock::now()),
	stopCompressionWorker(false),
	forceRotationRequested(false),
	multiOutputEnabled(false)
{
	sWrittenThreads = std::thread(&LightLogWrite_Impl::RunWriteThread, this);
	StartCompressionWorker();
	
	// Initialize multi-output system
	multiOutputManager = std::make_shared<LogOutputManager>();
	multiOutputConfig = std::make_unique<MultiOutputLogConfig>();
}

LightLogWrite_Impl::~LightLogWrite_Impl()
{
	StopCompressionWorker();
	CloseLogStream();
	
	// Cleanup multi-output system
	if (multiOutputManager) {
		multiOutputManager->ShutdownAll();
	}
}

void LightLogWrite_Impl::SetLogsFileName(const std::wstring& sFilename)
{
	std::lock_guard<std::mutex> sWriteLock(pLogWriteMutex);
	if (pLogFileStream.is_open())
		pLogFileStream.close();
	ChecksDirectory(sFilename); 
	pLogFileStream.open(sFilename, std::ios::app);
}

void LightLogWrite_Impl::SetLogsFileName(const std::string& sFilename)
{
	std::wstring wFilename = UniConv::GetInstance()->LocaleToWideString(sFilename);
	//Utf8ConvertsToUcs4(sFilename)
	SetLogsFileName(wFilename);
}

void LightLogWrite_Impl::SetLogsFileName(const std::u16string& sFilename)
{
	std::wstring wFilename = UniConv::GetInstance()->U16StringToWString(sFilename);
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
	SetLastingsLogs(UniConv::GetInstance()->U16StringToWString(sFilePath), UniConv::GetInstance()->U16StringToWString(sBaseName));
}

void LightLogWrite_Impl::SetLastingsLogs(const std::string& sFilePath, const std::string& sBaseName)
{
	SetLastingsLogs(UniConv::GetInstance()->LocaleToWideString(sFilePath), UniConv::GetInstance()->LocaleToWideString(sBaseName));
}

void LightLogWrite_Impl::WriteLogContent(const std::wstring& sTypeVal, const std::wstring& sMessage)
{


	bool bNeedReport = false;
	size_t currentDiscard = 0;
	static std::atomic<bool> inErrorReport = false;

	if (queueFullStrategy == LogQueueOverflowStrategy::Block) {
		std::unique_lock<std::mutex> sWriteLock(pLogWriteMutex);
		// std::wcerr << L"[WriteLogContent] Try push (Block), queue size: " <<
		// pLogWriteQueue.size() << std::endl;
		pWrittenCondVar.wait(sWriteLock, [this] { return pLogWriteQueue.size() < kMaxQueueSize; });
		pLogWriteQueue.push({ sTypeVal, sMessage });
		// std::wcerr << L"[WriteLogContent] Pushed (Block), queue size: " <<
		// pLogWriteQueue.size() << std::endl;
	}
	else if ( queueFullStrategy == LogQueueOverflowStrategy::DropOldest ) {
		std::lock_guard<std::mutex> sWriteLock(pLogWriteMutex);
		// std::wcerr << L"[WriteLogContent] Try push (DropOldest), queue size: " <<
		// pLogWriteQueue.size() << std::endl;
		if (pLogWriteQueue.size() >= kMaxQueueSize) {
			// std::wcerr << L"[WriteLogContent] Drop oldest, queue full: " <<
			// pLogWriteQueue.size() << std::endl;
			pLogWriteQueue.pop();
			++discardCount;
			if (discardCount - lastReportedDiscardCount >= reportInterval) {
				bNeedReport = true;
				currentDiscard = discardCount;
				lastReportedDiscardCount.store(discardCount.load());
			}
		}
		pLogWriteQueue.push({ sTypeVal, sMessage });
		// std::wcout << L"[WriteLogContent] Pushed (DropOldest), queue size: " <<
		// pLogWriteQueue.size() << std::endl;
	}
	pWrittenCondVar.notify_one();
	if (bNeedReport && !inErrorReport.exchange(true)) {
		std::wstring overflowMsg = L"The log queue overflows and has been discarded " + std::to_wstring(currentDiscard) + L" logs";
		// std::wcerr << L"[WriteLogContent] Report overflow: " << overflowMsg << std::endl;
		WriteLogContent(L"LOG_OVERFLOW", overflowMsg);
		inErrorReport = false;
	}
}

void LightLogWrite_Impl::WriteLogContent(const std::string& sTypeVal, const std::string& sMessage)
{
    std::wstring wContent = UniConv::GetInstance()->LocaleToWideString(sMessage);
	WriteLogContent(UniConv::GetInstance()->LocaleToWideString(sTypeVal), UniConv::GetInstance()->LocaleToWideString(sMessage));
}

void LightLogWrite_Impl::WriteLogContent(const std::u16string& sTypeVal, const std::u16string& sMessage)
{
	WriteLogContent(UniConv::GetInstance()->U16StringToWString(sTypeVal), UniConv::GetInstance()->U16StringToWString(sMessage));
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
	
	// 更新当前日志文件名以便轮转使用
	currentLogFileName = sOutFileName;
}


void LightLogWrite_Impl::RunWriteThread()
{
	while (true) {
		if (bHasLogLasting)
			if (bLastingTmTags != (GetCurrsTimerTm().tm_hour > 12))
				CreateLogsFile();
		
		LightLogWriteInfo sLogMessageInf;

		{
			auto sLock = std::unique_lock<std::mutex>(pLogWriteMutex);
			pWrittenCondVar.wait(sLock, [this]
				{ return !pLogWriteQueue.empty() || bIsStopLogging || forceRotationRequested.load(); });

			if (bIsStopLogging && pLogWriteQueue.empty())
				break; 
			if (!pLogWriteQueue.empty()) {
				sLogMessageInf = pLogWriteQueue.front();
				pLogWriteQueue.pop();
				pWrittenCondVar.notify_one();
			}
		}
		
		// 在锁外检查并执行日志轮转，避免死锁
		CheckAndPerformRotation();
		if (!sLogMessageInf.sLogContentVal.empty() && pLogFileStream.is_open()) {
			pLogFileStream << sLogMessageInf.sLogTagNameVal << L"-//>>>" << GetCurrentTimer() << " : " << sLogMessageInf.sLogContentVal << "\n";
		}
	}
	pLogFileStream.close();
	std::cerr << "Log write thread Exit\n";
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
	
	// 触发回调（在写入队列之前）
	TriggerLogCallbacks(level, levelStr, sMessage);
	
	// Write to multi-output system if enabled
	if (multiOutputEnabled.load() && multiOutputManager) {
		LogCallbackInfo logInfo;
		logInfo.level = level;
		logInfo.levelString = levelStr;
		logInfo.message = sMessage;
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
		pLogWriteQueue.push({ levelStr, sMessage });
	}
	else if ( queueFullStrategy == LogQueueOverflowStrategy::DropOldest ) {
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
		pLogWriteQueue.push({ levelStr, sMessage });
	}
	pWrittenCondVar.notify_one();
	
	if (bNeedReport && !inErrorReport.exchange(true)) {
		std::wstring overflowMsg = L"The log queue overflows and has been discarded " + std::to_wstring(currentDiscard) + L" logs";
		WriteLogContent(LogLevel::Warning, overflowMsg);
		inErrorReport = false;
	}
}

void LightLogWrite_Impl::WriteLogContent(LogLevel level, const std::string& sMessage)
{
	WriteLogContent(level, UniConv::GetInstance()->LocaleToWideString(sMessage));
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
	pLogCallbacks.push_back({handle, callback, minLevel});
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
	rotationConfig = config;
	
	// 如果设置了归档目录但为空，则使用日志目录
	if (rotationConfig.archiveDirectory.empty()) {
		rotationConfig.archiveDirectory = sLogLastingDir;
	}
}

LogRotationConfig LightLogWrite_Impl::GetLogRotationConfig() const
{
	std::lock_guard<std::mutex> lock(rotationMutex);
	return rotationConfig;
}

void LightLogWrite_Impl::ForceLogRotation()
{
	// 设置手动轮转标志，让工作线程处理轮转，避免死锁
	forceRotationRequested = true;
	
	// 唤醒工作线程处理轮转请求
	{
		std::lock_guard<std::mutex> lock(pLogWriteMutex);
		pWrittenCondVar.notify_one();
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

void LightLogWrite_Impl::CheckAndPerformRotation()
{
	bool needRotation = false;
	std::wstring rotationReason;
	static int counter = 0;
	// 检查手动轮转请求
	if (forceRotationRequested.exchange(false)) {
		needRotation = true;
		rotationReason = L"Manual rotation requested";
	}
	
	// 如果没有手动轮转请求，检查自动轮转条件
	if (!needRotation && rotationConfig.strategy != LogRotationStrategy::None) {
		// 检查文件大小 - 使用无锁版本避免递归锁
		if (rotationConfig.strategy == LogRotationStrategy::Size || 
			rotationConfig.strategy == LogRotationStrategy::SizeAndTime) {
			
			size_t currentSize = GetCurrentLogFileSizeUnlocked();
			size_t maxSizeBytes = rotationConfig.maxFileSizeMB * 1024 * 1024;
			
			if (currentSize >= maxSizeBytes) {
				needRotation = true;
				rotationReason = L"File size limit reached (" + std::to_wstring(currentSize) + L" bytes)";

				++counter;
				std::cout << "[LogRotation] Size limit reached: " << currentSize << " bytes" << "current count: " << counter <<"\n";
			}
		}
		
		// 检查时间间隔
		if ((rotationConfig.strategy == LogRotationStrategy::Time || 
			 rotationConfig.strategy == LogRotationStrategy::SizeAndTime) && !needRotation) {
			if (IsTimeRotationNeeded()) {
				needRotation = true;
				rotationReason = L"Time interval reached";
			}
		}
	}
	
	if (needRotation) {
		PerformLogRotation(rotationReason);
	}
}

void LightLogWrite_Impl::PerformLogRotation(const std::wstring& reason)
{
	// 使用std::lock同时获取两个锁，避免死锁
	std::unique_lock<std::mutex> logLock(pLogWriteMutex, std::defer_lock);
	std::unique_lock<std::mutex> rotLock(rotationMutex, std::defer_lock);
	
	// 同时获取两个锁，防止死锁
	std::lock(logLock, rotLock);
	
	if (currentLogFileName.empty() || !std::filesystem::exists(currentLogFileName)) {
		return;
	}
	
	try {
		// 关闭当前文件
		if (pLogFileStream.is_open()) {
			pLogFileStream << L"[ROTATION] Log rotation triggered: " << reason << std::endl;
			pLogFileStream.close();
		}
		
		// 生成归档文件名
		auto now = std::chrono::system_clock::now();
		std::wstring archiveFileName = GenerateArchiveFileName(currentLogFileName, now);
		
		// 确保归档目录存在
		std::filesystem::path archivePath(archiveFileName);
		std::filesystem::create_directories(archivePath.parent_path());
		
		// 移动当前文件到归档位置
		std::filesystem::path tempArchivePath = archivePath;
		tempArchivePath.replace_extension(L".log");
		
		std::filesystem::rename(currentLogFileName, tempArchivePath);
		
		// 如果启用压缩，使用异步压缩队列
		if (rotationConfig.enableCompression) {
			// 将压缩任务加入队列，避免阻塞
			EnqueueCompressionTask(tempArchivePath.wstring(), archiveFileName);
		} else {
			std::filesystem::rename(tempArchivePath, archiveFileName);
		}
		
		// 创建新的日志文件 (使用无锁版本，因为已经持有锁)
		CreateLogsFileUnlocked();
		
		// 更新轮转时间
		lastRotationTime = now;
		
		// 清理旧归档文件
		CleanupOldArchives();
		
		// 记录轮转完成
		if (pLogFileStream.is_open()) {
			pLogFileStream << L"[ROTATION] Log rotation completed. Archive: " << archiveFileName << std::endl;
		}
	}
	catch (const std::exception& e) {
		// 轮转失败，尝试重新打开原文件 (使用无锁版本，因为已经持有锁)
		if (!pLogFileStream.is_open()) {
			CreateLogsFileUnlocked();
		}
		if (pLogFileStream.is_open()) {
			std::wstring errorMsg = L"[ROTATION_ERROR] Failed to rotate log: ";
			// 注意：这里需要将 std::string 转换为 std::wstring
			// errorMsg += std::wstring(e.what(), e.what() + strlen(e.what()));
			errorMsg += L"(see console for details)";
			pLogFileStream << errorMsg << std::endl;
		}
	}
}

bool LightLogWrite_Impl::CompressLogFile(const std::wstring& sourceFile, const std::wstring& targetFile)
{
	try {
		// 将宽字符路径转换为UTF-8字符串
		std::string sourceFileUtf8(sourceFile.begin(), sourceFile.end());
		std::string targetFileUtf8(targetFile.begin(), targetFile.end());
		
		// 使用miniz创建ZIP文件
		miniz_cpp::zip_file zipFile;
		
		// 读取源文件内容
		std::ifstream source(sourceFileUtf8, std::ios::binary);
		if (!source.is_open()) {
			return false;
		}
		
		// 获取文件内容
		std::string fileContent((std::istreambuf_iterator<char>(source)),
								std::istreambuf_iterator<char>());
		source.close();
		
		// 提取源文件的文件名（用作ZIP内部文件名）
		std::filesystem::path sourcePath(sourceFile);
		std::string internalFileName = sourcePath.filename().string();
		
		// 将文件添加到ZIP
		zipFile.writestr(internalFileName, fileContent);
		
		// 保存ZIP文件
		zipFile.save(targetFileUtf8);
		
		return true;
	}
	catch (const std::exception& e) {
		// 记录错误信息（可选）
		// 这里可以添加日志记录
		return false;
	}
	catch (...) {
		return false;
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
	
	// 构建归档文件名
	std::filesystem::path archiveDir(rotationConfig.archiveDirectory);
	std::wstring extension = rotationConfig.enableCompression ? L".zip" : L".log";
	std::wstring archiveFileName = baseName + L"_" + oss.str() + extension;
	
	return (archiveDir / archiveFileName).wstring();
}

bool LightLogWrite_Impl::IsTimeRotationNeeded() const
{
	auto now = std::chrono::system_clock::now();
	auto duration = now - lastRotationTime;
	
	switch (rotationConfig.timeInterval) {
		case TimeRotationInterval::Hourly:
			return duration >= std::chrono::hours(1);
		case TimeRotationInterval::Daily:
			return duration >= std::chrono::hours(24);
		case TimeRotationInterval::Weekly:
			return duration >= std::chrono::hours(24 * 7);
		case TimeRotationInterval::Monthly:
			return duration >= std::chrono::hours(24 * 30);  // 近似一个月
		default:
			return false;
	}
}

void LightLogWrite_Impl::CleanupOldArchives()
{
	if (rotationConfig.maxArchiveFiles == 0) {
		return;  // 无限制
	}
	
	try {
		std::filesystem::path archiveDir(rotationConfig.archiveDirectory);
		if (!std::filesystem::exists(archiveDir)) {
			return;
		}
		
		// 收集所有归档文件
		std::vector<std::filesystem::directory_entry> archiveFiles;
		std::wstring pattern = sLogsBasedName + L"_";
		
		for (const auto& entry : std::filesystem::directory_iterator(archiveDir)) {
			if (entry.is_regular_file()) {
				std::wstring fileName = entry.path().filename().wstring();
				if (fileName.find(pattern) == 0) {
					archiveFiles.push_back(entry);
				}
			}
		}
		
		// 按修改时间排序（最新的在前）
		std::sort(archiveFiles.begin(), archiveFiles.end(),
			[](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
				return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
			});
		
		// 删除超出限制的文件
		if (archiveFiles.size() > rotationConfig.maxArchiveFiles) {
			for (size_t i = rotationConfig.maxArchiveFiles; i < archiveFiles.size(); ++i) {
				std::filesystem::remove(archiveFiles[i].path());
			}
		}
	}
	catch (...) {
		// 清理失败，记录到当前日志文件（如果可能）
		if (pLogFileStream.is_open()) {
			pLogFileStream << L"[CLEANUP_ERROR] Failed to cleanup old archive files" << std::endl;
		}
	}
}

void LightLogWrite_Impl::StartCompressionWorker()
{
	stopCompressionWorker = false;
	compressionWorkerThread = std::thread(&LightLogWrite_Impl::CompressionWorkerLoop, this);
}

void LightLogWrite_Impl::StopCompressionWorker()
{
	// 设置停止标志
	stopCompressionWorker = true;
	
	// 通知工作线程
	{
		std::lock_guard<std::mutex> lock(compressionMutex);
		compressionCondVar.notify_all();
	}
	
	// 等待工作线程结束
	if (compressionWorkerThread.joinable()) {
		compressionWorkerThread.join();
	}
}

void LightLogWrite_Impl::EnqueueCompressionTask(const std::wstring& sourceFile, const std::wstring& targetFile)
{
	CompressionTask task;
	task.sourceFile = sourceFile;
	task.targetFile = targetFile;
	task.createdTime = std::chrono::system_clock::now();
	
	// 临时调试输出
	std::wcout << L"[DEBUG] Enqueuing compression task: " << sourceFile << L" -> " << targetFile << std::endl;
	
	{
		std::lock_guard<std::mutex> lock(compressionMutex);
		compressionQueue.push(task);
		std::wcout << L"[DEBUG] Compression task enqueued, queue size: " << compressionQueue.size() << std::endl;
		compressionCondVar.notify_one();
	}
}

void LightLogWrite_Impl::CompressionWorkerLoop()
{
	while (!stopCompressionWorker) {
		CompressionTask task;
		bool hasTask = false;
		
		// 从队列中获取任务
		{
			std::unique_lock<std::mutex> lock(compressionMutex);
			compressionCondVar.wait(lock, [this] {
				return !compressionQueue.empty() || stopCompressionWorker;
			});
			
			if (stopCompressionWorker && compressionQueue.empty()) {
				break;
			}
			
			if (!compressionQueue.empty()) {
				task = compressionQueue.front();
				compressionQueue.pop();
				hasTask = true;
			}
		}
		
		// 执行压缩任务
		if (hasTask) {
			try {
				// 记录压缩开始时间（用于性能监控）
				auto startTime = std::chrono::high_resolution_clock::now();
				
				// 执行压缩
				bool success = CompressLogFile(task.sourceFile, task.targetFile);
				
				// 计算压缩耗时
				auto endTime = std::chrono::high_resolution_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
				
				if (success) {
					// 压缩成功，删除原文件
					std::filesystem::remove(task.sourceFile);
					
					// 可选：记录压缩统计信息
					// std::wcout << L"Compression completed: " << task.targetFile 
					//           << L" (took " << duration.count() << L"ms)" << std::endl;
				} else {
					// 压缩失败，保留原文件并重命名为目标文件名（但不压缩）
					std::filesystem::path targetPath(task.targetFile);
					targetPath.replace_extension(L".log");
					std::filesystem::rename(task.sourceFile, targetPath);
					
					// 可选：记录错误信息
					// std::wcerr << L"Compression failed for: " << task.sourceFile << std::endl;
				}
			}
			catch (const std::exception& e) {
				// 处理异常，保留原文件
				try {
					std::filesystem::path targetPath(task.targetFile);
					targetPath.replace_extension(L".log");
					if (std::filesystem::exists(task.sourceFile)) {
						std::filesystem::rename(task.sourceFile, targetPath);
					}
				}
				catch (...) {
					// 重命名也失败了，至少不删除原文件
				}
			}
		}
	}
	
	// 处理剩余的压缩任务
	while (!compressionQueue.empty()) {
		CompressionTask task;
		{
			std::lock_guard<std::mutex> lock(compressionMutex);
			if (!compressionQueue.empty()) {
				task = compressionQueue.front();
				compressionQueue.pop();
			} else {
				break;
			}
		}
		
		// 快速处理剩余任务
		try {
			if (CompressLogFile(task.sourceFile, task.targetFile)) {
				std::filesystem::remove(task.sourceFile);
			} else {
				std::filesystem::path targetPath(task.targetFile);
				targetPath.replace_extension(L".log");
				std::filesystem::rename(task.sourceFile, targetPath);
			}
		}
		catch (...) {
			// 忽略异常，确保线程能正常退出
		}
	}
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

