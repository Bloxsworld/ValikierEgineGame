#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>

enum class LogLevel3D {
    Info,
    Warn,
    Error,
    Anim,
    Gui,
    Build
};

struct LogEntry3D {
    std::string timestamp;
    LogLevel3D level;
    std::string channel;
    std::string message;
};

class Logger3D {
public:
    static Logger3D& Instance() {
        static Logger3D inst;
        return inst;
    }

    void Init(const std::string& logFilePath = "game_runtime.log", size_t maxHistory = 200) {
        m_LogPath = logFilePath;
        m_MaxHistory = maxHistory;
        m_Entries.clear();
        std::ofstream clearFile(m_LogPath, std::ios::trunc);
        Log(LogLevel3D::Info, "Logger3D", "Initialized runtime logger -> " + m_LogPath);
    }

    void Log(LogLevel3D level, const std::string& channel, const std::string& msg) {
        std::string ts = NowStamp();
        LogEntry3D entry{ ts, level, channel, msg };

        m_Entries.push_back(entry);
        if (m_Entries.size() > m_MaxHistory) {
            m_Entries.erase(m_Entries.begin());
        }

        std::string formatted = "[" + ts + "] [" + LevelName(level) + "] [" + channel + "] " + msg;
        std::cout << formatted << "\n";

        std::ofstream out(m_LogPath, std::ios::app);
        if (out.is_open()) {
            out << formatted << "\n";
        }
    }

    void Info(const std::string& ch, const std::string& msg) { Log(LogLevel3D::Info, ch, msg); }
    void Warn(const std::string& ch, const std::string& msg) { Log(LogLevel3D::Warn, ch, msg); }
    void Error(const std::string& ch, const std::string& msg) { Log(LogLevel3D::Error, ch, msg); }
    void Anim(const std::string& ch, const std::string& msg) { Log(LogLevel3D::Anim, ch, msg); }
    void Gui(const std::string& ch, const std::string& msg) { Log(LogLevel3D::Gui, ch, msg); }
    void Build(const std::string& ch, const std::string& msg) { Log(LogLevel3D::Build, ch, msg); }

    const std::vector<LogEntry3D>& GetHistory() const { return m_Entries; }

    std::vector<std::string> GetRecentFormatted(size_t count) const {
        std::vector<std::string> out;
        size_t start = (m_Entries.size() > count) ? (m_Entries.size() - count) : 0;
        for (size_t i = start; i < m_Entries.size(); ++i) {
            const auto& e = m_Entries[i];
            out.push_back("[" + e.timestamp + "] [" + LevelName(e.level) + "] [" + e.channel + "] " + e.message);
        }
        return out;
    }

private:
    Logger3D() = default;

    std::string NowStamp() const {
        std::time_t now = std::time(nullptr);
        char buf[32]{};
        std::tm tmBuf{};
        localtime_s(&tmBuf, &now);
        std::strftime(buf, sizeof(buf), "%H:%M:%S", &tmBuf);
        return std::string(buf);
    }

    const char* LevelName(LogLevel3D lvl) const {
        switch (lvl) {
        case LogLevel3D::Info:  return "INFO";
        case LogLevel3D::Warn:  return "WARN";
        case LogLevel3D::Error: return "ERR";
        case LogLevel3D::Anim:  return "ANIM";
        case LogLevel3D::Gui:   return "GUI";
        case LogLevel3D::Build: return "BUILD";
        }
        return "LOG";
    }

    std::string m_LogPath = "game_runtime.log";
    size_t m_MaxHistory = 200;
    std::vector<LogEntry3D> m_Entries;
};