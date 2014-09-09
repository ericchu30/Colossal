/* The MIT License

   Copyright (C) 2011, 2012 Zilong Tan (eric.zltan@gmail.com)

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#include "tree.h"

struct tree_root_np *
tree_search(struct tree_root_np *entry,
	    int (*compare)(const void *, const void *),
	    struct tree_root_np *root)
{
	int sgn;

	while (root != NULL) {
		sgn = compare(entry, root);
		if (sgn == 0)
			return root;
		root = root->ptrs[sgn > 0];
	}
	return root;
}

struct tree_root_np *
tree_min(struct tree_root_np *root)
{
	if (root != NULL) {
		while (LEFT_PTR(root) != NULL)
			root = LEFT_PTR(root);
	}
	return root;
}

struct tree_root_np *
tree_max(struct tree_root_np *root)
{
	if (root != NULL) {
		while (RIGHT_PTR(root) != NULL)
			root = RIGHT_PTR(root);
	}
	return root;
}

struct tree_root *
tree_successor(struct tree_root *root)
{
	struct tree_root *succ = NULL;

	if (root != NULL) {
		if (RIGHT_PTR(root) != NULL)
			return (struct tree_root *)TREE_MIN(RIGHT_PTR(root));
		succ = PARENT_PTR(root);
		while (succ != NULL && root == RIGHT_PTR(succ)) {
			root = succ;
			succ = PARENT_PTR(succ);
		}
	}
	return succ;
}

struct tree_root *
tree_predecessor(struct tree_root *root)
{
	struct tree_root *pred = NULL;

	if (root != NULL) {
		if (LEFT_PTR(root) != NULL)
			return (struct tree_root *)TREE_MAX(LEFT_PTR(root));
		pred = PARENT_PTR(root);
		while (pred != NULL && root == LEFT_PTR(pred)) {
			root = pred;
			pred = PARENT_PTR(pred);
		}
	}
	return pred;
}

static inline void
__rotate_left(struct tree_root *entry, struct tree_root **root)
{
	struct tree_root *right;

	right = RIGHT_PTR(entry);
	RIGHT_PTR(entry) = LEFT_PTR(right);
	if (LEFT_PTR(right) != NULL)
		PARENT_PTR(LEFT_PTR(right)) = entry;
	PARENT_PTR(right) = PARENT_PTR(entry);
	if (PARENT_PTR(entry) == NULL)
		*root = right;
	else
		PARENT_PTR(entry)->ptrs[entry == RIGHT_PTR(PARENT_PTR(entry))] = right;
	LEFT_PTR(right) = entry;
	PARENT_PTR(entry) = right;
}

static inline void
__rotate_right(struct tree_root *entry, struct tree_root **root)
{
	struct tree_root *left;

	left = LEFT_PTR(entry);
	LEFT_PTR(entry) = RIGHT_PTR(left);
	if (RIGHT_PTR(left) != NULL)
		PARENT_PTR(RIGHT_PTR(left)) = entry;
	PARENT_PTR(left) = PARENT_PTR(entry);
	if (PARENT_PTR(entry) == NULL)
		*root = left;
	else
		PARENT_PTR(entry)->ptrs[entry == RIGHT_PTR(PARENT_PTR(entry))] = left;
	RIGHT_PTR(left) = entry;
	PARENT_PTR(entry) = left;
}

void tree_add(struct tree_root *new,
	      int (*compare)(const void *, const void *),
	      struct tree_root **root)
{
	int sgn = 0;
	struct tree_root *next = *root;
	struct tree_root *cur  = NULL;

	INIT_TREE_ROOT(new);

	while (next != NULL) {
		cur  = next;
		sgn  = compare(new, next);
		next = next->ptrs[sgn > 0];
	}

	PARENT_PTR(new) = cur;
	if (cur == NULL)
		*root = new;
	else
		cur->ptrs[sgn > 0] = new;
}

struct tree_root *
tree_map(struct tree_root *new,
	 int (*compare)(const void *, const void *),
	 struct tree_root **root)
{
	int sgn = 0;
	struct tree_root *next = *root;
	struct tree_root *cur  = NULL;

	INIT_TREE_ROOT(new);

	while (next != NULL) {
		cur = next;
		sgn = compare(new, next);
		if (sgn == 0)
			return next;
		next = next->ptrs[sgn > 0];
	}

	PARENT_PTR(new) = cur;
	if (cur == NULL)
		*root = new;
	else
		cur->ptrs[sgn > 0] = new;

	return new;
}

void
tree_del(struct tree_root *entry, struct tree_root **root)
{
	struct tree_root *child;
	struct tree_root *succ;

	if (LEFT_PTR(entry) == NULL || RIGHT_PTR(entry) == NULL)
		succ = entry;
	else
		succ = TREE_SUCCESSOR(entry);

	child = LEFT_PTR(succ)? LEFT_PTR(succ): RIGHT_PTR(succ);
	if (child != NULL)
		PARENT_PTR(child) = PARENT_PTR(succ);
	if (PARENT_PTR(succ) == NULL)
		*root = child;
	else
		PARENT_PTR(succ)->ptrs[succ == RIGHT_PTR(PARENT_PTR(succ))] = child;

	if (succ != entry) {
		LEFT_PTR(succ) = LEFT_PTR(entry);
		if (LEFT_PTR(entry))
			PARENT_PTR(LEFT_PTR(entry)) = succ;
		RIGHT_PTR(succ) = RIGHT_PTR(entry);
		if (RIGHT_PTR(entry))
			PARENT_PTR(RIGHT_PTR(entry)) = succ;
		PARENT_PTR(succ) = PARENT_PTR(entry);
		if (PARENT_PTR(entry) == NULL)
			*root = succ;
		else
			PARENT_PTR(entry)->ptrs[entry == RIGHT_PTR(PARENT_PTR(entry))] = succ;
	}
}

#define SPLAY_ROTATE_RIGHT(entry, tmp) do {				\
		LEFT_PTR(entry) = RIGHT_PTR(tmp);			\
		if (RIGHT_PTR(tmp)) PARENT_PTR(RIGHT_PTR(tmp)) = (entry); \
		RIGHT_PTR(tmp) = (entry);				\
		PARENT_PTR(tmp) = PARENT_PTR(entry);			\
		PARENT_PTR(entry) = (tmp);				\
		(entry) = (tmp);					\
	} while (0)

#define SPLAY_ROTATE_LEFT(entry, tmp) do {				\
		RIGHT_PTR(entry) = LEFT_PTR(tmp);			\
		if (LEFT_PTR(tmp)) PARENT_PTR(LEFT_PTR(tmp)) = (entry);	\
		LEFT_PTR(tmp) = (entry);				\
		PARENT_PTR(tmp) = PARENT_PTR(entry);			\
		PARENT_PTR(entry) = (tmp);				\
		(entry) = (tmp);					\
	} while (0)

#define SPLAY_LINK_RIGHT(entry, large) do {	\
		LEFT_PTR(large) = (entry);	\
		PARENT_PTR(entry) = (large);	\
		(large) = (entry);		\
		(entry) = LEFT_PTR(entry);	\
	} while (0)

#define SPLAY_LINK_LEFT(entry, small) do {	\
		RIGHT_PTR(small) = (entry);	\
		PARENT_PTR(entry) = (small);	\
		(small) = (entry);		\
		(entry) = RIGHT_PTR(entry);	\
	} while (0)

#define SPLAY_ASSEMBLE(head, node, small, large) do {		\
		RIGHT_PTR(small) = LEFT_PTR(head);		\
		if (LEFT_PTR(head))				\
			PARENT_PTR(LEFT_PTR(head)) = (small);	\
		LEFT_PTR(large) = RIGHT_PTR(head);		\
		if (RIGHT_PTR(head))				\
			PARENT_PTR(RIGHT_PTR(head)) = (large);	\
		LEFT_PTR(head) = RIGHT_PTR(node);		\
		if (RIGHT_PTR(node))				\
			PARENT_PTR(RIGHT_PTR(node)) = (head);	\
		RIGHT_PTR(head) = LEFT_PTR(node);		\
		if (LEFT_PTR(node))				\
			PARENT_PTR(LEFT_PTR(node)) = (head);	\
	} while (0)

#define SPLAY_ROTATE_RIGHT_NP(entry, tmp) do {		\
		LEFT_PTR(entry) = RIGHT_PTR(tmp);	\
		RIGHT_PTR(tmp) = (entry);		\
		(entry) = (tmp);			\
	} while (0)

#define SPLAY_ROTATE_LEFT_NP(entry, tmp) do {	\
		RIGHT_PTR(entry) = LEFT_PTR(tmp);	\
		LEFT_PTR(tmp) = (entry);	\
		(entry) = (tmp);		\
	} while (0)

#define SPLAY_LINK_RIGHT_NP(entry, large) do {	\
		LEFT_PTR(large) = (entry);	\
		(large) = (entry);		\
		(entry) = LEFT_PTR(entry);	\
	} while (0)

#define SPLAY_LINK_LEFT_NP(entry, small) do {	\
		RIGHT_PTR(small) = (entry);	\
		(small) = (entry);		\
		(entry) = RIGHT_PTR(entry);	\
	} while (0)

#define SPLAY_ASSEMBLE_NP(head, node, small, large) do {	\
		RIGHT_PTR(small) = LEFT_PTR(head);		\
		LEFT_PTR(large) = RIGHT_PTR(head);		\
		LEFT_PTR(head) = RIGHT_PTR(node);		\
		RIGHT_PTR(head) = LEFT_PTR(node);		\
	} while (0)

struct tree_root *
splay_search(struct tree_root *entry,
	     int (*compare)(const void *, const void *),
	     struct tree_root **root)
{
	int cmp;
	TREE_ROOT(node);  /* node for assembly use */
	struct tree_root *small, *large, *head, *tmp;

	head = *root;
	small = large = &node;

	while ((cmp = compare(entry, head)) != 0) {
		if (cmp < 0) {
			tmp = LEFT_PTR(head);
			if (tmp == NULL)
				break;
			if (compare(entry, tmp) < 0) {
				SPLAY_ROTATE_RIGHT(head, tmp);
				if (LEFT_PTR(head) == NULL)
					break;
			}
			SPLAY_LINK_RIGHT(head, large);
		} else {
			tmp = RIGHT_PTR(head);
			if (tmp == NULL)
				break;
			if (compare(entry, tmp) > 0) {
				SPLAY_ROTATE_LEFT(head, tmp);
				if (RIGHT_PTR(head) == NULL)
					break;
			}
			SPLAY_LINK_LEFT(head, small);
		}
	}
	PARENT_PTR(head) = NULL;
	SPLAY_ASSEMBLE(head, &node, small, large);
	*root = head;

	if (cmp != 0)
		return NULL;

	return head;
}

