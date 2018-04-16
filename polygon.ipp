#ifndef FMT_POLYGON_IPP
#define FMT_POLYGON_IPP

#include "partition.hpp"
#include "polygon.hpp"

#ifndef FMT_NOEXCEPT
#	include <stdexcept>
#endif

template<class T_vert, class T_ind>
fmt::Polygon<T_vert, T_ind>::Polygon(const std::vector<T_vert>& vec) {
	set_vertices(vec);
}

template<class T_vert, class T_ind>
void fmt::Polygon<T_vert, T_ind>::set_vertices(const std::vector<T_vert>& vec) {
#ifndef FMT_NOEXCEPT
	if (vec.size() % 2)
		throw std::invalid_argument("Input vector is malformed, it must list x and y succesively.");
	if (vec.size() < 6) // technically would be possible, yet it is bad practice and likely indicates
											// an unintended input vector.
		throw std::invalid_argument("Cannot create polygon with less than three vertices.");
#endif

	invalidate();

	// Resize poly vector, cache original size
	T_ind existing_size = poly.size();
	poly.resize(vec.size() / 2);
	bool is_greater = existing_size < poly.size();

#ifdef DEBUG
	std::cout << "Existing size: " << existing_size << "; Current size: " << poly.size()
						<< "; Is greater: " << is_greater << std::endl;
#endif

	// Update element zero if it exists, else create it
	if (existing_size)
		poly[0]->set(vec[0], vec[1]);
	else
		poly[0] = new Vertex(vec[0], vec[1]);

	// Update all existing elements
	for (T_ind i = T_ind(2); i < 2 * (is_greater ? existing_size : poly.size()); i += 2)
		poly[i / 2]->set(vec[i], vec[i + 1], poly[(i / 2) - 1]);

	// Create new elements
	if (is_greater)
		for (T_ind i = (existing_size ? existing_size * 2 : 2); i < vec.size(); i += 2)
			poly[i / 2] = new Vertex(vec[i], vec[i + 1], poly[(i / 2) - 1]);

	// Link begin and end
	poly[0]->prev = poly.back();
	poly.back()->next = poly[0];
}

template<class T_vert, class T_ind>
void fmt::Polygon<T_vert, T_ind>::add_diagonal(const T_ind from, const T_ind to) {
	poly[to]->prev = poly[from];
	poly[from]->next = poly[to];
	has_diagonals = true;
}

template<class T_vert, class T_ind>
void fmt::Polygon<T_vert, T_ind>::clear_diagonals() {
	for (T_ind i = T_ind(1); i < poly.size(); ++i) {
		poly[i]->prev = poly[i - 1];
		poly[i - 1]->next = poly[i];
	}

	poly[0]->prev = poly[poly.size() - 1];
	poly[poly.size() - 1]->next = poly[0];

	has_diagonals = false;
}

template<class T_vert, class T_ind>
void fmt::Polygon<T_vert, T_ind>::push_back(Vertex* v) {
	invalidate();

	v->prev = poly.back();
	poly.back()->next = v;

	poly.push_back(v);

	v->next = poly[0];
	poly[0]->next = v;
}

template<class T_vert, class T_ind>
const std::vector<T_ind>& fmt::Polygon<T_vert, T_ind>::get_indices() {
	if (has_valid_indices) return indices;
	// TODO: Support using cached diagonals
	if (has_diagonals) clear_diagonals();
	std::vector<MonoPart<T_ind>>* parts = partition();
	triangulate(parts);
	delete parts;
	return indices;
}

template<class T_vert, class T_ind>
fmt::Polygon<T_vert, T_ind>::~Polygon() {
	// Delete all vertices
	for (auto it = poly.begin(); it != poly.end(); ++it)
		delete *it;
	// Invalidate pointers stored in the vector
	poly.clear();
}
#endif
