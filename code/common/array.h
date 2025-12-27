/*
* USAGE:
* 1. Defining an array
*  Since in C, struct {int a; } != struct { int a; }, you cannot return an array(T)
*  for any T. So you will have to typedef every type that you want to use.
*  (i.e. typedef array(int) IntArray; typedef array(my_type) my_type_array; ...)
*  This is the major incoveniance that you will have with this library.
*
* 2. By reference or by value ?
*  Before we start, note that all function that start with "_array" expect a ptr
*  to the array casted to "array_void_t *" and all function that start with "array"
*  will expect passing by value. (i.e. they won't check for NULL pointer)
*
* 3. Creating an array
*  `array_init` allocate an array with the given capacity and memzero new space
*  if `ARRAY_ALLOC_ZERO` is defined.
*  `array_init2` same as `array_init` but set the size equals to the capacity.
*
* 4. Iterating over array
*  Similar to C89 you have to declare the iterator before the loop
*  e.g.
* ```c
*  int i;
*  for (i = 0; i < size; i++)
* ```
*
*  e.g.
*  IntArray array;
*  int *val;
*  array_forach(val, array)
*      do_something(*val);
*
*  In the same way use, array_foreach(entity, entities) { do_something(*entity); }
*  There is also a shortcut `array_for` that is create an iterator named `it`.
*  e.g. array_for(it, entities) { do_something(*it); }
*
*  Finally, reverse iterator are also provided `array_foreach_reverse` and `array_for_reverse`.
*/

#ifndef COMMON_ARRAY_H
#define COMMON_ARRAY_H

#define array_npos ((size_t)(-1))

typedef struct array_void_t {
    size_t capacity;
    size_t size;
    void  *data;
} array_void_t;

#define array(T)                \
union {                         \
    array_void_t base;          \
    struct {                    \
        size_t capacity;        \
        size_t size;            \
        T     *data;            \
    };                          \
}

typedef array(char)     array_char_t;
typedef array(int)      array_int_t;
typedef array(uint8_t)  array_uint8_t;
typedef array(uint16_t) array_uint16_t;
typedef array(uint32_t) array_uint32_t;
typedef array(uint64_t) array_uint64_t;
typedef array(int8_t)   array_int8_t;
typedef array(int16_t)  array_int16_t;
typedef array(int32_t)  array_int32_t;
typedef array(int64_t)  array_int64_t;

#define array_init(a)           _array_init(&(a)->base)
#define array_reset(a)          _array_reset(&(a)->base);

#define array_reserve(a, s)     _array_reserve(&(a)->base, (s), sizeof(*(a)->data))
#define array_resize(a, s)      _array_resize(&(a)->base, (s), sizeof(*(a)->data))
#define array_remove(a, i)      _array_remove(&(a)->base, (i), sizeof(*(a)->data))
#define array_insert(a, c, p)   _array_insert(&(a)->base, (c), (p), sizeof(*(a)->data))
#define array_copy(d, s)        _array_copy(&(d)->base, &(s)->base, sizeof(*(s)->data))
#define array_push(a, n)        _array_push(&(a)->base, (n), sizeof(*(a)->data))

#define array_remove_ordered(a, i) _array_remove_range_ordered(&(a)->base, (i), 1, sizeof(*(a)->data))
#define array_remove_range_ordered(a, i, c) _array_remove_range_ordered(&(a)->base, (i), (c), sizeof(*(a)->data))

#define array_add(a, e)         (array_reserve(a, 1) && (((a)->data[(a)->size++] = (e)), 1))
#define array_set(a, i, e)      (array_inside(a, i)  && (((a)->data[(i)] = (e)), 1))
#define array_pop(a)            ((a)->data[(a)->size ? --(a)->size : 0])
#define array_at(a, i)          ((a)->data[_array_at_chk(&(a)->base, (i), __FILE__, __LINE__)])

#define array_front(a)          ((a)->data[0])
#define array_back(a)           ((a)->data[(a)->size - 1])
#define array_peek(a)           ((a)->data[(a)->size - 1])

#define array_size(a)           ((a)->size)
#define array_data(a)           ((a)->data)
#define array_capacity(a)       ((a)->capacity)

#define array_inside(a, i)      (0 <= (i) && (size_t)(i) < (a)->size)
#define array_full(a)           ((a)->size == (a)->capacity)
#define array_empty(a)          ((a)->size == 0)
#define array_clear(a)          ((void)((a)->size =  0))

#define array_begin(a)          ((a)->data)
#define array_end(a)            ((a)->data + (a)->size)

#define array_rbegin(a)         ((a)->data + (a)->size - 1)
#define array_rend(a)           ((a)->data - 1)

#define array_foreach(it, a) \
    for (it = array_begin(a); it != array_end(a); ++it)

#define array_foreach_reverse(it, a) \
    for (it = array_rbegin(a); it != array_rend(a); --it)

#if !defined(_MSC_VER)
# define array_for(a)           array_foreach(it, a)
# define array_for_reverse(a)   array_foreach_reverse(it, a)
#endif

void _array_init(array_void_t *a);
void _array_reset(array_void_t *a);

int _array_resize(array_void_t *a, size_t size, const size_t elem_size);
int _array_reserve(array_void_t *a, size_t count, const size_t elem_size);
void _array_remove(array_void_t *a, size_t index, const size_t elem_size);
int _array_insert(array_void_t *a, size_t count, const void *ptr, const size_t elem_size);
int _array_copy(array_void_t *dest, array_void_t *src, const size_t elem_size);
void * _array_push(array_void_t *a, size_t n, const size_t elem_size);
void _array_remove_ordered(array_void_t *a, size_t index, const size_t elem_size);
void _array_remove_range_ordered(array_void_t *a, size_t index, size_t count, const size_t elem_size);
size_t _array_at_chk(array_void_t *a, size_t index, const char *file, int line);

#endif // COMMON_ARRAY_H
