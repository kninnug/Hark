#include "util.h"

void * fmalloc(size_t n){
	void * p = malloc(n);
	if(p == NULL){
		fprintf(stderr, "! malloc failed (%zu)\n", n);
		exit(EXIT_FAILURE);
	}
	return p;
}

size_t highFreq(fftw_complex * fftOut, int fftSize, double * intens){
	size_t i, idx;
	double high = 0, amp = 0;
	
	for(i = 1; i < fftSize/2 + 1; i++){
		amp = fftOut[i][0]*fftOut[i][0] + fftOut[i][1]*fftOut[i][1];
		if(amp > high){
			high = amp;
			idx = i;
		}
	}
	
	if(intens != NULL) *intens = high;
	
	return idx;
}

static void heap_enqueue(struct heap * h, double item, int addon);
static void heap_full(struct heap * h);
static void heap_swap(struct heap * h, int a, int b);
static void heap_upheap(struct heap * h, int n);
static int heap_removeMin(struct heap * h);
static void heap_downheap(struct heap * h, int n);

struct heap * heap_new(size_t length){
	struct heap * ret = fmalloc(sizeof *ret);
	ret->length = length + 1;
	ret->items = fmalloc(ret->length * sizeof *ret->items);
	ret->addons = fmalloc(ret->length * sizeof *ret->addons);
	ret->front = 1;
	
	return ret;
}

static void heap_enqueue(struct heap * h, double item, int addon){
	if(h->front == h->length){
		heap_full(h);
	}
	
	h->items[h->front] = item;
	h->addons[h->front] = addon;
	heap_upheap(h, h->front);
	h->front++;
}

static void heap_full(struct heap * h){
	heap_removeMin(h);
}

static void heap_swap(struct heap * h, int a, int b){
	double t = h->items[a];
	int i = h->addons[a];
	h->items[a] = h->items[b];
	h->addons[a] = h->addons[b];
	h->items[b] = t;
	h->addons[b] = i;
}

static void heap_upheap(struct heap * h, int n){
	if(n > 1){
		if(h->items[n] < h->items[n / 2]){
			heap_swap(h, n, n / 2);
			heap_upheap(h, n / 2);
		}
	}
}

static int heap_removeMin(struct heap * h){
	if(h->front < 1){
		return 0.0;
	}
	
	int addon = h->addons[1];
	h->front--;
	h->items[1] = h->items[h->front];
	h->addons[1] = h->addons[h->front];
	heap_downheap(h, 1);
	return addon;
}

static void heap_downheap(struct heap * h, int n){
	if(2 * n + 1 <  h->front){
		if(h->items[2 * n] > h->items[2 * n + 1] && h->items[n] > h->items[2 * n + 1]){
			heap_swap(h, n, 2 * n + 1);
			heap_downheap(h, 2 * n + 1);
		}else if(h->items[n] > h->items[2 * n]){
			heap_swap(h, n, 2 * n);
			heap_downheap(h, 2 * n);
		}
	}else if(h->front == 2 * n + 1 && h->items[n] > h->items[2 * n]){
		heap_swap(h, n, 2 * n);
	}
}

int * threshFreq(fftw_complex * fftOut, int fftSize, int threshold, int * in, size_t len, struct heap * hin){
	size_t i;
	double amp, ampLeft, ampRight;
	struct heap * h = heap_new(len);
	
	for(i = 1; i < fftSize / 2 + 1; i++){
		amp = fftOut[i][0]*fftOut[i][0] + fftOut[i][1]*fftOut[i][1];
		ampLeft = i > 1 ? fftOut[i - 1][0]*fftOut[i - 1][0] + fftOut[i - 1][1]*fftOut[i - 1][1] : 0.0;
		ampRight = i <= fftSize / 2 ? fftOut[i + 1][0]*fftOut[i + 1][0] + fftOut[i + 1][1]*fftOut[i + 1][1] : 0.0;
		if(amp > threshold && amp > ampLeft && amp > ampRight){
			heap_enqueue(h, amp, i);
		}
	}
	
	for(i = 0; i < h->length; i++){
		in[i] = heap_removeMin(h);
	}
	
	return in;
}

int _main(){
	size_t i;
	struct heap * h = heap_new(5);
	
	heap_enqueue(h, 12, 21);
	heap_enqueue(h, 13, 31);
	heap_enqueue(h, 14, 41);
	heap_enqueue(h, 15, 51);
	heap_enqueue(h, 9, 6);
	heap_enqueue(h, 17, 71);
	heap_enqueue(h, 18, 81);
	heap_enqueue(h, 19, 91);
	heap_enqueue(h, 10, 0);
	
	for(i = 0; i < h->length; i++){
		printf("%i\n", heap_removeMin(h));
	}
	
	return 0;
}