struct tree_root *
splay_map(struct tree_root *new,
	  int (*compare)(const void *, const void *),
	  struct tree_root **root)
{
	int cmp;
	TREE_ROOT(node);  /* node for assembly use */
	struct tree_root *small, *large, *head, *tmp;

	INIT_TREE_ROOT(new);
	small = large = &node;
	head = *root;

	while (head && (cmp = compare(new, head)) != 0) {
		if (cmp < 0) {
			tmp = LEFT_PTR(head);
			if (tmp == NULL) {
				/* zig */
				SPLAY_LINK_RIGHT(head, large);
				break;
			}
			cmp = compare(new, tmp);
			if (cmp < 0) {
				/* zig-zig */
				SPLAY_ROTATE_RIGHT(head, tmp);
				SPLAY_LINK_RIGHT(head, large);
			} else if (cmp > 0) {
				/* zig-zag */
				SPLAY_LINK_RIGHT(head, large);
				SPLAY_LINK_LEFT(head, small);
			} else {
				/* zig */
				SPLAY_LINK_RIGHT(head, large);
				break;
			}
		} else {
			tmp = RIGHT_PTR(head);
			if (tmp == NULL) {
				/* zag */
				SPLAY_LINK_LEFT(head, small);
				break;
			}
			cmp = compare(new, tmp);
			if (cmp > 0) {
				/* zag-zag */
				SPLAY_ROTATE_LEFT(head, tmp);
				SPLAY_LINK_LEFT(head, small);
			} else if (cmp < 0) {
				/* zag-zig */
				SPLAY_LINK_LEFT(head, small);
				SPLAY_LINK_RIGHT(head, large);
			} else {
				/* zag */
				SPLAY_LINK_LEFT(head, small);
				break;
			}
		}
	}

