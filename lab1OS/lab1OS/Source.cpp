#include <stdio.h>;
#include <stdlib.h>;
#include <stdbool.h>;
#include <stdint.h>
#include <malloc.h>
#include <assert.h>

#define HEAP_SIZE 1024

static char heap[HEAP_SIZE];
static void* PTR;

// size_t беззнаковый тип, используемый для подсчета или индексации массива
static size_t SIZE = 812;
#pragma pack(push, 1) //заносит текущее значение на вершину
typedef struct header
{
	bool status; // 0 - свободно, 1 - занято
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
	// uint8_t - беззнаковое целое размером 8 бит
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
void combine_headers(void* pointer1, void* pointer2) // сделать 1 блок из 2
{
	assert(get_status(pointer1) == 0 && get_status(pointer2) == 0);
	set_size(pointer1, get_size(pointer1) + get_size(pointer2) + SIZE_H);
	// Если следующий указатель существует, обновите его информацию о предыдущем блоке
	if (get_next(pointer2) != NULL)
	{
		set_previous_size(get_next(pointer2), get_size(pointer1));
	}
}
void* block(size_t size)
{
	//void* pointer = malloc(size + SIZE_H);
	//size -= SIZE_H;
	void* pointer = heap + SIZE_H;
	new_header(pointer, false, 0, size);

	return pointer;
}
void* get_best(size_t size) //выбераем лучшую область памяти
{
	void* pointer = PTR;
	void* best = NULL;
	while (pointer != NULL)
	{
		if ((best == NULL || get_size(best) > get_size(pointer)) && get_size(pointer) // если лучшее пусто или в нем больше пустого места
			>= size && get_status(pointer) == 0) // и указатель не занят
		{
			best = pointer;
		}
		pointer = get_next(pointer); // перейти к следующему 
	}
	return best;
}

void* mem_alloc(size_t size)
{
	if (size % 4 != 0) //выровнять 4 байта
	{
		size = size - size % 4 + 4;
	}
	void* pointer = get_best(size);
	if (pointer == NULL)
	{
		return pointer; // не может выделить, так как нет пустой области
	}
	//assert(pointer != NULL);

	// если размер найденного блока больше требуемого
	// создать новый заголовок в конце нового блока, который будет работать до конца блока
	// (разделение большого блока на два: один выделяется, другой - нет)
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
	// если следующий свободен, объедините два блока
	if (get_next(pointer) != NULL && get_status(get_next(pointer)) == 0)
	{
		combine_headers(pointer, get_next(pointer));
	} // если предыдущее пусто, объединить текущий с предыдущим
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
	if (get_size(pointer) > size) // если в существующем блоке больше памяти, чем необходимо
	{
		if (get_size(pointer) - size - SIZE_H >= 0) // новый размер должен быть больше, чем размер заголовка (достаточно места для формирования нового блока)
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
	if (get_next(pointer) != NULL && get_size(pointer) + get_size(get_next(pointer)) //если есть следующий блок size_1 + size_2> = size
		>= size)
	{
		new_header((uint8_t*)pointer + size + SIZE_H, false, size, get_size(get_next(pointer)) -
			(size - get_size(pointer))); //создайте новый блок с начала первого блока, первый блок теперь между новым и вторым
		set_size(pointer, size);
		return pointer;
	}
	if (get_next(pointer) != NULL && get_status(get_next(pointer)) == 0 &&
		get_size(pointer) + get_size(get_next(pointer)) + SIZE_H >= size) // если второй блок свободен, просто добавьте его во второй блок
	{
		set_size(pointer, get_size(pointer) + get_size(get_next(pointer)) + SIZE_H);
		return pointer;
	}
	void* best = mem_alloc(size); //если новый размер больше, чем блок1 + блок2
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