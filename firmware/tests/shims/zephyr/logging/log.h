/* Host-native test shim only — never used in the real Zephyr build (CMakeLists.txt
 * doesn't add this directory to the include path). Lets chunk_protocol.c compile with
 * a plain host compiler so its logic can be checked against the Python reference. */
#ifndef SHIM_ZEPHYR_LOGGING_LOG_H_
#define SHIM_ZEPHYR_LOGGING_LOG_H_

#include <stdio.h>

#define LOG_MODULE_REGISTER(name, level)                                                          \
	static const char *_log_module_name __attribute__((unused)) = #name

#define LOG_WRN(fmt, ...) fprintf(stderr, "[WRN] " fmt "\n", ##__VA_ARGS__)
#define LOG_INF(fmt, ...) fprintf(stderr, "[INF] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) fprintf(stderr, "[ERR] " fmt "\n", ##__VA_ARGS__)

#endif