	if (head == NULL)
		head = new;

	PARENT_PTR(head) = NULL;

	SPLAY_ASSEMBLE(head, &node, small, large);

	*root = head;

	return head;
}

struct tree_root *
splay_search_np(struct tree_root *entry,
		int (*compare)(const void *, const void *),
		struct tree_root **root)
{
	int cmp;
	TREE_ROOT(node);  /* node for assembly use */
	struct tree_root *small, *large, *head, *tmp;

	head = *root;
	small = large = &node;

	while ((cmp = compare(entry, head)) != 0) {
		if (cmp < 0) {
			tmp = LEFT_PTR(head);
			if (tmp == NULL)
				break;
			if (compare(entry, tmp) < 0) {
				SPLAY_ROTATE_RIGHT_NP(head, tmp);
				if (LEFT_PTR(head) == NULL)
					break;
			}
			SPLAY_LINK_RIGHT_NP(head, large);
		} else {
			tmp = RIGHT_PTR(head);
			if (tmp == NULL)
				break;
			if (compare(entry, tmp) > 0) {
				SPLAY_ROTATE_LEFT_NP(head, tmp);
				if (RIGHT_PTR(head) == NULL)
					break;
			}
			SPLAY_LINK_LEFT_NP(head, small);
		}
	}

	SPLAY_ASSEMBLE_NP(head, &node, small, large);
	*root = head;

