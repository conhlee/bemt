#include "unluacInterface.h"

#include "../cons/list.h"

#include "../cons/error.h"

#include "../cons/type.h"

#include <stdio.h>

#include <string.h>

char* UnluacRun(const char* luacPath) {
    char command[1024];
    FILE* fp;

    snprintf(command, sizeof(command), "java -jar unluac.jar \"%s\" 2>&1", luacPath);
    fp = popen(command, "r");

    if (fp == NULL) {
        Panic("UnluacRun: failed to run unluac");
        return NULL;
    }

    ConsList outputList;
    ListInit(&outputList, sizeof(char), 1024);

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp)) {
        for (u64 i = 0; buffer[i] != '\0'; ++i)
            ListAdd(&outputList, &buffer[i]);
    }

    pclose(fp);

    // Add null terminator.
    buffer[0] = '\0';
    ListAdd(&outputList, buffer + 0);

    char* outputString = strdup((char*)outputList.data);
    ListDestroy(&outputList);

    return outputString;
}
