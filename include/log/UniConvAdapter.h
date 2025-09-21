#pragma once

#include "UniConv.h"
#include <string>
#include <codecvt>
#include <locale>

/**
 * @brief Adapter class to bridge old UniConv API with new UniConv API
 * @details This class provides compatibility methods to support legacy code
 *          while using the new UniConv library API
 */
class UniConvAdapter {
public:
    /**
     * @brief Convert local encoding string to wide string
     * @param input Input string in local encoding
     * @return Wide string result
     */
    static std::wstring LocaleToWideString(const std::string& input) {
        // Get current system encoding
        std::string currentEncoding = UniConv::GetInstance()->GetCurrentSystemEncoding();
        if (currentEncoding.empty()) {
            currentEncoding = "UTF-8"; // fallback to UTF-8
        }
        
        // Convert from current encoding to UTF-8
        auto result = UniConv::GetInstance()->ConvertEncodingFast(input, currentEncoding.c_str(), "UTF-8");
        if (!result.IsSuccess()) {
            // Fallback: use standard library conversion
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            try {
                return converter.from_bytes(input);
            } catch (...) {
                return L""; // Return empty string on error
            }
        }
        
        // Convert UTF-8 to wide string
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        try {
            return converter.from_bytes(result.GetValue());
        } catch (...) {
            return L""; // Return empty string on error
        }
    }
    
    /**
     * @brief Convert U16 string to wide string
     * @param input Input U16 string
     * @return Wide string result
     */
    static std::wstring U16StringToWString(const std::u16string& input) {
        // Convert u16string to string first
        std::string utf8_str;
        utf8_str.reserve(input.size() * 3); // Reserve enough space
        
        for (char16_t ch : input) {
            if (ch < 0x80) {
                utf8_str.push_back(static_cast<char>(ch));
            } else if (ch < 0x800) {
                utf8_str.push_back(static_cast<char>(0xC0 | (ch >> 6)));
                utf8_str.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
            } else {
                utf8_str.push_back(static_cast<char>(0xE0 | (ch >> 12)));
                utf8_str.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
                utf8_str.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
            }
        }
        
        // Convert UTF-8 to wide string
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        try {
            return converter.from_bytes(utf8_str);
        } catch (...) {
            return L""; // Return empty string on error
        }
    }
    
    /**
     * @brief Convert wide string to local encoding string
     * @param input Input wide string
     * @return Local encoding string result
     */
    static std::string WideStringToLocale(const std::wstring& input) {
        // Convert wide string to UTF-8 first
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string utf8_str;
        try {
            utf8_str = converter.to_bytes(input);
        } catch (...) {
            return ""; // Return empty string on error
        }
        
        // Get current system encoding
        std::string currentEncoding = UniConv::GetInstance()->GetCurrentSystemEncoding();
        if (currentEncoding.empty()) {
            currentEncoding = "UTF-8"; // fallback to UTF-8
        }
        
        // Convert from UTF-8 to current encoding
        auto result = UniConv::GetInstance()->ConvertEncodingFast(utf8_str, "UTF-8", currentEncoding.c_str());
        if (!result.IsSuccess()) {
            // Fallback: return UTF-8 string directly
            return utf8_str;
        }
        
        return result.GetValue();
    }
};