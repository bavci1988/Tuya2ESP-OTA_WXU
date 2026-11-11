/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_LIST_H_
#define _SYS_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "compiler.h"
#include "sys/defs.h"

/* Backward compatibility for old macro definitions */
#define XR_LIST_BACKWARD_COMPATIBILITY 1

#if XR_LIST_BACKWARD_COMPATIBILITY

typedef struct list_head {
	struct list_head *next;
	struct list_head *prev;
} xr_list_t;

#else /* XR_LIST_BACKWARD_COMPATIBILITY */

typedef struct xr_list {
	struct xr_list *next;
	struct xr_list *prev;
} xr_list_t;

#endif /* XR_LIST_BACKWARD_COMPATIBILITY */

#define XR_LIST_HEAD(l) { &(l), &(l) }

#define XR_LIST_DEFINE(l) \
    xr_list_t l = { &(l), &(l) }

static __always_inline void _XR_List_InsertNode(xr_list_t *insert_node,
                                                xr_list_t *left,
                                                xr_list_t *right)
{
	right->prev = insert_node;
	insert_node->next = right;
	insert_node->prev = left;
	left->next = insert_node;
}

static __always_inline void _XR_List_DelNode(xr_list_t *left, xr_list_t *right)
{
	right->prev = left;
	left->next = right;
}

static __always_inline void _XR_List_InsertList(xr_list_t *insert_list,
                                                xr_list_t *left,
                                                xr_list_t *right)
{
	/* link the last node of 'inserted_list' to 'right' */
	insert_list->next->prev = left;
	left->next = insert_list->next;
	/* link the first node of 'inserted_list' to 'left' */
	insert_list->prev->next = right;
	right->prev = insert_list->prev;
}

#define _XR_LIST_IS_EMPTY(list_head) ((list_head)->next == (list_head))
#define _XR_LIST_NODE_IS_FIRST(node, list_head) ((list_head)->next == (node))
#define _XR_LIST_NODE_IS_LAST(node, list_head) ((node)->next == (list_head))
#define _XR_LIST_IS_ONLY_ONE_NODE(list) \
     (!_XR_LIST_IS_EMPTY(list) && ((list)->next == (list)->prev))

/**
 * @brief Initialize a doubly linked list
 */
#define XR_List_Init(list)          \
    do {                            \
        (list)->prev = (list);      \
        (list)->next = (list);      \
    } while (0)

/**
 * @brief Insert a node from the head of the double linked list
 * @param[in] node new node to be inserted
 * @param[in] list dl-list where the new node is inserted
 * @return None
 */
static __always_inline void XR_List_Insert(xr_list_t *node, xr_list_t *list)
{
	_XR_List_InsertNode(node, list, list->next);
}

/**
 * @brief Insert a node from the tail of the double linked list
 * @param[in] node New node to be inserted
 * @param[in] list dl-list where the new node is inserted
 * @return None
 */
static __always_inline void XR_List_InsertTail(xr_list_t *node, xr_list_t *list)
{
	_XR_List_InsertNode(node, list->prev, list);
}

/**
 * @brief Delete a specified node in the double linked list
 * @param[in] node Node to be deleted
 * @return None
 *
 * @note The deleted node will be initialized
 */
static __always_inline void XR_List_Del(xr_list_t *node)
{
	_XR_List_DelNode(node->prev, node->next);
	XR_List_Init(node);
}

/**
 * @brief Replace the old node in the double linked list
 * @param[in] new_node The new node in this double linked list
 * @param[in] old_node Old node to be replaced
 * @return None
 */
static __always_inline void XR_List_Replace(xr_list_t *new_node, xr_list_t *old_node)
{
	new_node->next = old_node->next;
	old_node->next->prev = new_node;
	new_node->prev = old_node->prev;
	old_node->prev->next = new_node;
}

/**
 * @brief Move a node to the head of the double linked list
 * @param[in] node Node to be moved
 * @param[in] list Double linked list of this node
 * @return None
 */
