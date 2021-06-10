#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include "parse.h"
#include "pcsa_net.h"
#define MAXBUF 1024

char *gettDateTime(void);
char *getExtension(const char *filename);
int checkHTTPversion(Request *request);
void getMime(char *extension, char *result);
