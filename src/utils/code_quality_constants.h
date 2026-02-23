// Code Quality Constants
// Centralized magic numbers and configuration values

#ifndef ROBOCLAW_CODE_QUALITY_CONSTANTS_H
#define ROBOCLAW_CODE_QUALITY_CONSTANTS_H

#include <chrono>

namespace roboclaw {
namespace constants {

// ============================================================================
// JSON Formatting
// ============================================================================
constexpr int JSON_INDENT_WIDTH = 2;

// ============================================================================
// Thread Pool Constants
// ============================================================================
constexpr size_t DEFAULT_MIN_THREADS = 4;
constexpr size_t MAX_DYNAMIC_ADD_THREADS = 4;
constexpr int WORKER_WAIT_MS = 10;
constexpr int MAX_BACKOFF_MS = 10000;
constexpr int INITIAL_BACKOFF_MS = 1000;

// ============================================================================
// HTTP Client Constants
// ============================================================================
constexpr int DEFAULT_HTTP_TIMEOUT_SECONDS = 60;
constexpr int DEFAULT_HTTP_RETRY_BACKOFF_MS = 1000;
constexpr int MAX_HTTP_RETRY_BACKOFF_MS = 10000;

// ============================================================================
// Browser Automation Constants
// ============================================================================
constexpr int DEFAULT_BROWSER_DRIVER_PORT = 9515;
constexpr int BROWSER_STARTUP_WAIT_MS = 1000;
constexpr int DEFAULT_BROWSER_WAIT_MS = 1000;

// ============================================================================
// Buffer Sizes
// ============================================================================
constexpr size_t FILE_READ_BUFFER_SIZE = 4096;
constexpr size_t SCRIPT_BUFFER_SIZE = 128;

// ============================================================================
// Timeout Constants
// ============================================================================
constexpr std::chrono::milliseconds DEFAULT_WAIT_MS{1000};
constexpr std::chrono::milliseconds LONG_POLL_TIMEOUT_MS{30000};  // 30 seconds for Telegram long polling

// ============================================================================
// Serial Port Constants
// ============================================================================
constexpr int DEFAULT_SERIAL_TIMEOUT_MS = 1000;
constexpr int DEFAULT_SERIAL_BAUD_RATE = 115200;

} // namespace constants
} // namespace roboclaw

#endif // ROBOCLAW_CODE_QUALITY_CONSTANTS_H
