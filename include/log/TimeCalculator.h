#pragma once

#include <chrono>
#include <ctime>
#include <string>

/*****************************************************************************
 *  TimeCalculator
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     TimeCalculator.h
 *  @brief    精确时间计算器
 *  @details  提供精确的轮转时间计算，处理月份差异等复杂情况
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/15
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

/**
 * @brief 时间轮转间隔枚举
 */
enum class PreciseTimeInterval {
    Minutely,       /*!< 每分钟 */
    Hourly,         /*!< 每小时 */
    Daily,          /*!< 每天 */
    Weekly,         /*!< 每周 */
    Monthly,        /*!< 每月(自然月) */
    Yearly          /*!< 每年 */
};

/**
 * @brief 时间对齐方式
 */
enum class TimeAlignment {
    None,           /*!< 不对齐 */
    Minute,         /*!< 对齐到分钟边界 */
    Hour,           /*!< 对齐到小时边界 */
    Day,            /*!< 对齐到天边界 */
    Week,           /*!< 对齐到周边界(周一) */
    Month,          /*!< 对齐到月边界 */
    Year            /*!< 对齐到年边界 */
};

/**
 * @brief 精确时间计算器
 * @details 提供精确的时间计算，处理闰年、月份天数变化等复杂情况
 */
class TimeCalculator {
public:
    /**
     * @brief 计算下次轮转时间
     * @param interval 时间间隔
     * @param baseTime 基准时间
     * @param alignment 时间对齐方式
     * @return 下次轮转时间
     */
    static std::chrono::system_clock::time_point GetNextRotationTime(
        PreciseTimeInterval interval,
        const std::chrono::system_clock::time_point& baseTime,
        TimeAlignment alignment = TimeAlignment::None);
    
    /**
     * @brief 判断是否到达轮转时间
     * @param interval 时间间隔
     * @param lastRotation 上次轮转时间
     * @param currentTime 当前时间
     * @param alignment 时间对齐方式
     * @return 是否应该轮转
     */
    static bool IsRotationTime(
        PreciseTimeInterval interval,
        const std::chrono::system_clock::time_point& lastRotation,
        const std::chrono::system_clock::time_point& currentTime,
        TimeAlignment alignment = TimeAlignment::None);
    
    /**
     * @brief 对齐时间到指定边界
     * @param timePoint 要对齐的时间
     * @param alignment 对齐方式
     * @return 对齐后的时间
     */
    static std::chrono::system_clock::time_point AlignTime(
        const std::chrono::system_clock::time_point& timePoint,
        TimeAlignment alignment);
    
    /**
     * @brief 获取月份的天数
     * @param year 年份
     * @param month 月份(1-12)
     * @return 天数
     */
    static int GetDaysInMonth(int year, int month);
    
    /**
     * @brief 判断是否为闰年
     * @param year 年份
     * @return 是否为闰年
     */
    static bool IsLeapYear(int year);
    
    /**
     * @brief 获取周的第一天(周一)
     * @param timePoint 任意时间点
     * @return 该周的周一
     */
    static std::chrono::system_clock::time_point GetWeekStart(
        const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief 获取月的第一天
     * @param timePoint 任意时间点
     * @return 该月的第一天
     */
    static std::chrono::system_clock::time_point GetMonthStart(
        const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief 获取年的第一天
     * @param timePoint 任意时间点
     * @return 该年的第一天
     */
    static std::chrono::system_clock::time_point GetYearStart(
        const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief 添加月份（处理月末日期问题）
     * @param timePoint 基准时间
     * @param months 要添加的月数
     * @return 添加后的时间
     */
    static std::chrono::system_clock::time_point AddMonths(
        const std::chrono::system_clock::time_point& timePoint,
        int months);
    
    /**
     * @brief 添加年份
     * @param timePoint 基准时间
     * @param years 要添加的年数
     * @return 添加后的时间
     */
    static std::chrono::system_clock::time_point AddYears(
        const std::chrono::system_clock::time_point& timePoint,
        int years);
    
    /**
     * @brief 格式化时间为字符串
     * @param timePoint 时间点
     * @param format 格式字符串(strftime格式)
     * @return 格式化后的字符串
     */
    static std::wstring FormatTime(
        const std::chrono::system_clock::time_point& timePoint,
        const std::wstring& format = L"%Y-%m-%d %H:%M:%S");
    
    /**
     * @brief 计算两个时间点的差异描述
     * @param start 开始时间
     * @param end 结束时间
     * @return 差异描述字符串
     */
    static std::wstring GetDurationDescription(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end);
    
    /**
     * @brief 获取时间间隔的描述
     * @param interval 时间间隔
     * @return 描述字符串
     */
    static std::wstring GetIntervalDescription(PreciseTimeInterval interval);

private:
    /**
     * @brief 将system_clock::time_point转换为tm结构
     * @param timePoint 时间点
     * @return tm结构
     */
    static std::tm TimePointToTm(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief 将tm结构转换为system_clock::time_point
     * @param tm tm结构
     * @return 时间点
     */
    static std::chrono::system_clock::time_point TmToTimePoint(const std::tm& tm);
    
    /**
     * @brief 标准化tm结构（处理月份、日期溢出）
     * @param tm 要标准化的tm结构
     * @return 标准化后的tm结构
     */
    static std::tm NormalizeTm(const std::tm& tm);
};