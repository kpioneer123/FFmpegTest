#include <pthread.h>
//解码
#include "include/libavcodec/avcodec.h"

#define ElementType AVPacket //存储数据元素的类型
#define true 1
#define false 0
typedef struct _Queue Queue;

Queue* CreateQueue() ;
/**
 * 队列压人元素
 */
void QueuePush(Queue* queue,ElementType element,pthread_mutex_t *mutex,pthread_cond_t *cond, int abort_request);

/**
 * 弹出元素
 */
ElementType QueuePop(Queue *queue,pthread_mutex_t *mutex,pthread_cond_t *cond, int abort_request);

/**
 * 队列压人元素
 */
void QueueFree(Queue* queue);