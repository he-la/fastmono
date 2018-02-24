#ifndef FMT_RBINTERVAL_IPP
#define FMT_RBINTERVAL_IPP

#include "rb_interval.hpp"

#include <cstdint>

namespace fmt {

template<class T_key, class T_data, class T_ind>
_::RB_Interval<T_key, T_data, T_ind>::RB_Interval() {}
template<class T_key, class T_data, class T_ind>
_::RB_Interval<T_key, T_data, T_ind>::~RB_Interval() {
	clear();
}

template<class T_key, class T_data, class T_ind>
void _::RB_Interval<T_key, T_data, T_ind>::clear() {
	_clear(_root);
	_root = nullptr;
	_size = 0;
}

template<class T_key, class T_data, class T_ind>
void _::RB_Interval<T_key, T_data, T_ind>::_clear(Node* node) {
	if (!node) return;
	_clear(node->left);
	_clear(node->right);
	delete node;
}

template<class T_key, class T_data, class T_ind>
void _::RB_Interval<T_key, T_data, T_ind>::rotate(Node* node, const RDir direction) {
	Node* tmp_node;
	if (direction == RIGHT) {
		tmp_node = node->left;
		node->left = tmp_node->right;
		if (node->left) node->left->dad = node;
		tmp_node->right = node;
	} else { // left
		tmp_node = node->right;
		node->right = tmp_node->left;
		if (node->right) node->right->dad = node;
		tmp_node->left = node;
	}

	if (node->dad)
		if (node->dad->left == node)
			node->dad->left = tmp_node;
		else
			node->dad->right = tmp_node;
	else
		_root = tmp_node;

	tmp_node->dad = node->dad;
	node->dad = tmp_node;
}

// The tree will only be a valid representation of the polygon when all
// insertions and updates have been processed. Must first call update on all
// existing entires to prevent key conflicts
template<class T_key, class T_data, class T_ind>
typename _::RB_Interval<T_key, T_data, T_ind>::Node*
		_::RB_Interval<T_key, T_data, T_ind>::insert(const T_key min, const T_data data) {
	++_size;
	if (!_root) {
		_root = new Node(min, data);
		return _root;
	}

	// locate min
	Node* tmp_node = _root;
	int_fast8_t flag = 0;
	while (!flag) // location routine
		if (tmp_node->key > min) {
			flag = tmp_node->left ? 0 : -1;
			if (tmp_node->left) tmp_node = tmp_node->left;
		} else {
			flag = tmp_node->right ? 0 : 1;
			if (tmp_node->right) tmp_node = tmp_node->right;
		}
	if (flag == -1) { // insert left
		tmp_node->left = new Node(min, data, tmp_node);
		fixup_postinsert(tmp_node->left);
	} else { // insert right
		tmp_node->right = new Node(min, data, tmp_node);
		fixup_postinsert(tmp_node->right);
	}
}

template<class T_key, class T_data, class T_ind>
void _::RB_Interval<T_key, T_data, T_ind>::fixup_postinsert(Node* node) {
	if (!node->dad) {
		node->color = BLACK;
		return;
	}
	if (node->dad->color == RED) {
		Node* grandpa = node->grandfather();
		Node* uncle = node->uncle();
		if (uncle && uncle->color == RED) {
			node->dad->color = BLACK;  // was red by parent if statement
			uncle->color = BLACK;      // was red (see this if statement)
			grandpa->color = RED;      // was black by definition
			fixup_postinsert(grandpa); // recursion
		} else {                     // uncle is black or dead (or grandpa is dead)
			if ((node == node->dad->right) && (node->dad == grandpa->left)) { // right left case
				rotate(node->dad, LEFT);
				node = node->left;
			} else if ((node == node->dad->left) && (node->dad == grandpa->right)) { // left right case
				rotate(node->dad, RIGHT);
				node = node->right;
			}

			node->dad->color = BLACK; // was red (see parent if statement)
			grandpa->color = RED;     // was black by definition

			if ((node == node->dad->left) && (node->dad == grandpa->left)) // left left case
				rotate(grandpa, RIGHT);
			else
				rotate(grandpa, LEFT);
		}
	}
}

// Does not check if node exists!
template<class T_key, class T_data, class T_ind>
void _::RB_Interval<T_key, T_data, T_ind>::remove(
		typename _::RB_Interval<T_key, T_data, T_ind>::Node* node) {
	--_size;
	Node* replacement;
	if (!node->left)
		replacement = node->right; // may be nullptr
	else if (!node->right)
		replacement = node->left; // may be nullptr
	else {
		// Node has two children; execute recvursive deletion algorithm:
		// Find inorder successor (could also find predecessor)
		// Switch node with replacement; delete replacement
		replacement = node->right;
		while (replacement->left)
			replacement = replacement->left;

		// Update references (cannot just copy data since owner of tree has references to nodes)
		if (replacement->dad->left == replacement)
			replacement->dad->left = replacement->right; // update dad's reference
		else
			replacement->dad->right = replacement->right;                     // update dad's reference
		if (replacement->right) replacement->right->dad = replacement->dad; // update child's reference

		// perform fixup
		fixup_postdelete(replacement, replacement->right);

		// update replacement's references to move to node's position
		replacement->dad = node->dad;
		replacement->left = node->left;
		replacement->right = node->right;
		// update node's dad's reference to child
		if (node->dad->left == node)
			node->dad->left = replacement;
		else
			node->dad->right = replacement;
		// update node's children to new dad. Note that replacement's children have changed.
		if (replacement->left) replacement->left->dad = replacement;
		if (replacement->right) replacement->right->dad = replacement;
		// Everything is (should be) updated properly. Now delete node.
		delete node;
		return;
	}

	if (node->dad) { // node is not root
		// update dad's reference to child
		if (node->dad->left == node)
			node->dad->left = replacement;
		else
			node->dad->right = replacement;
		if (replacement) replacement->dad = node->dad; // update child's reference to dad
	} else {                                         // node is root
		_root = replacement;
		if (replacement) replacement->dad = nullptr;
	}

	fixup_postdelete(node, replacement);
	delete node;
}

template<class T_key, class T_data, class T_ind>
void _::RB_Interval<T_key, T_data, T_ind>::fixup_postdelete(Node* node, Node* replacement) {
	if (node->color || _root == replacement ||
			(replacement && replacement->color)) { // either is red, simple case
		if (replacement) replacement->color = BLACK;
		return;
	}

	// Both are black (replacement may be nullptr (which is considered black))
	// replacement is thus 'double black'
recur_fixup_del:                  // FIXME: Don't use labels and gotos, this is not assembly
	Node* sister = node->sibling(); // is still well-defined.
	if (!sister) return;            // nothing to do

	if (BLACK == sister->color) { // sister is is black
		Node* r_left = (sister->left && sister->left->color) ? sister->left : nullptr;
		Node* r_right = (sister->right && sister->right->color) ? sister->right : nullptr;

		if (!r_left && !r_right) { // sister is black with two black chidlren.
			sister->color = RED;
			if (BLACK == sister->dad->color) {
				node = sister->dad;
				goto recur_fixup_del; // FIXME: really. fix me.
			}
		} else // sister has a red child
				if (sister->dad->left == sister)
			if ((r_left && r_right) || r_left) // left left case
				rotate(sister->dad, RIGHT);
			else { // left right case
				rotate(sister, LEFT);
				rotate(sister->dad, RIGHT);
			}
		else if ((r_right && r_left) || r_right) // right right case
			rotate(sister->dad, LEFT);
		else {
			rotate(sister, RIGHT);
			rotate(sister->dad, LEFT);
		}
	} else { // sister is red
		sister->color = Color(!sister->color);
		sister->dad->color = Color(!sister->dad->color);
		if (sister->dad->left == sister) { // rotate sister up
			sister->left->color = BLACK;
			rotate(sister->dad, RIGHT);
		} else {
			sister->right->color = BLACK;
			rotate(sister->dad, LEFT);
		}
	}
}

// query a point
template<class T_key, class T_data, class T_ind>
T_data _::RB_Interval<T_key, T_data, T_ind>::find(const T_key key) const {
	Node* tmp_node = _root;
	Node* last_right = nullptr;

	while (true) // location routine
		if (tmp_node->key > key)
			if (tmp_node->left)
				tmp_node = tmp_node->left;
			else
				return last_right->data;
		else if (tmp_node->right) {
			last_right = tmp_node;
			tmp_node = tmp_node->right;
		} else
			return tmp_node->data;
}
} // namespace fmt
#endif
