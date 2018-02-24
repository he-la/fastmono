#ifndef FMT_PARTITION_IPP
#define FMT_PARTITION_IPP
/* Implements the partitioning algorithm to subset a given polygon into monotone parts.
 * This file provides the bottom layer to the abstraction of the polygon class.
 */

#include "bst.hpp" // actually not a bst but an interval tree
#include "partition.hpp"
#include "polygon.hpp"
#include "rb_interval.hpp" // red-black interval tree

#include <algorithm>
#include <forward_list>
#include <list>

namespace fmt {

template<class T_vert, class T_ind>
typename std::vector<MonoPart<T_ind>>*
		Polygon<T_vert, T_ind>::partition(T_ind frac_starts, T_ind frac_merges, T_ind frac_splits,
																			T_ind frac_stops) {

	Polygon<T_vert, T_ind>& poly = *this;

	/* Stage 1: Building the event set */
	/*
		Step through each vertex consecutively, calculate if the
		x-direction has changed. If it has, handle the event by
		determining whether the angle is reflex using vector cross multiplication.
		From this, determine the type of vertex (either start, merge, split, or
		end), and push it to the vector. This process takes Θ(n) time.
	*/
	std::vector<_::EventVertex<T_ind>> evs;                // set of all event vertices
	std::vector<_::MergeVertex<T_ind>> ev_merges;          // set of merge vertex data
	std::vector<_::EventVertex<T_ind>*> ev_starts;         // cache references to start vertices
	std::vector<_::SplitVertex<T_vert, T_ind>*> ev_splits; // set of split vertex data as pointers

	// Guess number of vertex types for initial allocation
	T_ind g_starts = poly.size() / frac_starts, g_splits = poly.size() / frac_splits,
				g_merges = poly.size() / frac_merges, g_stops = poly.size() / frac_stops;

	g_starts = std::min(g_starts, T_ind(3));
	g_splits = std::min(g_splits, T_ind(3));
	g_merges = std::min(g_merges, T_ind(2));
	g_stops = std::min(g_stops, T_ind(3));

	evs.reserve(g_starts + g_splits + g_merges + g_stops);
	ev_starts.reserve(g_starts);
	ev_merges.reserve(g_merges);
	ev_splits.reserve(g_splits + 1);

	// Begin sweeping
	bool sweeping_right = poly[0]->x < poly[1]->x; // track current sweep direction
	T_vert max_x = NULL; // track max x coordinate to append dummy split event

	for (T_ind i = T_ind(0); i < poly.size(); ++i) {
		if ((poly[i - 1]->x < poly[i]->x) != sweeping_right) {
			sweeping_right = !sweeping_right;

			if (_::is_reflex<Polygon<T_vert, T_ind>, T_ind>(poly, i)) // reflex angle around i
				if (!sweeping_right) {                                  // Now sweeping LEFT!
					evs.emplace_back(i, evs.empty() ? nullptr : &evs.back(), _::MERGE);
					ev_merges.emplace_back(evs.back());
				} else { // Now sweeping RIGHT!
					evs.emplace_back(i, evs.empty() ? nullptr : &evs.back(), _::SPLIT);
					ev_splits.push_back(new _::SplitVertex<T_vert, T_ind>(evs.back(), poly[i]->x, poly[i]->y,
																																g_starts / g_splits));
				}
			else if (sweeping_right) { // Now sweeping RIGHT!
				evs.emplace_back(i, evs.empty() ? nullptr : &evs.back(), _::START);
				ev_starts.push_back(&evs.back());
			} else
				evs.emplace_back(i, evs.empty() ? nullptr : &evs.back(), _::STOP);
		}
		max_x = poly[i]->x > max_x ? poly[i]->x : max_x;
	}

	evs.back().next = &evs.front();
	evs.front().prev = &evs.back();

#ifdef DEBUG
	_::report_vector_reallocation(evs.size(), g_starts + g_merges + g_splits + g_stops, "evs");
	_::report_vector_reallocation(ev_starts.size(), g_starts, "ev_starts");
	_::report_vector_reallocation(ev_merges.size(), g_merges, "ev_merges");
	_::report_vector_reallocation(ev_splits.size(), g_splits, "ev_splits");
#endif

	/* Stage 2: Building the split set */

	// Sort split vertices in guaranteed Θ(s log s) for s split vertices
	_::qsort<T_vert, T_ind>(ev_splits, 0, ev_splits.size() - 1); // uses quicksort defined above

	// Dummy to attach starts behind last split. Note that the event pointed to is meaningless
	ev_splits.push_back(
			new _::SplitVertex<T_vert, T_ind>(evs.back(), max_x + 1.0f, 0.0f, g_starts / g_splits));
	ev_splits.shrink_to_fit();

	// Link start vertices to the split vertex after them for handling
	if (ev_splits.size() != 1) {
		// Build simple balanced BST onto ev_splits in Θ(s)
		_::BST<T_vert, _::SplitVertex<T_vert, T_ind>*, T_ind> split_tree =
				_::BST<T_vert, _::SplitVertex<T_vert, T_ind>*, T_ind>(ev_splits);

		// Locate in Θ(k log s) for k start vertices
		for (auto&& it = ev_starts.begin(); it != ev_starts.end(); ++it)
			split_tree.find(poly[(*it)->index]->x)->starts.push_front(*it);
	} else {
		// No split vertex exists, attach all to dummy.
		// ev_splits.back()->starts.reserve(ev_starts.size());
		// TODO: Write custom allocator to pre-allocate forward list

		for (auto&& it = ev_starts.begin(); it != ev_starts.end(); ++it)
			ev_splits.back()->starts.push_front(*it);
	}

	/* Stage 3: Core logic: partitioning */
	auto parts = new std::vector<MonoPart<T_ind>>();
	parts->reserve(ev_starts.size() + ev_splits.size());

	std::forward_list<MonoPart<T_ind>*> actives;

	_::RB_Interval<T_vert, MonoPart<T_ind>*, T_ind> rbtree;
	std::vector<_::EventVertex<T_ind>> new_starts;
	new_starts.reserve(ev_splits.size());

	// Iterate all split vertices (including dummy)
	for (auto&& it = ev_splits.begin(); it != ev_splits.end() - 1; ++it) {
		_::SplitVertex<T_vert, T_ind>& this_split = **it;
		// Logic:
		// 1. Add starts to active set, step up to split and handle merges / stops recursively
		// 2. Active list now only has currently active parts: update interval tree
		// 3. Locate split in in interval tree, build diagonal, hand over to next split

		// Add new vertices to active set
		for (auto&& it_s = this_split.starts.cbegin(); it_s != this_split.starts.cend(); ++it_s) {
			parts->emplace_back((*it_s)->index, (*it_s)->next, (*it_s)->prev);
			MonoPart<T_ind>& this_part = parts->back();
			actives.push_front(&this_part);

			this_part.active = true;

			if ((*it_s)->prev->type == _::MERGE)
				(*it_s)->prev->template get_data<_::MergeVertex<T_ind>>()->part_above = &this_part;
			if ((*it_s)->next->type == _::MERGE)
				(*it_s)->next->template get_data<_::MergeVertex<T_ind>>()->part_below = &this_part;

			// update merge vertices to reflect new part
			// REMOVED with new in-place tracking system, keeping in case of revert
			// if ((*it_s)->merge_below) (*it_s)->merge_below.part_above = &this_part;
			// if ((*it_s)->merge_above) (*it_s)->merge_above.part_below = &this_part;
		}

		// Step
		for (auto&& it_p = actives.begin(); it_p != actives.end(); ++it_p) {
			MonoPart<T_ind>& this_part = **it_p;

			if (!this_part.active)
				continue; // TODO: Reevaluate the necessity of this check with the new deletion model

			// Step through each vertex in the part in a while loop so that we can manually handle
			// deletions and increments. Termination is handled by the condition in the loop.
			while (true) {
				bool is_upper;
				_::EventVertex<T_ind>* this_vert;
				if (poly[this_part.upper->index]->x <= poly[this_part.lower->index]->x) {
					this_vert = this_part.upper->next;
					is_upper = true;
				} else {
					this_vert = this_part.lower->prev;
					is_upper = false;
				}

				if (poly[this_vert->index]->x > this_split.x) break;

				if (is_upper)
					this_part.upper = this_vert;
				else
					this_part.lower = this_vert;

				if (this_vert->type == _::NORMAL) continue; // Vertex has been handled by another part

				// Handle vertex types (merge and stop)
				// If merge, else if stop
				if (this_vert->type == _::MERGE) {
					// Logic:
					// 1. Check whether the vertex with higher x than this vertex on the upper opposing chain
					// has an x below the current split vertex. 2.a If it does, set merge_to to that vertex.
					// 2.b If it does not, repeat for the lower opposing chain.
					// 3.a If this vertex cannot be merged to either, continue with the next part. This merge
					// vertex must behandled at the next split vertex. 3.b Else, a merge_to vertex exist.
					// Merge to it and mark the current part as done.

					T_ind merge_to;
					typename _::EventVertex<T_ind>*upper,
							*lower; // EventVertex structs above and below the merge vertex

					upper = is_upper
											? this_vert->template get_data<_::MergeVertex<T_ind>>()->part_above->upper
											: this_part.upper;
					lower = is_upper
											? this_part.lower
											: this_vert->template get_data<_::MergeVertex<T_ind>>()->part_below->lower;

					// Lambda since C++ doesn't do proper macros very well and moving this out into a function
					// would become quite confusing.
					auto check_merge = [&merge_to, &this_vert, &this_split,
															&poly](const Polygon::Vertex* vertex,
																		 void (*oprtr)(const Polygon::Vertex*)) {
						for (; vertex->x <= poly[this_vert->index]->x; oprtr(vertex))
							; // Step index until a vertex right of the current merge vertex is found

						if (vertex->x > this_split.x) return false; // broke bounds on split vertex

						merge_to = vertex - poly[0];
						return true;
					};

					// It would be better to figure out how to pass the operator directly, but the compiler
					// should inline this so it's effectively the same.
					bool merged_high = false, merged_low = false;
					merged_high =
							check_merge(poly[upper->index], [](const Polygon::Vertex* val) { val = val->next; });
					if (!merged_high)
						merged_low = check_merge(poly[lower->index],
																		 [](const Polygon::Vertex* val) { val = val->prev; });

					if (merged_high || merged_low) {
						// Merge to merge_to
						poly.add_diagonal(this_vert->index, merge_to);

						// Update links between EventVertex structs to reflect new diagonal.
						// First need to locate the next EventVertex after the probably normal vertex at
						// merge_to.
						if (merged_high) {
							for (; poly[upper->index]->x < poly[merge_to]->x; upper = upper->next)
								;
							this_vert->next = upper;
							upper->prev = this_vert;
						} else {
							for (; poly[lower->index]->x < poly[merge_to]->x; lower = lower->prev)
								;
							this_vert->prev = lower;
							lower->next = this_vert;

							if (lower->type == _::MERGE)
								lower->template get_data<_::MergeVertex<T_ind>>()->part_above = &this_part;
						}

						this_vert->type = _::NORMAL; // Flag as handled

						if ((is_upper && merged_low) || (!is_upper && merged_high)) {
							this_part.active = false;
							this_part.tail = merge_to;
							break; // break out of current part as it is done.
						}

						if (is_upper)
							this_vert->next->template get_data<_::MergeVertex<T_ind>>()->part_below =
									this_vert->template get_data<_::MergeVertex<T_ind>>()->part_below;
						else
							this_vert->prev->template get_data<_::MergeVertex<T_ind>>()->part_above =
									this_vert->template get_data<_::MergeVertex<T_ind>>()->part_above;
					} // We cannot merge. Continue to next event.

				} else // If not merge, it must be stop
#ifdef DEBUG   // If debugging, check if remaining vertex is truly stop, else throw exception.
						if (vert.type == STOP)
#endif
				{ // Stop Vertex
					this_part.active = false;
					this_part.tail = this_vert->index;
					break;
				}
#ifdef DEBUG
				else // Vertex was neither merge nor stop. Something is wrong.
					throw std::runtime_error("Reached invalid event type during step!");
#endif
			} // iteration through this_part
		}   // iteration through actives

		// Update RB Interval tree
		// TODO: Consider first iterating for deletion, then iterating for insertion
		// this would minimize tree lookups at the cost of iterating actives twice.
		// could also track list of to_delete, delete them, then update actives.
		auto&& it_p = actives.begin();
		auto&& it_p_last = actives.before_begin();
		while (it_p != actives.end()) {
			MonoPart<T_ind>& this_part = **it_p;
			if (!this_part.active) {
				rbtree.remove(this_part.template get_node<T_vert, MonoPart<T_ind>*>());
				actives.erase_after(it_p_last);
				it_p = it_p_last;
				++it_p;
				continue;
			}

			T_ind lowest = this_part.lower->index;
			for (; poly[lowest]->x <= this_split.x; --lowest)
				;

			if (this_part.node)
				this_part.template get_node<T_vert, MonoPart<T_ind>*>()->key = poly[lowest]->y;
			else
				this_part.node = rbtree.insert(lowest, &this_part);

			++it_p;
		} // Second iteration through actives

		if (it == ev_splits.end()) break; // No need to lookup dummy

		MonoPart<T_ind>* to_split = rbtree.find(this_split.y);

		T_ind upper = to_split->upper->index;
		T_ind lower = to_split->lower->index;

		for (; poly[upper]->x <= this_split.x; ++upper)
			;
		for (; poly[lower]->x <= this_split.x; --lower)
			;

		_::EventVertex<T_ind>* this_start;
		if (poly[upper]->x > poly[lower]->x) {
			// Split to upper
			new_starts.emplace_back(upper, _::START);
			this_start = &new_starts.back();

			this_start->next = &this_split.event;
			this_split.event.prev = this_start;
		} else {
			// Split to lower
			new_starts.emplace_back(lower, _::START);
			this_start = &new_starts.back();

			this_start->prev = &this_split.event;
			this_split.event.next = this_start;
		}
		(*(it + 1))->starts.push_front(this_start);

		add_diagonal(this_split.event.index, this_start->index);

		this_split.event.type = _::NORMAL;
	} // iteration through splits

	/* Finally: Update flags and do cleanup */
	// Delete vector of split pointers
	for (auto it = ev_splits.begin(); it != ev_splits.end(); ++it)
		delete *it;

	poly.has_valid_diagonals = true;

	return parts; // caller must handle cleanup
}
} // namespace fmt
#endif
