#define profile(level,fmt...) profile_timer(level,MODULE,__PRETTY_FUNCTION__,fmt)
void profile_timer(int level, const char *module, const char *function, const char *fmt, ...);
