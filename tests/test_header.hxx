
#ifndef CXXCMD_TEST_HEADER
#define CXXCMD_TEST_HEADER 1

#define DECLARE_ARGS(...)\
const char * argv[] = { __VA_ARGS__ };\
int argc = sizeof(argv) / sizeof(const char *);

#endif // !CXXCMD_TEST_HEADER
