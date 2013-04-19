/* default server port (can be changed by -p command line argument),
 * and it's length (ASCII bytes including terminating null byte) */
#define DEFAULT_PORT 4567
#define DEFAULT_PORT_LEN 6 /* enough for all valid port numbers */

/* exit code for invalid command line arguments */
#define EXIT_INVALID 1
/* exit code for network problems */
#define EXIT_NETFAIL 2
/* exit code for memory errors */
#define EXIT_MEMFAIL 3

/* macro and function to check if memory allocation was successful */
#define CHKALLOC(a) chkalloc(a, __FILE__, __LINE__)
void chkalloc(void *ptr, char *file, int line);

/* based on timeradd in <sys/time.h> */
#define timespecadd(a, b, result)					\
	do								\
	{								\
		(result)->tv_sec = (a)->tv_sec + (b)->tv_sec;		\
		(result)->tv_nsec = (a)->tv_nsec + (b)->tv_nsec;	\
		if ((result)->tv_nsec >= 1000000000)			\
		{							\
			++(result)->tv_sec;				\
			(result)->tv_nsec -= 1000000000;		\
		}							\
	} while (0)
