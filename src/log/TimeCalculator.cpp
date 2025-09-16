#include "../../include/log/TimeCalculator.h"
#include <sstream>
#include <iomanip>
#include <cmath>

/*****************************************************************************
 *  TimeCalculator
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     TimeCalculator.cpp
 *  @brief    精确时间计算器实现
 *  @details  提供精确的轮转时间计算，处理月份差异等复杂情况
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/16
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

std::chrono::system_clock::time_point TimeCalculator::GetNextRotationTime(
    PreciseTimeInterval interval,
    const std::chrono::system_clock::time_point& baseTime,
    TimeAlignment alignment)
{
    std::chrono::system_clock::time_point alignedTime = AlignTime(baseTime, alignment);
    
    switch (interval) {
        case PreciseTimeInterval::Minutely:
            return alignedTime + std::chrono::minutes(1);
            
        case PreciseTimeInterval::Hourly:
            return alignedTime + std::chrono::hours(1);
            
        case PreciseTimeInterval::Daily:
            return alignedTime + std::chrono::hours(24);
            
        case PreciseTimeInterval::Weekly:
            return alignedTime + std::chrono::hours(24 * 7);
            
        case PreciseTimeInterval::Monthly:
            return AddMonths(alignedTime, 1);
            
        case PreciseTimeInterval::Yearly:
            return AddYears(alignedTime, 1);
            
        default:
            return alignedTime + std::chrono::hours(1);
    }
}

bool TimeCalculator::IsRotationTime(
    PreciseTimeInterval interval,
    const std::chrono::system_clock::time_point& lastRotation,
    const std::chrono::system_clock::time_point& currentTime,
    TimeAlignment alignment)
{
    std::chrono::system_clock::time_point nextRotationTime = 
        GetNextRotationTime(interval, lastRotation, alignment);
    
    return currentTime >= nextRotationTime;
}

std::chrono::system_clock::time_point TimeCalculator::AlignTime(
    const std::chrono::system_clock::time_point& timePoint,
    TimeAlignment alignment)
{
    if (alignment == TimeAlignment::None) {
        return timePoint;
    }
    
    std::tm tm = TimePointToTm(timePoint);
    
    switch (alignment) {
        case TimeAlignment::Minute:
            tm.tm_sec = 0;
            break;
            
        case TimeAlignment::Hour:
            tm.tm_sec = 0;
            tm.tm_min = 0;
            break;
            
        case TimeAlignment::Day:
            tm.tm_sec = 0;
            tm.tm_min = 0;
            tm.tm_hour = 0;
            break;
            
        case TimeAlignment::Week:
            return GetWeekStart(timePoint);
            
        case TimeAlignment::Month:
            return GetMonthStart(timePoint);
            
        case TimeAlignment::Year:
            return GetYearStart(timePoint);
            
        default:
            break;
    }
    
    return TmToTimePoint(tm);
}

int TimeCalculator::GetDaysInMonth(int year, int month)
{
    static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (month < 1 || month > 12) {
        return 0;
    }
    
    if (month == 2 && IsLeapYear(year)) {
        return 29;
    }
    
    return daysInMonth[month - 1];
}

bool TimeCalculator::IsLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

std::chrono::system_clock::time_point TimeCalculator::GetWeekStart(
    const std::chrono::system_clock::time_point& timePoint)
{
    std::tm tm = TimePointToTm(timePoint);
    
    // 将时间对齐到当天的开始
    tm.tm_sec = 0;
    tm.tm_min = 0;
    tm.tm_hour = 0;
    
    // 计算到周一的天数差
    int daysToMonday = (tm.tm_wday + 6) % 7; // tm_wday: 0=Sunday, 1=Monday, ...
    
    // 减去相应的天数
    std::chrono::system_clock::time_point dayStart = TmToTimePoint(tm);
    return dayStart - std::chrono::hours(24 * daysToMonday);
}

std::chrono::system_clock::time_point TimeCalculator::GetMonthStart(
    const std::chrono::system_clock::time_point& timePoint)
{
    std::tm tm = TimePointToTm(timePoint);
    
    tm.tm_sec = 0;
    tm.tm_min = 0;
    tm.tm_hour = 0;
    tm.tm_mday = 1; // 月份的第一天
    
    return TmToTimePoint(tm);
}

std::chrono::system_clock::time_point TimeCalculator::GetYearStart(
    const std::chrono::system_clock::time_point& timePoint)
{
    std::tm tm = TimePointToTm(timePoint);
    
    tm.tm_sec = 0;
    tm.tm_min = 0;
    tm.tm_hour = 0;
    tm.tm_mday = 1;
    tm.tm_mon = 0; // 一月
    
    return TmToTimePoint(tm);
}

std::chrono::system_clock::time_point TimeCalculator::AddMonths(
    const std::chrono::system_clock::time_point& timePoint,
    int months)
{
    std::tm tm = TimePointToTm(timePoint);
    
    int originalDay = tm.tm_mday;
    
    // 添加月份
    tm.tm_mon += months;
    
    // 标准化tm结构
    tm = NormalizeTm(tm);
    
    // 处理月末日期问题（例如：1月31日 + 1个月 应该是2月28日/29日）
    int daysInTargetMonth = GetDaysInMonth(tm.tm_year + 1900, tm.tm_mon + 1);
    if (originalDay > daysInTargetMonth) {
        tm.tm_mday = daysInTargetMonth;
    }
    
    return TmToTimePoint(tm);
}

std::chrono::system_clock::time_point TimeCalculator::AddYears(
    const std::chrono::system_clock::time_point& timePoint,
    int years)
{
    std::tm tm = TimePointToTm(timePoint);
    
    tm.tm_year += years;
    
    // 处理闰年问题（2月29日 + 1年 到非闰年）
    if (tm.tm_mon == 1 && tm.tm_mday == 29 && !IsLeapYear(tm.tm_year + 1900)) {
        tm.tm_mday = 28;
    }
    
    return TmToTimePoint(tm);
}

std::wstring TimeCalculator::FormatTime(
    const std::chrono::system_clock::time_point& timePoint,
    const std::wstring& format)
{
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm* tm = std::localtime(&time);
    
    if (!tm) {
        return L"Invalid Time";
    }
    
    std::wostringstream oss;
    oss << std::put_time(tm, format.c_str());
    return oss.str();
}

std::wstring TimeCalculator::GetDurationDescription(
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end)
{
    auto duration = end - start;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    
    if (seconds < 0) {
        seconds = -seconds;
    }
    
    std::wostringstream desc;
    
    if (seconds < 60) {
        desc << seconds << L" seconds";
    } else if (seconds < 3600) {
        int minutes = static_cast<int>(seconds / 60);
        int remainingSeconds = static_cast<int>(seconds % 60);
        desc << minutes << L" minutes";
        if (remainingSeconds > 0) {
            desc << L", " << remainingSeconds << L" seconds";
        }
    } else if (seconds < 86400) {
        int hours = static_cast<int>(seconds / 3600);
        int remainingMinutes = static_cast<int>((seconds % 3600) / 60);
        desc << hours << L" hours";
        if (remainingMinutes > 0) {
            desc << L", " << remainingMinutes << L" minutes";
        }
    } else {
        int days = static_cast<int>(seconds / 86400);
        int remainingHours = static_cast<int>((seconds % 86400) / 3600);
        desc << days << L" days";
        if (remainingHours > 0) {
            desc << L", " << remainingHours << L" hours";
        }
    }
    
    return desc.str();
}

std::wstring TimeCalculator::GetIntervalDescription(PreciseTimeInterval interval)
{
    switch (interval) {
        case PreciseTimeInterval::Minutely:
            return L"Every minute";
        case PreciseTimeInterval::Hourly:
            return L"Every hour";
        case PreciseTimeInterval::Daily:
            return L"Every day";
        case PreciseTimeInterval::Weekly:
            return L"Every week";
        case PreciseTimeInterval::Monthly:
            return L"Every month";
        case PreciseTimeInterval::Yearly:
            return L"Every year";
        default:
            return L"Unknown interval";
    }
}

// 私有方法实现

std::tm TimeCalculator::TimePointToTm(const std::chrono::system_clock::time_point& timePoint)
{
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm* tm = std::localtime(&time);
    
    if (tm) {
        return *tm;
    }
    
    // 返回一个默认的tm结构
    std::tm defaultTm = {};
    defaultTm.tm_year = 70; // 1970
    defaultTm.tm_mon = 0;   // January
    defaultTm.tm_mday = 1;  // 1st
    return defaultTm;
}

std::chrono::system_clock::time_point TimeCalculator::TmToTimePoint(const std::tm& tm)
{
    std::tm mutableTm = tm;
    std::time_t time = std::mktime(&mutableTm);
    
    if (time == -1) {
        // 如果转换失败，返回epoch时间
        return std::chrono::system_clock::from_time_t(0);
    }
    
    return std::chrono::system_clock::from_time_t(time);
}

std::tm TimeCalculator::NormalizeTm(const std::tm& tm)
{
    std::tm normalizedTm = tm;
    
    // 处理月份溢出
    while (normalizedTm.tm_mon >= 12) {
        normalizedTm.tm_mon -= 12;
        normalizedTm.tm_year++;
    }
    
    while (normalizedTm.tm_mon < 0) {
        normalizedTm.tm_mon += 12;
        normalizedTm.tm_year--;
    }
    
    // 处理日期溢出
    while (normalizedTm.tm_mday > GetDaysInMonth(normalizedTm.tm_year + 1900, normalizedTm.tm_mon + 1)) {
        normalizedTm.tm_mday -= GetDaysInMonth(normalizedTm.tm_year + 1900, normalizedTm.tm_mon + 1);
        normalizedTm.tm_mon++;
        
        if (normalizedTm.tm_mon >= 12) {
            normalizedTm.tm_mon = 0;
            normalizedTm.tm_year++;
        }
    }
    
    while (normalizedTm.tm_mday < 1) {
        normalizedTm.tm_mon--;
        
        if (normalizedTm.tm_mon < 0) {
            normalizedTm.tm_mon = 11;
            normalizedTm.tm_year--;
        }
        
        normalizedTm.tm_mday += GetDaysInMonth(normalizedTm.tm_year + 1900, normalizedTm.tm_mon + 1);
    }
    
    // 处理小时溢出
    while (normalizedTm.tm_hour >= 24) {
        normalizedTm.tm_hour -= 24;
        normalizedTm.tm_mday++;
    }
    
    while (normalizedTm.tm_hour < 0) {
        normalizedTm.tm_hour += 24;
        normalizedTm.tm_mday--;
    }
    
    // 处理分钟和秒的溢出
    while (normalizedTm.tm_min >= 60) {
        normalizedTm.tm_min -= 60;
        normalizedTm.tm_hour++;
    }
    
    while (normalizedTm.tm_min < 0) {
        normalizedTm.tm_min += 60;
        normalizedTm.tm_hour--;
    }
    
    while (normalizedTm.tm_sec >= 60) {
        normalizedTm.tm_sec -= 60;
        normalizedTm.tm_min++;
    }
    
    while (normalizedTm.tm_sec < 0) {
        normalizedTm.tm_sec += 60;
        normalizedTm.tm_min--;
    }
    
    // 递归处理可能的连锁溢出
    if (normalizedTm.tm_hour >= 24 || normalizedTm.tm_hour < 0 ||
        normalizedTm.tm_mday > GetDaysInMonth(normalizedTm.tm_year + 1900, normalizedTm.tm_mon + 1) ||
        normalizedTm.tm_mday < 1 || normalizedTm.tm_mon >= 12 || normalizedTm.tm_mon < 0) {
        return NormalizeTm(normalizedTm);
    }
    
    return normalizedTm;
}