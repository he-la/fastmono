#ifndef FMT_BST_IPP
#define FMT_BST_IPP

#include "bst.hpp"

#ifndef FMT_NOEXCEPT
#	include <stdexcept>
#endif

// TODO: breaks if vec is empty
template<class T_key, class T_data, class T_ind>
fmt::_::BST<T_key, T_data, T_ind>::BST(const std::vector<T_data>& vec) {
	// initialise root for performance (no need to check on every recursion)
	const T_ind left = T_ind(0), right = T_ind(vec.size() - 1), piv = right / T_ind(2);
	_root = new Node(vec[piv]->x, vec[piv]);

	// Recurse
	if (piv > left) _construct(vec, left, piv - 1, _root, true);
	if (piv < right) _construct(vec, piv + 1, right, _root, false);
}

template<class T_key, class T_data, class T_ind>
void fmt::_::BST<T_key, T_data, T_ind>::_construct(const std::vector<T_data>& vec, const T_ind left,
																									 const T_ind right, Node* dad,
																									 const bool is_left) {
	const T_ind piv = (left + right) / T_ind(2);
	Node* node = new Node(dad, vec[piv]->x, vec[piv], is_left);

	// Recurse
	if (piv > left) _construct(vec, left, piv - 1, node, true);
	if (piv < right) _construct(vec, piv + 1, right, node, false);
}

template<class T_key, class T_data, class T_ind>
void fmt::_::BST<T_key, T_data, T_ind>::_destruct(Node* node) {
	if (node->right) _destruct(node->right);
	if (node->left) _destruct(node->left);

	delete node;
}

template<class T_key, class T_data, class T_ind>
T_data fmt::_::BST<T_key, T_data, T_ind>::find(const T_key key) const {
	Node* node = _root;

	while (true) {
		if (key == node->key) break;
		if (key > node->key) {
			if (!node->right) break;
			node = node->right;
		} else {
			if (!node->left) break;
			node = node->left;
		}
	}

	return node->data;
}
#endif
