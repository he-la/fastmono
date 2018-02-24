#ifndef FMT_TRIANGULATE_IPP
#define FMT_TRIANGULATE_IPP

// Triangulates monotone parts into a set of indices

#include "partition.hpp"
#include "polygon.hpp"

#include <cmath>
#include <iterator>
#include <list>
#include <vector>

namespace fmt {

// Could also be implemented with OpenCL or OpenMP for lightning performance
template<class T_vert, class T_ind>
void Polygon<T_vert, T_ind>::triangulate(std::vector<MonoPart<T_ind>>* parts) {
	indices.clear();
	Polygon<T_vert, T_ind>& poly = *this;

	for (auto&& it = parts->cbegin(); it != parts->cend(); ++it) {
		const MonoPart<T_ind>& this_part = *it;

		std::list<T_ind> L{this_part.head, poly[this_part.head + 1]->x < poly[this_part.head - 1]->x
																					 ? this_part.head + 1
																					 : this_part.head - 1};

		// TODO: Properly cache part length
		// L.reserve(std::abs(this_part.head - this_part.tail));
		// FIXME: This is a very unprecise measure as it ignores all jumps etc and can tend to
		// drastically overallocate

		while (true) {
			L.push_back(poly[L.front() + 1]->x < poly[L.front() - 1]->x ? L.front() + 1 : L.front() - 1);

			if (L.back() == this_part.tail ||
					(L.size() == 3 && (L.front() == *(std::next(L.begin())) + 1 == *(std::next(std::next(L.begin()))) + 2 ||
														 L.front() == *(std::next(L.begin())) - 1 == *(std::next(std::next(L.begin()))) - 2))) {
				// All on same chain
				indices.push_back(L.front());
				indices.push_back(*(std::next(L.begin())));
				indices.push_back(*(std::next(std::next(L.begin()))));
				// an homage to (cdr (cdr (cdr (list vertices))))

				L.erase(L.begin());
			} else {
				for (auto&& it = L.cbegin(); it != (std::prev(std::prev(L.cend()))); ++it) {
					indices.push_back(*it);
					indices.push_back(*(std::next(it)));
					indices.push_back(L.back());

					it = std::prev(L.erase(it));
				}
			}

			if (L.back() == this_part.tail) break;
		}

		has_valid_indices = true;
	}
}

} // namespace fmt

#endif