	if (cmp != 0)
		return NULL;

	return head;
}

struct tree_root *
splay_map_np(struct tree_root *new,
	     int (*compare)(const void *, const void *),
	     struct tree_root **root)
{
	int cmp;
	TREE_ROOT(node);  /* node for assembly use */
	struct tree_root *small, *large, *head, *tmp;

	INIT_TREE_ROOT(new);
	small = large = &node;
	head = *root;

	while (head && (cmp = compare(new, head)) != 0) {
		if (cmp < 0) {
			tmp = LEFT_PTR(head);
			if (tmp == NULL) {
				/* zig */
				SPLAY_LINK_RIGHT_NP(head, large);
				break;
			}
			cmp = compare(new, tmp);
			if (cmp < 0) {
				/* zig-zig */
				SPLAY_ROTATE_RIGHT_NP(head, tmp);
				SPLAY_LINK_RIGHT_NP(head, large);
			} else if (cmp > 0) {
				/* zig-zag */
				SPLAY_LINK_RIGHT_NP(head, large);
				SPLAY_LINK_LEFT_NP(head, small);
			} else {
				/* zig */
				SPLAY_LINK_RIGHT_NP(head, large);
				break;
			}
		} else {
			tmp = RIGHT_PTR(head);
			if (tmp == NULL) {
				/* zag */
				SPLAY_LINK_LEFT_NP(head, small);
				break;
			}
			cmp = compare(new, tmp);
			if (cmp > 0) {
				/* zag-zag */
				SPLAY_ROTATE_LEFT_NP(head, tmp);
				SPLAY_LINK_LEFT_NP(head, small);
			} else if (cmp < 0) {
				/* zag-zig */
				SPLAY_LINK_LEFT_NP(head, small);
				SPLAY_LINK_RIGHT_NP(head, large);
			} else {
				/* zag */
				SPLAY_LINK_LEFT_NP(head, small);
				break;
			}
		}
	}

