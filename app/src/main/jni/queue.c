#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>
#define  LOG_TAG    "ffmpegandroidplayer"
#define  LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,FORMAT,##__VA_ARGS__);
#define  LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,FORMAT,##__VA_ARGS__);
#define  LOGD(FORMAT,...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,FORMAT, ##__VA_ARGS__)

#define MAXSIZE 15 //存储数据元素的最大个数
#define ERROR -99 //ElementType的特殊值，标志错误


struct _Queue {
	ElementType data[MAXSIZE];
	int front; //记录队列头元素位置
	int rear; //记录队列尾元素位置
	int size; //存储数据元素的个数
};

Queue* CreateQueue() {
	Queue* q = (Queue*)malloc(sizeof(Queue));
	if (!q) {
		printf("空间不足\n");
		return NULL;
	}
	q->front = -1;
	q->rear = -1;
	q->size = 0;
	return q;
}

int IsFullQ(Queue* q) {
	return (q->size == MAXSIZE);
}

void QueuePush(Queue* q, ElementType item, pthread_mutex_t *mutex, pthread_cond_t *cond, int abort_request) {

	if (abort_request) {
		LOGI("put_packet abort");
		return;
	}

		if (!IsFullQ(q)) {
			q->rear++;
			q->rear %= MAXSIZE;
			q->size++;
			q->data[q->rear] = item;
			//通知
			pthread_cond_broadcast(cond);

		}
		else {
			LOGI("队列已满");
			//阻塞
			pthread_cond_wait(cond, mutex);
		}



	return;
}

int IsEmptyQ(Queue* q) {
	return (q->size == 0);
}

ElementType QueuePop(Queue* q, pthread_mutex_t *mutex, pthread_cond_t *cond, int abort_request) {
	if (abort_request) {
		LOGI("put_packet abort");
		ElementType ele;
		return ele;
	}

     while(true){

		if (!IsEmptyQ(q)) {
			q->front++;
			q->front %= MAXSIZE; //0 1 2 3 4 5
			q->size--;
			//通知
			pthread_cond_broadcast(cond);
			return q->data[q->front];

		}
		else {
			LOGI("队列为空");

			//阻塞
			pthread_cond_wait(cond, mutex);
		}
}



}

/**
 * 销毁队列
 */
void QueueFree(Queue* queue){

	free(queue->data);
	free(queue);
}