static __always_inline void XR_List_Move(xr_list_t *node, xr_list_t *list)
{
	_XR_List_DelNode(node->prev, node->next);
	XR_List_Insert(node, list);
}

/**
 * @brief Move a node to the tail of the double linked list
 * @param[in] node Node to be moved
 * @param[in] list Double linked list of this node
 * @return None
 */
static __always_inline void XR_List_MoveTail(xr_list_t *node, xr_list_t *list)
{
	_XR_List_DelNode(node->prev, node->next);
	XR_List_InsertTail(node, list);
}

/**
 * @brief Judge whether a node is the first one in the double linked list
 * @param[in] node Nodes to be judged
 * @param[in] list Double linked list of nodes
 * @retval Status of check:
 *         1: this node is the first
 *         0: this node is not the first
 */
static __always_inline int XR_List_IsFirst(const xr_list_t *node, const xr_list_t *list)
{
	return _XR_LIST_NODE_IS_FIRST(node, list);
}

/**
 * @brief Judge whether a node is the last one in the double linked list
 * @param[in] node Nodes to be judged
 * @param[in] list Double linked list of nodes
 * @retval Status of check:
 *         1: this node is the last
 *         0: this node is not the last
 */
static __always_inline int XR_List_IsLast(const xr_list_t *node, const xr_list_t *list)
{
	return _XR_LIST_NODE_IS_LAST(node, list);
}

/**
 * @brief Check whether a double linked list is empty
 * @param[in] list List to be checked
 * @retval Status of check:
 *         1: this list is empty
 *         0: this list is not empty
 */
static __always_inline int XR_List_IsEmpty(const xr_list_t *list)
{
	return _XR_LIST_IS_EMPTY(list);
}

/**
 * @brief Check whether the double linked list has only one element
 * @param[in] list List to be checked
 * @retval Status of check:
 *         1: This double linked list has only one node
 *         0: This double linked list is empty or has multiple nodes
 */
static __always_inline int XR_List_IsOnlyOne(const xr_list_t *list)
{
	return _XR_LIST_IS_ONLY_ONE_NODE(list);
}

/**
 * @brief Insert a double linked list from the head to the specified double linked list
 * @param[in] ori_list Dest double linked list
 * @param[in] insert_list double linked list to be inserted
 * @return None
 */
static __always_inline void XR_List_Concat(xr_list_t *ori_list, xr_list_t *insert_list)
{
	if (_XR_LIST_IS_EMPTY(insert_list))
		return;
	_XR_List_InsertList(insert_list, ori_list, ori_list->next);
}

/**
 * @brief Insert a double linked list from the tail to the specified double linked list
 * @param[in] ori_list Dest double linked list
 * @param[in] insert_list Double linked list to be inserted
 * @return None
 */
static __always_inline void XR_List_ConcatTail(xr_list_t *ori_list, xr_list_t *insert_list)
{
	if (_XR_LIST_IS_EMPTY(insert_list))
		return;
	_XR_List_InsertList(insert_list, ori_list->prev, ori_list);
}

 /**
 * @brief Split all nodes before the specified node into a sub double linked list
 * @param[in] node Specified node
 * @param[in] ori_list Source double linked list to be split
 * @param[in] sub_list Sub double linked list generated by splitting
 * @return None
 *
 * @note From ori_list first element to param 'node', all elements are split into sub_list,
 *       and the remaining elements are refreshed to ori_list.
 *       The param 'node' must to be an element node, not ori_list head!
 *
 * @examples are as follows:
 *   ori_list is: element1 <-> element2 <-> element3 <-> element4 <-> element5
 *   sub_list is: empty
 *
 *   if param 'node' is element3, the split results are as follows:
 *   ori_list is: element4 <-> element5
 *   sub_list is: element1 <-> element2 <-> element3
 */
