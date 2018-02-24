#ifndef FMT_BST_H
#define FMT_BST_H

/* Defines a simple binary search interval tree with no self-balancing */

#include <vector>

namespace fmt {
namespace _ {

/*! A simple binary search interval tree. Not meant for external use.

	Takes a *sorted* vector as input to construct a constant balanced bst as an interval tree. All
 intervals are assumed to be non-overlapping. This implementation is specific to the triangulation
 algorithm in that it deduces the key from accessing the member variable "x" from the provided data.

	Note that destroying the tree does not destroy the linked data objects.

	@tparam T_key Type of the node's key, == T_vert.
	@tparam T_data Type of the data object, must be a pointer.
	@tparam T_ind Integer type to use for indexing
*/
template<class T_key, class T_data, class T_ind>
class BST {
private:
	struct Node {
		Node *left = nullptr, *right = nullptr, *dad;
		const T_data data;
		const T_key key;

		Node(const T_key key, const T_data data)
				: dad(nullptr), key(key), data(data) {} // root constructor

		Node(Node* dad, const T_key key, const T_data data, const bool left)
				: dad(dad), key(key), data(data) { // normal constructor
			if (left)
				dad->left = this;
			else
				dad->right = this;
		}
	};

	Node* _root = nullptr;

	// Build the tree using system stack recursion
	void _construct(const std::vector<T_data>& vec, const T_ind left, const T_ind right, Node* dad,
									const bool is_left);
	// Destroy the tree using system stack recursion
	void _destruct(Node* node);

public:
	BST(const std::vector<T_data>& vec);

	~BST() { _destruct(_root); };

	//! Performs relative interval location, finds next higher key
	T_data find(const T_key key) const;
};
} // namespace _
} // namespace fmt
#endif
