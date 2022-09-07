#include <stdio.h>
#include <pthread.h>

const char* thread_tls = "child thread";
const char* main_tls = "main thread";


void* thread_proc(void*  user) {
    pthread_key_t key = *((pthread_key_t*)user);
    pthread_setspecific(key, thread_tls);
    printf("thread=%lu %s\n", pthread_self(), (const char*)pthread_getspecific(key));
    return NULL;
}

void thread_destruct(void* ) {
    printf("thread=%lu destructor is called\n", pthread_self());
}


int main(int argc, char** argv) {
    pthread_t tid;
    pthread_key_t key;

    pthread_key_create(&key, thread_destruct);
    pthread_create(&tid, NULL, thread_proc, &key);

    pthread_setspecific(key, main_tls);
    printf("thread=%lu %s\n", pthread_self(), (const char*)pthread_getspecific(key));

    pthread_join(tid, NULL);
    pthread_key_delete(key);
    return 0;
}