static __always_inline void XR_List_Split(xr_list_t *node, xr_list_t *ori_list,
                                          xr_list_t *sub_list)
{
	/* record the first new node for the origin list */
	xr_list_t *ori_first = node->next;

	/* check assert */
	if ((_XR_LIST_IS_EMPTY(ori_list) || node == ori_list) ||
	    (_XR_LIST_IS_ONLY_ONE_NODE(ori_list) && !_XR_LIST_NODE_IS_FIRST(node, ori_list)))
		return;

	/* construct sublist */
	sub_list->next = ori_list->next;
	ori_list->next->prev = sub_list;
	node->next = sub_list;
	sub_list->prev = node;

	/* update origin list */
	ori_list->next = ori_first;
	ori_first->prev = ori_list;
}

/**
 * @brief Get the pointer to a structure that contains a doubly linked list
 * @param[in] item_ptr The pointer of struct xr_dl_list_s node
 * @param[in] type Structure type
 * @param[in] field Field name of the doubly linked list in the structure
 */
#define XR_LIST_ENTRY(item_ptr, type, field)       \
    XR_CONTAINER_OF(item_ptr, type, field)

/**
 * @brief Get the the last entry from a list
 * @param[in] list_ptr The double linked list head
 * @param[in] type Structure type of this entry
 * @param[in] field Field name of the doubly linked list in the structure
 */
#define XR_LIST_LAST_ENTRY(list_ptr, type, field)  \
    XR_LIST_ENTRY((list_ptr)->prev, type, field)

/**
 * @brief Get the the first entry from a list
 * @param[in] list_ptr The double linked list head
 * @param[in] type Structure type of this entry
 * @param[in] field Field name of the doubly linked list in the structure
 */
#define XR_LIST_FIRST_ENTRY(list_ptr, type, field) \
    XR_LIST_ENTRY((list_ptr)->next, type, field)

/**
 * @brief Get the structure pointer containing the first member
 *        of the linked list.
 * @param[in] list_ptr The double linked list head
 * @param[in] type Structure type of this entry
 * @param[in] field Field name of the doubly linked list in the structure
 *
 * @note If the first member of the linked list does not exist,
 *       null will be returned
 */
#define XR_LIST_FIRST_ENTRY_OR_NULL(list_ptr, type, field) \
    (_XR_LIST_IS_EMPTY(list_ptr) ? NULL : XR_LIST_FIRST_ENTRY(list_ptr, type, field))


/**
 * @brief iterate a double linked list in positive order from the head
 * @param[in] cur A xr_list_t pointer, used to point to the node traversed
 * @param[in] list Iterated double linked list header
 *
 * @note XR_List_Del() nodes during iteration is not recommended,
 *       because it is unsafe, if you need to do so, you can refer to
 *       the use of XR_LIST_FOR_EACH_SAFE()
 */
#define XR_LIST_FOR_EACH(cur, list)       \
    for (cur = (list)->next;              \
         cur != (list);                   \
         cur = cur->next)

/**
 * @brief iterate a double linked list in reverse order from the tail
 * @param[in] cur A xr_list_t pointer, used to point to the node traversed
 * @param[in] list Iterated double linked list header
 *
 * @note XR_List_Del() nodes during iteration is not recommended,
 *       because it is unsafe, if you need to do so, you can refer to
 *       the use of XR_LIST_FOR_EACH_PREV_SAFE()
 */
#define XR_LIST_FOR_EACH_PREV(cur, list)  \
    for (cur = (list)->prev;              \
         cur != (list);                   \
         cur = cur->prev)

/**
 * @brief Safely iterate a double linked list in positive order from the head
 * @param[in] cur A xr_list_t pointer, used to point to the node traversed
 * @param[in] tmp A xr_list_t pointer, Used to cache the next node during iteration,
 *                Prevent the loss of the next node due to deletion during iteration
 * @param[in] list Iterated double linked list header
 */
#define XR_LIST_FOR_EACH_SAFE(cur, tmp, list)     \
    for (cur = (list)->next, tmp = cur->next;     \
         cur != (list);                           \
         cur = tmp, tmp = cur->next)

