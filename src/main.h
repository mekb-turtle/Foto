#define die(exit) die3(NULL, NULL, NULL, exit)
#define die1(msg, exit) die3(msg, NULL, NULL, exit)
#define die2(msg, msg2, exit) die3(msg, msg2, NULL, exit)

void die3(const char *msg, const char *msg2, const char *msg3, int exitcode);
