/* default server port (can be changed by -p command line argument),
 * and it's length (ASCII bytes including terminating null byte) */
#define DEFAULT_PORT 4567
#define DEFAULT_PORT_LEN 6 /* enough for all valid port numbers */

/* exit code for invalid command line arguments */
#define EXIT_INVALID 1
#define EXIT_NETFAIL 2