/**
 * @brief Safely iterate a double linked list in reverse order from the tail
 * @param[in] cur A xr_list_t pointer, used to point to the node traversed
 * @param[in] tmp A xr_list_t pointer, Used to cache the next node during iteration,
 *                Prevent the loss of the next node due to deletion during iteration
 * @param[in] list Iterated double linked list header
 */
#define XR_LIST_FOR_EACH_PREV_SAFE(cur, tmp, list) \
    for (cur = (list)->prev, tmp = cur->prev;      \
         cur != (list);                            \
         cur = tmp, tmp = cur->prev)

/**
 * @brief Iterate the structure entity of the embedded double linked list
 *        in positive order from the head
 * @param[in] list Iterated double linked list header
 * @param[in] entry The pointer to a entity type
 * @param[in] type Structure type of this entry
 * @param[in] field Field name of the doubly linked list in the structure
 */
#define XR_LIST_FOR_EACH_ENTRY(list, entry, type, field)           \
    for (entry = XR_LIST_ENTRY((list)->next, type, field);         \
         &entry->field != (list);                                  \
         entry = XR_LIST_ENTRY((entry)->field.next, type, field))

/**
 * @brief Iterate the structure entity of the embedded double linked list
 *        in reverse order from the tail
 * @param[in] list Iterated double linked list header
 * @param[in] entry The pointer to a entity type
 * @param[in] type Structure type of this entry
 * @param[in] field Field name of the doubly linked list in the structure
 */
#define XR_LIST_FOR_EACH_ENTRY_REVERSE(list, entry, type, field)    \
    for (entry = XR_LIST_ENTRY((list)->prev, type, field);          \
         &entry->field != (list);                                   \
         entry = XR_LIST_ENTRY((entry)->field.prev, type, field))

/**
 * @brief Safely iterate the structure entity of the embedded double linked list
 *        in positive order from the head
 * @param[in] list Iterated double linked list header
 * @param[in] entry The pointer to a entity type
 * @param[in] tmp Another xr_list_t pointer to use as temporary storage
 * @param[in] type Structure type of this entry
 * @param[in] field Field name of the doubly linked list in the structure
 */
#define XR_LIST_FOR_EACH_ENTRY_SAFE(list, entry, tmp, type, field)           \
    for (entry = XR_LIST_ENTRY((list)->next, type, field),                   \
         tmp = XR_LIST_ENTRY((entry)->field.next, type, field);              \
         &entry->field != (list);                                            \
         entry = tmp, tmp = XR_LIST_ENTRY((entry)->field.next, type, field))

/**
 * @brief Safely iterate the structure entity of the embedded double linked list
 *        in reverse order from the tail
 * @param[in] list Iterated double linked list header
 * @param[in] entry The pointer to a entity type
 * @param[in] tmp Another xr_list_t pointer to use as temporary storage
 * @param[in] type Structure type of this entry
 * @param[in] field Field name of the doubly linked list in the structure
 */
#define XR_LIST_FOR_EACH_ENTRY_SAFE_REVERSE(list, entry, tmp, type, field)   \
    for (entry = XR_LIST_ENTRY((list)->prev, type, field),                   \
        tmp = XR_LIST_ENTRY((entry)->field.prev, type, field);               \
        &entry->field != (list);                                             \
        entry = tmp, tmp = XR_LIST_ENTRY((entry)->field.prev, type, field))

/**
 * @brief Prepare a structure entity node to use
 * @param[in] list Double linked list header
 * @param[in] entry The pointer to a entity type
 * @param[in] type Structure type of this entry
 * @param[in] field Field name of the doubly linked list in the structure
 *
 * @note If the 'entry' is not null, the value of the 'entry' is itself,
 *       if the 'entry' is null, a virtual entry pointer is forcibly extended
 *       from the list header.
 *       It generally used with XR_LIST_FOR_EACH_ENTRY_CONTINUE()
 */
