#ifndef THREAD_FLEX_BYTES_H
#define THREAD_FLEX_BYTES_H

struct thread_flex_bytes_s;
typedef struct thread_flex_bytes_s ThreadFlexBytes;

ThreadFlexBytes* thread_flex_bytes_create(size_t inital_bytes);
void             thread_flex_bytes_free(ThreadFlexBytes* arr);

void*            thread_flex_bytes_allocate_local_copy(ThreadFlexBytes* arr, size_t* len, size_t extra_bytes);
size_t           thread_flex_bytes_copy_to_fixed_buffer(ThreadFlexBytes* arr, void* dest, size_t buffer_capacity);
void             thread_flex_bytes_copy_from(ThreadFlexBytes* arr, const void* src, size_t bytes);
size_t           thread_flex_bytes_get_length(ThreadFlexBytes* arr);

#endif