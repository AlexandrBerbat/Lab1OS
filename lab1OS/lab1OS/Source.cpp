#include <stdio.h>;
#include <stdlib.h>;
#include <stdbool.h>;
#include <stdint.h>
#include <malloc.h>
#include <assert.h>

#define HEAP_SIZE 4096

static char heap[HEAP_SIZE];
static void* PTR;


static size_t SIZE = 2501;
#pragma pack(push, 1) 
typedef struct header
{
	bool status; 
	size_t previous_size;
	size_t size;
}header_t;
static int SIZE_H = sizeof(header_t);
void set_status(void* pointer, size_t status)
{
	((header_t*)pointer)->status = status;
}
bool get_status(void* pointer)
{
	return ((header_t*)pointer)->status;
}
void set_previous_size(void* pointer, size_t previous_size)
{
	((header_t*)pointer)->previous_size = previous_size;
}
size_t get_previous_size(void* pointer)
{
	return ((header_t*)pointer)->previous_size;
}
void set_size(void* pointer, size_t size)
{
	((header_t*)pointer)->size = size;
}
size_t get_size(void* pointer)
{

	return ((header_t*)pointer)->size;
}
void* get_next(void* pointer)
{
	
	if ((uint8_t*)pointer + get_size(pointer) + SIZE_H == (uint8_t*)PTR + SIZE + SIZE_H)
	{
		return NULL;
	}
	return (uint8_t*)pointer + get_size(pointer) + SIZE_H;
}
void* get_previous(void* pointer)
{
	if (pointer == PTR)
	{
		return NULL;
	}
	return (uint8_t*)pointer - get_previous_size(pointer) - SIZE_H;
}
void new_header(void* pointer, bool status, size_t previous_size, size_t size)
{
	header_t a;
	a.status = status;
	a.previous_size = previous_size;
	a.size = size;
	*((header_t*)pointer) = a;
}
void combine_headers(void* pointer1, void* pointer2) 
{
	assert(get_status(pointer1) == 0 && get_status(pointer2) == 0);
	set_size(pointer1, get_size(pointer1) + get_size(pointer2) + SIZE_H);
	
	if (get_next(pointer2) != NULL)
	{
		set_previous_size(get_next(pointer2), get_size(pointer1));
	}
}
void* block(size_t size)
{
	
	void* pointer = heap + SIZE_H;
	new_header(pointer, false, 0, size);

	return pointer;
}
void* get_best(size_t size)
{
	void* pointer = PTR;
	void* best = NULL;
	while (pointer != NULL)
	{
		if ((best == NULL || get_size(best) > get_size(pointer)) && get_size(pointer)
			>= size && get_status(pointer) == 0) 
		{
			best = pointer;
		}
		pointer = get_next(pointer);
	}
	return best;
}

void* mem_alloc(size_t size)
{
	if (size % 4 != 0)
	{
		size = size - size % 4 + 4;
	}
	void* pointer = get_best(size);
	if (pointer == NULL)
	{
		return pointer;
	}

	if (get_size(pointer) > size + SIZE_H)
	{
		new_header((uint8_t*)pointer + size + SIZE_H, 0, size, get_size(pointer) - size - SIZE_H);
		set_size(pointer, size);
	}
	set_status(pointer, true);
	return (uint8_t*)pointer + SIZE_H;
}
void mem_free(void* pointer)
{
	pointer = (uint8_t*)pointer - SIZE_H;
	set_status(pointer, false);
	
	if (get_next(pointer) != NULL && get_status(get_next(pointer)) == 0)
	{
		combine_headers(pointer, get_next(pointer));
	} 
	if (get_previous(pointer) != NULL && get_status(get_previous(pointer)) == 0)
	{
		combine_headers(get_previous(pointer), pointer);
	}
}
void* mem_realloc(void* pointer, size_t size)
{
	pointer = (uint8_t*)pointer - SIZE_H;
	if (size % 4 != 0)
	{
		size = size - size % 4 + 4;
	}
	if (get_size(pointer) == size)
	{
		return pointer;
	}
	if (get_size(pointer) > size) 
	{
		if (get_size(pointer) - size - SIZE_H >= 0)
		{
			new_header((uint8_t*)pointer + size + SIZE_H, false, size, get_size(pointer) - size -
				SIZE_H);
			set_size(pointer, size);
			if (get_next(get_next(pointer)) != NULL &&
				get_status(get_next(get_next(pointer))) == 0)
			{
				combine_headers(get_next(pointer), get_next(get_next(pointer)));
			}
		}
		return pointer;

	}
	if (get_next(pointer) != NULL && get_size(pointer) + get_size(get_next(pointer))
		>= size)
	{
		new_header((uint8_t*)pointer + size + SIZE_H, false, size, get_size(get_next(pointer)) -
			(size - get_size(pointer)));
		set_size(pointer, size);
		return pointer;
	}
	if (get_next(pointer) != NULL && get_status(get_next(pointer)) == 0 &&
		get_size(pointer) + get_size(get_next(pointer)) + SIZE_H >= size)
	{
		set_size(pointer, get_size(pointer) + get_size(get_next(pointer)) + SIZE_H);
		return pointer;
	}
	void* best = mem_alloc(size);
	if (best == NULL)
	{
		return best;
	}
	mem_free((uint8_t*)pointer + SIZE_H);
	return best;
}
void mem_dump()
{
	void* pointer = PTR;
	size_t size_h = 0;
	size_t size_b = 0;
	printf(" ______________________________________________________________________\n");
	printf("% 15s | % 6s | % 7s | % 7s | % 15s | % 15s | \n", "address", "status", "pr_size", "size",
		"previous", "next");
	while (pointer != NULL)
	{
		printf("% 15p | % 6d | % 7ld | % 7ld | % 15p | % 15p | \n", pointer, get_status(pointer),
			get_previous_size(pointer), get_size(pointer), get_previous(pointer),
			get_next(pointer));
		size_h = size_h + SIZE_H;
		size_b = size_b + get_size(pointer);
		pointer = get_next(pointer);
	}
	printf("----------------------------------------------------------------------\n");
	printf("headers: %ld\nblocks : %ld\nsummary : %ld\n\n\n", size_h, size_b, size_h +
		size_b);
}
int main()
{
	PTR = block(SIZE);
	void* x1 = mem_alloc(1);
	mem_dump();
	void* x2 = mem_alloc(1);
	mem_dump();
	void* x3 = mem_alloc(3);
	mem_dump();
	void* x4 = mem_alloc(80);
	mem_dump();
	mem_free(x2);
	mem_free(x3);
	mem_dump();
	void* x5 = mem_alloc(20);
	void* x6 = mem_alloc(20);
	void* x7 = mem_alloc(70);
	mem_dump();

	x1 = mem_realloc(x1, 30);

	mem_dump();
	return 0;
}