#define XR_LIST_PREPARE_ENTRY(list, entry, type, field)             \
    ((!entry) ? XR_LIST_ENTRY(list, type, field) : (entry))

/**
 * @brief Iterate the structure entity of the embedded double linked list
 *        in positive order from the specified entity node
 * @param[in] list Iterated double linked list header
 * @param[in] entry The pointer to a entity type
 * @param[in] type Structure type of this entry
 * @param[in] field Field name of the doubly linked list in the structure
 */
#define XR_LIST_FOR_EACH_ENTRY_CONTINUE(list, entry, type, field)   \
    for (entry = XR_LIST_ENTRY((entry)->field.next, type, field);   \
         &entry->field != (list);                                   \
         entry = XR_LIST_ENTRY((entry)->field.next, type, field))

/**
 * @brief Iterate the structure entity of the embedded double linked list
 *        in reverse order from the specified entity node
 * @param[in] list Iterated double linked list header
 * @param[in] entry The pointer to a entity type
 * @param[in] type Structure type of this entry
 * @param[in] field Field name of the doubly linked list in the structure
 */
#define XR_LIST_FOR_EACH_ENTRY_CONTINUE_REVERSE(list, entry, type, field)   \
    for (entry = XR_LIST_ENTRY((entry)->field.prev, type, field);           \
         &entry->field != (list);                                           \
         entry = XR_LIST_ENTRY((entry)->field.prev, type, field))

/**
 * @brief Safely iterate the structure entity of the embedded double linked list
 *        in positive order from the specified entity node
 * @param[in] list Iterated double linked list header
 * @param[in] entry The pointer to a entity type
 * @param[in] type Structure type of this entry
 * @param[in] field Field name of the doubly linked list in the structure
 */
#define XR_LIST_FOR_EACH_ENTRY_SAFE_CONTINUE(list, entry, tmp, type, field) \
    for (entry = XR_LIST_ENTRY((entry)->field.next, type, field),           \
         tmp = XR_LIST_ENTRY((entry)->field.next, type, field);             \
         &entry->field != (list);                                           \
         entry = tmp, tmp = XR_LIST_ENTRY((tmp)->field.next, type, field))

#if XR_LIST_BACKWARD_COMPATIBILITY

/* old macro definitions for backward compatibility */
#define list_for_each(pos, head)              XR_LIST_FOR_EACH(pos, head)
#define list_for_each_prev(pos, head)         XR_LIST_FOR_EACH_PREV(pos, head)
#define list_for_each_safe(pos, n, head)      XR_LIST_FOR_EACH_SAFE(pos, n, head)
#define list_for_each_prev_safe(pos, n, head) XR_LIST_FOR_EACH_PREV_SAFE(pos, n, head)
#define list_entry(ptr, type, member)         XR_LIST_ENTRY(ptr, type, member)
#define list_first_entry(ptr, type, member)   XR_LIST_FIRST_ENTRY(ptr, type, member)
#define list_last_entry(ptr, type, member)    XR_LIST_LAST_ENTRY(ptr, type, member)

#define list_first_entry_or_null(ptr, type, member)                             \
    XR_LIST_FIRST_ENTRY_OR_NULL(ptr, type, member)

#define list_next_entry(pos, member)                                            \
    XR_LIST_ENTRY((pos)->member.next, typeof(*(pos)), member)

#define list_prev_entry(pos, member)                                            \
    XR_LIST_ENTRY((pos)->member.prev, typeof(*(pos)), member)

#define list_safe_reset_next(pos, n, member)                                    \
    (n = XR_LIST_ENTRY((pos)->member.next, typeof(*(pos)), member))

#define list_for_each_entry(pos, head, member)                                  \
    XR_LIST_FOR_EACH_ENTRY(head, pos, typeof(*(pos)), member)

#define list_for_each_entry_reverse(pos, head, member)                          \
    XR_LIST_FOR_EACH_ENTRY_REVERSE(head, pos, typeof(*(pos)), member)

