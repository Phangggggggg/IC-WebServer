#include "service.h"

char *gettDateTime(void)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    return asctime(tm);
}

char *getExtension(const char *filename)
{
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    return dot + 1;
}
int checkHTTPversion(Request *request)
{
    char *reqHTTPVersion = request->http_version;
    char *httpVersion = "HTTP/1.1";
    if (!strcmp(reqHTTPVersion, httpVersion))
    {
        return 1;
    }
    return 0;
}

void getMime(char *extension, char *result)
{
    if (!strcmp(extension, "html"))
    {
        strcpy(result, "text/html");
    }
    else if (!strcmp(extension, "js"))
    {
        strcpy(result, "text/javascript");
    }
    else if (!strcmp(extension, "png"))
    {
        strcpy(result, "image/png");
    }
    else if (!strcmp(extension, "jpeg"))
    {
        strcpy(result, "image/jpeg");
    }
    else if (!strcmp(extension, "jpg"))
    {
        strcpy(result, "image/jpg");
    }
    else if (!strcmp(extension, "css"))
    {
        strcpy(result, "text/css");
    }
    else if (!strcmp(extension, "gif"))
    {
        strcpy(result, "image/gif");
    }
    else if (!strcmp(extension, "txt"))
    {
        strcpy(result, "text/plain");
    }
    else
    { // none of above mime
        strcpy(result, "none");
    }
}