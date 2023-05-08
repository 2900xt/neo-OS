#pragma once

void assert_fail(const char *assertion, const char *file, unsigned int line);
#define assert(expression) (static_cast<bool>(expression) ? void(0) : assert_fail(#expression, __FILE__, __LINE__))