#define list_for_each_entry_safe(pos, n, head, member)                          \
    XR_LIST_FOR_EACH_ENTRY_SAFE(head, pos, n, typeof(*(pos)), member)

#define list_for_each_entry_safe_reverse(pos, n, head, member)                  \
    XR_LIST_FOR_EACH_ENTRY_SAFE_REVERSE(head, pos, n, typeof(*(pos)), member)

#define list_prepare_entry(pos, head, member)                                   \
    XR_LIST_PREPARE_ENTRY(head, pos, typeof(*(pos)), member)

#define list_for_each_entry_continue(pos, head, member)                         \
    XR_LIST_FOR_EACH_ENTRY_CONTINUE(head, pos, typeof(*(pos)), member)

#define list_for_each_entry_continue_reverse(pos, head, member)                 \
    XR_LIST_FOR_EACH_ENTRY_CONTINUE_REVERSE(head, pos, typeof(*(pos)), member)

#define list_for_each_entry_safe_continue(pos, n, head, member)                 \
    XR_LIST_FOR_EACH_ENTRY_SAFE_CONTINUE(head, pos, n, typeof(*(pos)), member)

#define list_for_each_entry_from(pos, head, member)                             \
    for (; &pos->member != (head);                                              \
         pos = XR_LIST_ENTRY((pos)->member.next, typeof(*(pos)), member)

#define list_for_each_entry_safe_from(pos, n, head, member)                     \
    for (n = XR_LIST_ENTRY((pos)->member.next, typeof(*(pos)), member);         \
         &pos->member != (head);                                                \
         pos = n, n = XR_LIST_ENTRY((n)->member.next, typeof(*(n)), member)

#define LIST_HEAD_INIT(name)                  XR_LIST_HEAD(name)
#define LIST_HEAD_DEF(name)                   XR_LIST_DEFINE(name)
#define INIT_LIST_HEAD(list)                  XR_List_Init(list)
#define list_add(newl, head)                  XR_List_Insert(newl, head)
#define list_add_tail(newl, head)             XR_List_InsertTail(newl, head)
#define list_del(entry)                       XR_List_Del(entry)
#define list_replace(old, newl)               XR_List_Replace(newl, old)
#define list_del_init(entry)                  list_del(entry)
#define list_move(list, head)                 XR_List_Move(list, head)
#define list_move_tail(list, head)            XR_List_MoveTail(list, head)
#define list_is_last(list, head)              XR_List_IsLast(list, head)
#define list_empty(head)                      XR_List_IsEmpty(head)
#define list_empty_careful(head)              list_empty(head)
#define list_is_singular(head)                XR_List_IsOnlyOne(head)
#define list_cut_position(list, head, entry)  XR_List_Split(entry, head, list)
#define list_splice(list, head)               XR_List_Concat(head, list)
#define list_splice_tail(list, head)          XR_List_ConcatTail(head, list)

/* move first node of 'list' to tail */
#define list_rotate_left(list)                \
    do {                                      \
        if (XR_List_IsEmpty(list))            \
            break;                            \
        XR_List_MoveTail(list->next, list);   \
    } while (0)                               \

#define list_splice_init(list, head)          \
    do {                                      \
        if (XR_List_IsEmpty(list))            \
            break;                            \
        XR_List_Concat(head, list);           \
        XR_List_Init(list);                   \
    } while (0)                               \

#define list_splice_tail_init(list, head)     \
    do {                                      \
        if (XR_List_IsEmpty(list))            \
            break;                            \
        XR_List_ConcatTail(head, list);       \
        XR_List_Init(list);                   \
    } while (0)                               \

#define list_replace_init(old, newl)          \
    do {                                      \
        XR_List_Replace(newl, old);           \
        XR_List_Init(old);                    \
    } while (0)                               \

#endif /* XR_LIST_BACKWARD_COMPATIBILITY */

#ifdef __cplusplus
}
#endif

#endif /* _SYS_LIST_H_ */
