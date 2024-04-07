
typedef enum
{
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
} LogLevel;

const char* log_level_strings[] = {
  "INFO", "WARN", "ERROR"
};

const char* log_level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};

va_list ap;

void _log(LogLevel level, const char* file, int line, const char* fmt, ...)
{
    time_t t = time(NULL);
    struct tm* _time = localtime(&t);

    char time_str[10] = {0};
    strftime(time_str, sizeof(time_str), "%H:%M:%S", _time);

    va_start(ap, fmt);
    fprintf(stdout, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", time_str, log_level_colors[level], log_level_strings[level], file, line);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    fflush(stdout);
    va_end(ap);
}

#define logi(...) _log(LOG_LEVEL_INFO,  __FILE__, __LINE__, __VA_ARGS__);
#define logw(...) _log(LOG_LEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__);
#define loge(...) _log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__);
