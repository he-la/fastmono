#ifndef FMT_RBINTERVAL_H
#define FMT_RBINTERVAL_H

/*
 * Defines an interval tree based on a red-black binary search tree.
 * This data structure should not be used as a normal binary search tree.
 *
 * All intervals are assumed to be non-overlapping, hence this is not a true
 * interval tree but rather a more specific implementation for this algorithm.
 */

namespace fmt {
namespace _ {
//! Not meant to be used externally
template<class T_key, class T_data, class T_ind>
class RB_Interval {
public:
	struct Node; // forward-declaration
private:
	Node* _root = nullptr;
	T_ind _size = 0;

	// Rotation direction for the rotate method
	enum RDir : bool { LEFT = true, RIGHT = false };

	// Node color
	enum Color : bool { RED = true, BLACK = false };

	void rotate(Node* node, const RDir direction);
	void _clear(Node* node);

	// Enforce red-black rules
	void fixup_postinsert(Node* node);
	void fixup_postdelete(Node* node, Node* replacement);

public:
	//! Node is only exposed for efficient handling of deletions from the caller
	struct Node {
		T_key key;
		const T_data data; //! Passed around with copy assignments, should be a pointer!
		Color color;
		Node *left = nullptr, *right = nullptr, *dad;

		Node(T_key key, const T_data data)
				: key(key), data(data), color(BLACK), dad(nullptr) {} // root constructor
		Node(T_key key, const T_data data, Node* dad)
				: key(key), data(data), dad(dad), color(RED) {} // standard constructor

		Node* grandfather() const {
			if (dad) return dad->dad;
			return nullptr;
		}
		Node* sibling() const {
			if (!dad) return nullptr;
			if (dad->left == this) return dad->right;
			return dad->left;
		}
		Node* uncle() const {
			if (!dad) return nullptr;
			return dad->sibling();
		}
	};

	RB_Interval();
	~RB_Interval();

	const T_ind size() const { return _size; }
	const Node* get_root() const { return _root; }

	void clear();

	Node* insert(const T_key min, const T_data data);
	T_data find(const T_key key) const;

	void remove(Node* node);
};
} // namespace _
} // namespace fmt
#endif
