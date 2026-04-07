#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "net/serpent_net.h"
#include "value/value.h"
#include "object/object.h"
#include "memory/memory.h"

// Struct to hold response data
struct ResponseData {
    char* data;
    size_t size;
};

// Callback to write received data into a string
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    struct ResponseData* mem = (struct ResponseData*)userp;

    char* ptr = realloc(mem->data, mem->size + realsize + 1);
    if (ptr == NULL) return 0; // Out of memory

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

Value httpGetNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) return NIL_VAL;
    const char* url = AS_CSTRING(args[0]);

    CURL* curl = curl_easy_init();
    if (!curl) return NIL_VAL;

    struct ResponseData response = { malloc(1), 0 };
    response.data[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Serpent-HttpClient/1.0");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(response.data);
        return NIL_VAL;
    }

    // Convert to Serpent String
    // Note: copyString handles the allocation/copying into the VM's heap
    Value result = OBJ_VAL(copyString(response.data, (int)response.size));
    free(response.data);
    return result;
}

Value httpPostNative(int argCount, Value* args) {
    if (argCount < 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) return NIL_VAL;
    const char* url = AS_CSTRING(args[0]);
    const char* postData = AS_CSTRING(args[1]);
    const char* contentType = (argCount >= 3 && IS_STRING(args[2])) ? AS_CSTRING(args[2]) : "text/plain";

    CURL* curl = curl_easy_init();
    if (!curl) return NIL_VAL;

    struct ResponseData response = { malloc(1), 0 };
    response.data[0] = '\0';

    struct curl_slist* headers = NULL;
    char headerBuffer[256];
    snprintf(headerBuffer, sizeof(headerBuffer), "Content-Type: %s", contentType);
    headers = curl_slist_append(headers, headerBuffer);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Serpent-HttpClient/1.0");

    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(response.data);
        return NIL_VAL;
    }

    Value result = OBJ_VAL(copyString(response.data, (int)response.size));
    free(response.data);
    return result;
}
