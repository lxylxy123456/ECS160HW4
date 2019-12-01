#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_LINE 1030		// 1024
#define MAX_LINE_EXTRA (MAX_LINE + 10)
#define MAX_NLINE 20010		// 20000
#define INIT_CAPACITY 20020	// Initial capacity of vector
#define MALLOC_OFFSET 3		// Small extra space for malloc

void error()
{
	printf("Invalid Input Format\n");
	exit(1);
}

void assert(bool exp)
{
	if (!exp)
		error();
}

char *csv_tok(char *str)
{
	// Similar to strtok(str, ",") that does not collapse deliminators
	// Return NULL on error
	static char *data = NULL;	// Internal
	if (str)
		data = str;
	if (!data)
		return data;	// End of line
	char *p;		// Pointer to first comma / end of string
	char *prev_str = data;	// Pointer to previous string
	if ((p = strchr(data, ','))) {
		*p = '\0';
		data = p + 1;
	} else {
		p = strchr(data, '\0');
		data = NULL;
	}
	// Eliminate '"'
	if (p >= prev_str + 2 && prev_str[0] == '\"' && p[-1] == '\"') {
		prev_str++;
		p[-1] = '\0';
	}
	return prev_str;
}

int read_line(char *line, FILE * f)
{
	// `line` size should be MAX_LINE_EXTRA
	// Return: 0: success; -1: EOF; no error
	char *str = NULL;
	size_t n = 0;
	size_t len = getline(&str, &n, f);
	if (len == -1) {	//end of file
		free(str);
		return -1;
	}
	assert(len == strlen(str));
	assert(strlen(str) < MAX_LINE);
	if (strlen(str) > 0 && str[strlen(str) - 1] == '\n')
		str[strlen(str) - 1] = '\0';
	if (strlen(str) > 0 && str[strlen(str) - 1] == '\r')
		str[strlen(str) - 1] = '\0';
	strncpy(line, str, MAX_LINE_EXTRA);
	free(str);
	return 0;
}

int col_index(FILE * f)
{
	// index of column "name" in first line, always success
	char line[MAX_LINE_EXTRA];
	assert(read_line(line, f) == 0);
	char *str = line;
	int ans = -1;
	for (int i = 0; i < MAX_LINE; i++) {
		char *token = csv_tok(str);
		if (!token)
			break;
		if (strcmp(token, "name") == 0) {
			assert(ans == -1);
			ans = i;
		}
		str = NULL;
	}
	assert(ans != -1);
	return ans;
}

// Begin: "class" vector (similar to std::vector)

typedef struct vector_entry {
	char *name;
	int count;
} vector_entry_t;

typedef struct vector {
	int size, capacity;
	vector_entry_t *entry;
} *vector_t;

vector_t vector_new()
{
	vector_t ans = malloc(sizeof(struct vector));
	assert(ans);
	ans->size = 0;
	ans->capacity = INIT_CAPACITY;
	ans->entry = malloc(ans->capacity * sizeof(vector_entry_t));
	assert(ans->entry);
	return ans;
}

void vector_push_back(vector_t v, vector_entry_t elem)
{
	assert(v->size >= 0 && v->size < MAX_NLINE);
	assert(v->size <= v->capacity);	// Internal
	if (v->size == v->capacity) {
		v->capacity *= 2;
		v->entry =
		    realloc(v->entry, v->capacity * sizeof(vector_entry_t));
		assert(v->entry);
	}
	assert(v->size < v->capacity);	// Internal
	v->entry[v->size] = elem;
	v->size += 1;
}

vector_entry_t vector_get(vector_t v, int index)
{
	// Does not fail
	assert(index >= 0 && index < v->size);
	assert(v->size <= v->capacity);	// Internal
	return v->entry[index];
}

void vector_set(vector_t v, int index, vector_entry_t elem)
{
	// Does not fail
	assert(index >= 0 && index < v->size);
	assert(v->size <= v->capacity);	// Internal
	v->entry[index] = elem;
}

int vector_size(vector_t v)
{
	return v->size;
}

// End: "class" vector

void add_name(char *token, vector_t v)
{
	for (int i = 0; i < vector_size(v); i++) {
		vector_entry_t got = vector_get(v, i);
		if (strcmp(got.name, token) == 0) {
			got.count++;
			vector_set(v, i, got);
			return;
		}
	}
	int sl = strlen(token);
	char *chr = malloc((sl + MALLOC_OFFSET) * sizeof(char));
	assert(chr);
	assert(strcpy(chr, token) == chr);
	assert(strlen(chr) == sl);
	vector_entry_t elem;
	elem.name = chr;
	elem.count = 1;
	vector_push_back(v, elem);
}

int get_name(FILE * f, int index, vector_t v)
{
	// return: 0: success; -1: EOF
	char line[MAX_LINE_EXTRA];
	if (read_line(line, f) == -1)
		return -1;
	char *str = line;
	char *token;
	assert(index >= 0);	// internal assert
	for (int i = 0; i <= index; i++) {
		assert((token = csv_tok(str)));
		str = NULL;
	}
	add_name(token, v);
	return 0;
}

int top_name(vector_t v)
{
	// Print the most frequent name
	// Return 0: top found; -1: not found
	// Setting count to -1 means already processed
	if (!vector_size(v))
		return -1;
	int top_index = 0;
	vector_entry_t top_entry = vector_get(v, 0);
	for (int i = 1; i < vector_size(v); i++) {
		vector_entry_t next_entry = vector_get(v, i);
		if (next_entry.count > top_entry.count) {
			top_index = i;
			top_entry = next_entry;
		}
	}
	if (top_entry.count == -1)
		return -1;	// Not found
	printf("%s: %d\n", top_entry.name, top_entry.count);
	top_entry.count = -1;
	vector_set(v, top_index, top_entry);
	return 0;
}

int main(int argc, char *argv[])
{
	assert(argc == 2);
	FILE *f = fopen(argv[1], "r");
	assert(f);
	int index = col_index(f);
	vector_t counter = vector_new();
	for (int i = 0;; i++) {
		assert(i < MAX_NLINE);
		if (get_name(f, index, counter) == -1)
			break;
	}
	for (int i = 0; i < 10; i++)
		if (top_name(counter) == -1)
			break;
	return 0;
}
