#include <stdint.h>
#include <windows.h>

#include "thread-flex-bytes.h"

struct thread_flex_bytes_s {
    void*  dat;
    HANDLE mtx;
    size_t len;
    size_t cap;
};

ThreadFlexBytes* thread_flex_bytes_create(size_t inital_capacity) {
    ThreadFlexBytes* arr = malloc(sizeof(ThreadFlexBytes));
    if(arr == NULL) {
        return NULL;
    }
    arr->dat = malloc(inital_capacity);
    if(arr->dat == NULL) {
        free(arr);
        return NULL;
    }
    arr->mtx = CreateMutex(NULL, FALSE, NULL);
    if(arr->mtx == NULL) {
        free(arr->dat);
        free(arr);
        return NULL;
    }
    arr->len = 0;
    arr->cap = inital_capacity;
    return arr;
}

void* thread_flex_bytes_allocate_local_copy(ThreadFlexBytes* arr, size_t* len, size_t extra_bytes) {
    WaitForSingleObject(arr->mtx, INFINITE);
    void* tmp = malloc(arr->len + extra_bytes);
    if(tmp == NULL) {
        ReleaseMutex(arr->mtx);
        return NULL;
    }
    memcpy(tmp, arr->dat, arr->len);
    *len = arr->len;
    ReleaseMutex(arr->mtx);
    return tmp;
}

size_t thread_flex_bytes_copy_to_fixed_buffer(ThreadFlexBytes* arr, void* dest, size_t buffer_capacity) {
    WaitForSingleObject(arr->mtx, INFINITE);
    size_t bytes = arr->len;
    if(bytes > buffer_capacity) {
        bytes = buffer_capacity;
    }
    memcpy(dest, arr->dat, bytes);
    ReleaseMutex(arr->mtx);
    return bytes;
}

void thread_flex_bytes_copy_from(ThreadFlexBytes* arr, const void* src, size_t bytes) {
    WaitForSingleObject(arr->mtx, INFINITE);
    // check size
    if(bytes > arr->cap) {
        void* tmp = realloc(arr->dat, bytes);
        if(tmp == NULL) {
            MessageBox(NULL, "ThreadFlexBytes can not allocate memory! Exiting Process.", "Error!", MB_OK | MB_ICONERROR);
            ReleaseMutex(arr->mtx);
            ExitProcess(0);
        }
        arr->dat = tmp;
        arr->cap = bytes;
    }

    memcpy(arr->dat, src, bytes);
    arr->len = bytes;
    ReleaseMutex(arr->mtx);
}

size_t thread_flex_bytes_get_length(ThreadFlexBytes* arr) {
    size_t len = 0;
    WaitForSingleObject(arr->mtx, INFINITE);
    len = arr->len;
    ReleaseMutex(arr->mtx);
    return len;
}

void thread_flex_bytes_free(ThreadFlexBytes* arr) {
    WaitForSingleObject(arr->mtx, INFINITE);
    ReleaseMutex(arr->mtx);
    CloseHandle(arr->mtx);
    free(arr->dat);
    free(arr);
}
