#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <Lmcons.h>
#include <windows.h>
#include <io.h>
#include <assert.h>
#include <combaseapi.h>
#include "cJSON.h"


static char *locateSettingsFile()
{
    /* user name */
    long unsigned int userNameLength = UNLEN + 1;
    char *userName = malloc(userNameLength * sizeof(char));
    GetUserName(userName, &userNameLength);
    /* settings file */
    char *settingsFile = malloc(1024 * sizeof(char));
    sprintf(settingsFile, "C:\\Users\\%s\\AppData\\Local\\Packages\\Microsoft.WindowsTerminal_8wekyb3d8bbwe\\LocalState\\settings.json", userName);
    free(userName);
    assert(!access(settingsFile, R_OK));
    return settingsFile;
}


static cJSON *readFile(char *settingsFile)
{
    char *buffer = malloc(10240 * 1024 * sizeof(char));
    FILE *openFile = fopen(settingsFile, "rb");
    assert(openFile);
    fread(buffer, 1024 * 1024, 1, openFile);
    fclose(openFile);
    /* json */
    cJSON *json;
    json = cJSON_Parse(buffer);
    free(buffer);
    return json;
}


static int listProfiles(cJSON *profileList)
{
    int profiles = cJSON_GetArraySize(profileList);
    assert(profiles); /* list is empty */
    for (int i = 0; i < profiles; i++)
    {
        cJSON *profile = cJSON_GetArrayItem(profileList, i);
        cJSON *profileCommandLine = cJSON_GetObjectItem(profile, "commandline");
        if (profileCommandLine && !strncmp(profileCommandLine->valuestring, "ssh", 3))
        {
            unsigned int port;
            char userHost[1024];
            cJSON *profileName = cJSON_GetObjectItem(profile, "name");
            sscanf(profileCommandLine->valuestring, "ssh -o ServerAliveInterval=60 -p %u %s",&port, userHost);
            printf("%s -> %s:%u\n", profileName->valuestring, userHost, port);
        }
    }
    return 0;
}


/*
static int getProfileNames(cJSON *profileList)
{
    int profiles = cJSON_GetArraySize(profileList);
    assert(profiles);
    char *profileNameList = malloc(profiles * sizeof(char *));
    for (int i = 0; i < profiles; i++)
    {
        cJSON *profile = cJSON_GetArrayItem(profileList, i);
        cJSON *profileName = cJSON_GetObjectItem(profile, "name");
        assert(profileName);
        profileNameList[i] = profileName->valuestring;
    }
    return profiles;
}
*/


static int getGuid(char *guidString)
{
    GUID guid;
    CoCreateGuid(&guid);
    sprintf(guidString, "{%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return 0;
}


static int isValidHost(char *host)
{
    int flag = 0;
    int a, b, c, d;
    if (sscanf(host, "%d.%d.%d.%d", &a, &b, &c, &d) == 4)
    {
        if (a >= 0 && a <= 255 && b >= 0 && b <= 255 && c >= 0 && c <= 255 && d >= 0 && d <= 255)
            flag = 1;
    }
    return flag;
}


static int isValidPort(int port)
{
    int flag = (port >= 0 && port <= 65535) ? 1 : 0;
    return flag;
}


static int addProfile(cJSON *profileList, char *profileString)
{
    char name[1024];
    char user[1024];
    char host[1024];
    char commandLine[1024];
    char tabTitle[1024];
    unsigned int port;
    char guid[39];
    
    assert(sscanf(profileString, "%[^:]:%[^@]@%[^:]:%u", &name, &user, &host, &port) == 4);
    assert(isValidHost(host));
    assert(isValidPort(port));
    sprintf(commandLine, "ssh -o ServerAliveInterval=60 -p %u %s@%s", port, user, host);
    sprintf(tabTitle, "%s (%s)", name, host);
    getGuid(guid);

    cJSON *profile = cJSON_CreateObject();
    cJSON_AddStringToObject(profile, "name", name);
    cJSON_AddStringToObject(profile, "guid", guid);
    cJSON_AddStringToObject(profile, "tabTitle", tabTitle);
    cJSON_AddStringToObject(profile, "commandline", commandLine);
    cJSON_AddTrueToObject(profile, "suppressApplicationTitle");
    cJSON_AddFalseToObject(profile, "hidden");
    cJSON_AddNumberToObject(profile, "historySize", 10000);
    cJSON_AddItemToArray(profileList, profile);
    return 0;
}


static int deleteProfile(cJSON *profileList, char *profileNameString)
{
    int profiles = cJSON_GetArraySize(profileList);
    for (int i = 0; i < profiles; i++)
    {
        cJSON *profile = cJSON_GetArrayItem(profileList, i);
        cJSON *profileName = cJSON_GetObjectItem(profile, "name");
        if (profileName && !strcmp(profileName->valuestring, profileNameString))
        {
            cJSON_DeleteItemFromArray(profileList, i);
            profiles--;
            i--;
        }
    }
    return 0;
}


static int saveProfile(char *settingsFile, cJSON *root)
{
    char *json = cJSON_Print(root);
    FILE *openFile = fopen(settingsFile, "w");
    assert(openFile);
    fputs(json, openFile);
}


int main(int argc, char *argv[])
{
    /* read settings file */
    char *settingsFile = locateSettingsFile();
    cJSON *profileRoot = readFile(settingsFile);
    cJSON *profileList = cJSON_GetObjectItem(cJSON_GetObjectItem(profileRoot, "profiles"), "list");
    int flag = 0;
    for (int i = 1; i < argc; i++)
    {
        if (!strncasecmp("-h", argv[i], 2) || !strncasecmp("--h", argv[i], 3));
        else if (!strncasecmp("-l", argv[i], 2) || !strncasecmp("--l", argv[i], 3))
        {
            listProfiles(profileList);
            exit(EXIT_SUCCESS);
        }
        else if (!strncasecmp("-a", argv[i], 2) || !strncasecmp("--a", argv[i], 3))
            flag = 1;
        else if (!strncasecmp("-d", argv[i], 2) || !strncasecmp("--d", argv[i], 3))
            flag = 2;
        else if (flag == 1)
            addProfile(profileList, argv[i]);
        else if (flag == 2)
            deleteProfile(profileList, argv[i]);
    }
    if (flag)
        saveProfile(settingsFile, profileRoot);
    else
    {
        puts("\nSSH helper for Windows11 v0.2 (https://github.com/liu-congcong/winSSHelper)");
        puts("Usage:");
        puts("List profiles:");
        printf("    %s --list\n", argv[0]);
        puts("Add new profiles:");
        printf("    %s --add <node>:<user>@<ip>:<port> ...\n", argv[0]);
        puts("Remove profiles:");
        printf("    %s --delete <node> ...\n", argv[0]);
    }
    return 0;
}