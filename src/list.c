//  Original pkgutils:
//  Copyright (c) 2000-2005 Per Liden
//
//  That C-rewrite:
//  Copyright (c) 2006 Anton Vorontsov <cbou@mail.ru>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
//  USA.

#include <pkgutils/list.h>
#include <stdlib.h>
#ifdef LIST_ABORT_ON_ERROR
#include <stdio.h>
#endif

void *list_malloc_failed() {
	#ifdef LIST_ABORT_ON_ERROR
	fputs("list: malloc() failed\n", stderr);
	abort();
	#endif
	return NULL;
}

int list_init(list_t *list) {
	list->head = malloc(sizeof(list_entry_t));
	if (!list->head) {
		list_malloc_failed();
		return -1;
	}
	list->tail = malloc(sizeof(list_entry_t));
	if (!list->tail) {
		free(list->head);
		list_malloc_failed();
		return -1;
	}

	list->head->next = list->tail;
	list->tail->prev = list->head;

	list->head->prev = NULL;
	list->tail->next = NULL;
	
	list->size = 0;

	return 0;
}

void list_free(list_t *list) {
	list_entry_t *i, *j;
	i = list->head->next;
	while (i->next) {
		j = i;
		i = i->next;
		free(j);
	}
	free(list->head);
	free(list->tail);
	list->size = (size_t)-1;
	return;
}

list_entry_t *list_insert_after(list_t *list, list_entry_t *i, void *data) {
	list_entry_t *new_entry = malloc(sizeof(list_entry_t));
	if (!new_entry) return list_malloc_failed();

	new_entry->next = i->next;
	new_entry->prev = i;

	i->next->prev = new_entry;
	i->next = new_entry;

	new_entry->data = data;
	list->size++;

	return new_entry;
}

list_entry_t *list_insert_before(list_t *list, list_entry_t *i, void *data) {
	list_entry_t *new_entry = malloc(sizeof(list_entry_t));
	if (!new_entry) return list_malloc_failed();

	new_entry->next = i;
	new_entry->prev = i->prev;

	i->prev->next = new_entry;
	i->prev = new_entry;

	new_entry->data = data;
	list->size++;

	return new_entry;
}

list_entry_t *list_prepend(list_t *list, void *data) {
	return list_insert_after(list, list->head, data);
}

list_entry_t *list_append(list_t *list, void *data) {
	return list_insert_before(list, list->tail, data);
}

void list_delete(list_t *list, list_entry_t *i) {
	i->next->prev = i->prev;
	i->prev->next = i->next;
	list->size--;
	free(i);
	return;
}
