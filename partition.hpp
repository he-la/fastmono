#ifndef FMT_PARTITION_H
#define FMT_PARTITION_H

#include <cstdint>
#include <forward_list>
#include <vector>

#ifdef DEBUG
#	include <iostream>
#endif

#include "rb_interval.hpp"

namespace fmt {

// Forward Declarations
template<class T_vert, class T_ind>
class Polygon;

template<class T_ind>
struct MonoPart;

// Detail namespace
namespace _ {
// start at 1 so type is always truthy (since logically both STOP and NORMAL would have to be falsy)
enum VertexType : uint_fast8_t { STOP = 1, START = 2, MERGE = 3, SPLIT = 4, NORMAL = 5 };

/*! Handles generalised event vertices with pointers for inner doubly-linked lists to track
	diagonals on the main chain.

	Contains a void* to a data object of either MergeVertex or SplitVertex. Equally,
	these data structs should contain a valid reference to their EventVertex. This solution may not be
	the most elegant, but it works with (almost) zero runtime cost.
 */
template<class T_ind>
struct EventVertex {
	const T_ind index; //!< index of this vertex in the polygon
	VertexType type;   //!< type of the vertex

	void* data; //!< pointer to a struct (MergeVertex or SplitVertex)

	template<class T_data>
	T_data* get_data() const {
		return static_cast<T_data*>(data);
	}

	EventVertex<T_ind>*next, *prev; //!< Links for inner linked lists.

	//! Construct a new vertex and point the previous vertex to this one
	EventVertex(const T_ind index, EventVertex<T_ind>* prev, const VertexType type)
			: index(index), prev(prev), type(type) {
		prev->next = this;
	}

	EventVertex(const T_ind index, VertexType type) : index(index), type(type) {}
};

template<class T_ind>
struct MergeVertex {
	EventVertex<T_ind>& event; //!< Store reference to EventVertex struct
	//! Pointers to upper and lower parts
	MonoPart<T_ind>*part_above, *part_below;

	MergeVertex(EventVertex<T_ind>& event) : event(event) { event.data = static_cast<void*>(this); }
};

// Split vertex needs to contain a reference to itself in the chain and all start vertices to the
// left of it.
template<class T_vert, class T_ind>
struct SplitVertex {
	EventVertex<T_ind>& event; //!< Store reference to EventVertex struct
	
	//! cache x and y coordinates (uses more memory, reduces number of dereferences)
	const T_vert x, y;

	std::forward_list<EventVertex<T_ind>*> starts; //!< allow start vertices to hook into split vertex

	SplitVertex(EventVertex<T_ind>& event, const T_vert x, const T_vert y, const T_ind reserve)
			: event(event), x(x), y(y) {
		event.data = static_cast<void*>(this);

		// starts.reserve(reserve);
		// TODO: Custom allocator function
	}

	// Operators for quicksort. They are very constant.
	constexpr inline bool operator<=(const SplitVertex& rhs) const { return x <= rhs.x; }
	constexpr inline bool operator>=(const SplitVertex& rhs) const { return x >= rhs.x; }
};

// Basically z-component of cross multiplication
// let the angle be between (x1,y1)|(x2,y2)|(x3,y3)
// => (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1)
template<class T_vec, class T_ind>
constexpr inline bool is_reflex(const T_vec& v, const T_ind i) {
	return ((v[i]->x - v[i - 1]->x) * (v[i + 1]->y - v[i - 1]->y) -
					(v[i]->y - v[i - 1]->y) * (v[i + 1]->x - v[i - 1]->x)) > 0;
}

// Quicksort implementation using system stack recursion.
template<class T_vert, class T_ind>
inline void qsort(std::vector<SplitVertex<T_vert, T_ind>*>& vec, const T_ind left,
									const T_ind right) {
	// Create tmp value, cache pivot (center), etc
	SplitVertex<T_vert, T_ind>* tmp = vec[left];
	T_ind i = left, j = right;
	const T_ind piv_i = (left + right) / T_ind(2);
	const SplitVertex<T_vert, T_ind> piv = *vec[piv_i];

	// Standard quicksort algorithm
	while (i < j) {
		while (i < piv_i && *vec[i] <= piv)
			++i;
		while (j > piv_i && *vec[j] >= piv)
			--j;
		if (i < j) {
			tmp = vec[i];
			vec[i] = vec[j];
			vec[j] = tmp;
			++i;
			--j;
			continue;
		}
	}

	// Recurse on the system stack
	if (left < j) qsort(vec, left, right);
	if (i < right) qsort(vec, i, right);
}

#ifdef DEBUG
void report_vector_reallocation(const T_ind vecsize, const T_ind gsize,
																const char* vecname) if (vecsize > gsize) {
	std::cout << "Had to re-allocate " << vecname << " as guessed length was insufficient!"
						<< std::endl
						<< "    Guessed size: " << gsize << "    Used size: " << vecsize << std::endl;
}
#endif

} // namespace _

/*! Helper struct used returned by Polygon::partition that indicates a monotone region starting at
	 the index in head.

	 Note that the pointers to upper and lower events as well as the RB Tree node pointer are invalid
	 outside the context of the partition function. Also, the active flag is meaningless outside of
	 this context.
 */
template<class T_ind>
struct MonoPart {
	const T_ind head; //!< Index in the polygonal chain where the part starts
	T_ind tail;       //!< Index of the stop vertex ending the part

	_::EventVertex<T_ind>*upper, *lower; //!< Pointers to events on the upper and lower chain

	bool active = false; //!< Ugly hack for lazy deletion from active set

	void* node = nullptr; //!< Reference node in RB Interval tree

	//! Get node by casting to appropriate pointer type
	template<class T_vert, class T_data>
	typename _::RB_Interval<T_vert, T_data, T_ind>::Node* get_node() const {
		return static_cast<typename _::RB_Interval<T_vert, T_data, T_ind>::Node*>(node);
	}

	//! Construct new MonoPart
	//!
	//! @param head: Index of the starting vertex.
	//! @param upper: Pointer to the part's upper EventVertex
	//! @param lower: Pointer to the part's lower EventVertex
	MonoPart(const T_ind head, _::EventVertex<T_ind>* upper, _::EventVertex<T_ind>* lower)
			: head(head), upper(upper), lower(lower) {}
};
} // namespace fmt
#endif
