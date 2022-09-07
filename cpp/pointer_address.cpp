#include <stdio.h>
#include <stdint.h>

int main() {
	int a[10];
	int* p1 = a;
	int* p2 = p1 + 1;
	void* p3 = p1;
	void* p4 = p2;

	printf("p1=%p,p2=%p,p3=%p,p4=%p\n", p1, p2, p3, p4);
	printf("p1=0x%lx,p2=0x%lx,p3=0x%lx,p4=0x%lx\n", (uint64_t)p1, (uint64_t)p2, (uint64_t)p3, (uint64_t)p4);
	
	printf("p2 - p1 = %ld\n",  p2 - p1);

	// compile error
	// fprintf("p4 - p3 = %ld\n",  p4 - p3);
    
	return 0;
}