	if (head == NULL)
		head = new;

	SPLAY_ASSEMBLE_NP(head, &node, small, large);

	*root = head;

	return head;
}

static inline void
__avl_balance(struct avl_root *new, struct avl_root **root)
{
	int balance = 0;
	struct avl_root *child, *grandson;

	while (PARENT_PTR(new) && balance == 0) {
		balance = PARENT_PTR(new)->balance;
		if (new == LEFT_PTR(PARENT_PTR(new)))
			--PARENT_PTR(new)->balance;
		else
			++PARENT_PTR(new)->balance;
		new = PARENT_PTR(new);
	}

	if (new->balance == -2) {
		child = LEFT_PTR(new);
		if (child->balance == -1) {
			__rotate_right((struct tree_root *)new,
				       (struct tree_root **)root);
			child->balance = 0;
			new->balance = 0;
		} else {
			grandson = RIGHT_PTR(child);
			__rotate_left((struct tree_root *)child,
				      (struct tree_root **)root);
			__rotate_right((struct tree_root *)new,
				       (struct tree_root **)root);
			if (grandson->balance == -1) {
				child->balance = 0;
				new->balance = 1;
			} else if (grandson->balance == 0) {
				child->balance = 0;
				new->balance = 0;
			} else {
				child->balance = -1;
				new->balance = 0;
			}
			grandson->balance = 0;
		}
	} else if (new->balance == 2) {
		child = RIGHT_PTR(new);
		if (child->balance == 1) {
			__rotate_left((struct tree_root *)new,
				      (struct tree_root **)root);
			child->balance = 0;
			new->balance = 0;
		} else {
			grandson = LEFT_PTR(child);
			__rotate_right((struct tree_root *)child,
				       (struct tree_root **)root);
			__rotate_left((struct tree_root *)new,
				      (struct tree_root **)root);
			if (grandson->balance == -1) {
				child->balance = 1;
				new->balance = 0;
			} else if (grandson->balance == 0) {
				child->balance = 0;
				new->balance = 0;
			} else {
				child->balance = 0;
				new->balance = -1;
			}
			grandson->balance = 0;
		}
	}
}

void avl_add(struct avl_root *new,
	int (*compare)(const void *, const void *),
	struct avl_root **root)
{
	new->balance = 0;
	TREE_ADD(new, compare, root);
	__avl_balance(new, root);
}

struct avl_root *
avl_map(struct avl_root *new,
	int (*compare)(const void *, const void *),
	struct avl_root **root)
{
	struct avl_root *node;

	new->balance = 0;
	node = (struct avl_root *)TREE_MAP(new, compare, root);
	if (node != new)
		return node;
	__avl_balance(new, root);
	return new;
}

