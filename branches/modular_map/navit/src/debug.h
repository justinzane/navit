void debug_init(void);
int debug_level;
void debug_print(int level, const char *module, const char *function, const char *fmt, ...);

#define dbg(level,fmt...) if (debug_level >= level) debug_print(level,MODULE,__PRETTY_FUNCTION__,fmt)