void avl_del(struct avl_root *entry, struct avl_root **root)
{
	int dir = 0;
	int dir_next = 0;
	struct avl_root *new, *child, *parent, *succ;

	if (RIGHT_PTR(entry) == NULL) {
		/* Case 1: entry has no right child */
		if (LEFT_PTR(entry) != NULL)
			PARENT_PTR(LEFT_PTR(entry)) = PARENT_PTR(entry);
		if (PARENT_PTR(entry) == NULL) {
			*root = LEFT_PTR(entry);
			return;
		} else {
			dir = (entry == RIGHT_PTR(PARENT_PTR(entry)));
			PARENT_PTR(entry)->ptrs[dir] = LEFT_PTR(entry);
		}
		new = PARENT_PTR(entry);
	} else if (LEFT_PTR(RIGHT_PTR(entry)) == NULL) {
		/* Case 2: entry's right child has no left child */
		LEFT_PTR(RIGHT_PTR(entry)) = LEFT_PTR(entry);
		if (LEFT_PTR(entry) != NULL)
			PARENT_PTR(LEFT_PTR(entry)) = RIGHT_PTR(entry);
		PARENT_PTR(RIGHT_PTR(entry)) = PARENT_PTR(entry);
		if (PARENT_PTR(entry) == NULL)
			*root = RIGHT_PTR(entry);
		else
			PARENT_PTR(entry)->ptrs[entry == RIGHT_PTR(PARENT_PTR(entry))] = RIGHT_PTR(entry);
		RIGHT_PTR(entry)->balance = entry->balance;
		new = RIGHT_PTR(entry);
		dir = 1;
	} else {
		/* Case 3: entry's right child has a left child */
		succ = (struct avl_root *)TREE_SUCCESSOR(entry);
		if (RIGHT_PTR(succ) != NULL)
			PARENT_PTR(RIGHT_PTR(succ)) = PARENT_PTR(succ);
		LEFT_PTR(PARENT_PTR(succ)) = RIGHT_PTR(succ);
		new = PARENT_PTR(succ);
		LEFT_PTR(succ) = LEFT_PTR(entry);
		PARENT_PTR(LEFT_PTR(entry)) = succ;
		RIGHT_PTR(succ) = RIGHT_PTR(entry);
		PARENT_PTR(RIGHT_PTR(entry)) = succ;
		PARENT_PTR(succ) = PARENT_PTR(entry);
		if (PARENT_PTR(entry) == NULL)
			*root = succ;
		else
			PARENT_PTR(entry)->ptrs[entry == RIGHT_PTR(PARENT_PTR(entry))] = succ;
		succ->balance = entry->balance;
		dir = 0;
	}

	for (;;) {
		parent = PARENT_PTR(new);
		if (parent)
			dir_next = (new == RIGHT_PTR(parent));
		if (!dir) {
			new->balance++;
			if (new->balance == 1)
				break;
			if (new->balance == 2) {
				child = RIGHT_PTR(new);
				if (child->balance == -1) {
					succ = LEFT_PTR(child);
					__rotate_right((struct tree_root *)child,
						       (struct tree_root **)root);
					__rotate_left((struct tree_root *)new,
						      (struct tree_root **)root);
					if (succ->balance == -1) {
						child->balance = 1;
						new->balance = 0;
					} else if (succ->balance == 0) {
						child->balance = 0;
						new->balance = 0;
					} else {
						child->balance = 0;
						new->balance = -1;
					}
					succ->balance = 0;
				} else {
					__rotate_left((struct tree_root *)new,
						      (struct tree_root **)root);
					if (child->balance == 0) {
						child->balance = -1;
						new->balance = 1;
						break;
					} else {
						child->balance = 0;
						new->balance = 0;
					}
				}
			}
		} else {
			new->balance--;
			if (new->balance == -1)
				break;
			if (new->balance == -2) {
				child = LEFT_PTR(new);
				if (child->balance == 1) {
					succ = RIGHT_PTR(child);
					__rotate_left((struct tree_root *)child,
						      (struct tree_root **)root);
					__rotate_right((struct tree_root *)new,
						       (struct tree_root **)root);
					if (succ->balance == -1) {
						child->balance = 0;
						new->balance = 1;
					} else if (succ->balance == 0) {
						child->balance = 0;
						new->balance = 0;
					} else {
						child->balance = -1;
						new->balance = 0;
					}
					succ->balance = 0;
				} else {
					__rotate_right((struct tree_root *)new,
						       (struct tree_root **)root);
					if (child->balance == 0) {
						child->balance = 1;
						new->balance = -1;
						break;
					} else {
						child->balance = 0;
						new->balance = 0;
					}
				}
			}
		}
		if (parent == NULL)
			break;
		dir = dir_next;
		new = parent;
	}
}
