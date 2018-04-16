#ifndef FMT_POLYGON_H
#define FMT_POLYGON_H

#ifdef DEBUG
#	ifdef FMT_NOEXCEPT
#		warning                                                                                        \
				"Defining DEBUG automatically undefines FMT_NOEXCEPT as FastMono builds extra runtime checks in DEBUG mode. Do not define DEBUG if you wish to build without support for exceptions"
#		undef FMT_NOEXCEPT
#	endif
#endif

/* Defines a polygon as a vector of vertices
 * Allows polygons to have diagonals, which are represented as a linked list living inside the
 * vertex vector.
 */

#include "partition.hpp"

#include <memory>
#include <vector>

//! FastMono Triangulator namespace
namespace fmt {
/*! A polygon formed for FastMono triangulation.

	Defines a polygon as a vector of vertices with inner linked list loops forming individual
	compartemnts as created by inserting diagonals.

	This class provides the interface to the FastMono algorithm implementation. Since the algorithm
	relies on attributing vertices with special data, this special polygon class is used. The
	algorithm itself resides in the functions Polygon::partition and Polygon::triangulate.

	You can learn more about the algorithm used at github.com/henriklaxhuber/fastmono.

	@tparam T_vert numeric type of a coordinate component, e.g. float.
	@tparam T_ind numeric type of an index, e.g. uint_fast32_t
*/

template<class T_vert, class T_ind>
class Polygon {
public:
	struct Vertex; // forward declaration

private:
	std::vector<Vertex*> poly; //!< Vertices of the polygon

	bool has_diagonals = false;       //!< Indicates whether the polygon contains any diagonals.
	bool has_valid_diagonals = false; //!< Indicates whether the current set of diagonals is up-to-date
	bool has_valid_indices = false;   //!< Indicates whether the current set of triangle indices is up-to-date

	std::vector<T_ind> indices; //!< Caches the latest triangle indices

	//! Invalidate the current set of diagonals and indices.
	//! Convenience function.
	inline void invalidate() {
		has_valid_diagonals = false;
		has_valid_indices = false;
	}

	//! Add a diagonal to the polygon
	//! Convenience function. Also sets has_diagonals to true.
	void add_diagonal(const T_ind from, const T_ind to);

public:
	/*! A vertex stored as an element of a linked list.

		Represents a vertex with x, y of type T_vert. Should not be used to represent a single point on
		a plane.
	*/
	struct Vertex {
		T_vert x, y;
		Vertex *next = nullptr, *prev = nullptr;

		//! Change the coordinates of the vertex
		void set(T_vert x, T_vert y) {
			this->x = x;
			this->y = y;
		}
		//! Change the coordinates and previous link of the vertex. Also updates the previous vertex.
		void set(T_vert x, T_vert y, Vertex* prev) {
			this->x = x;
			this->y = y;
			this->prev = prev;
			prev->next = this;
		}

		//! Construct a new vertex
		Vertex(T_vert x, T_vert y) : x(x), y(y) {}
		//! Construct a new vertex. Also updates the previous vertex
		Vertex(T_vert x, T_vert y, Vertex* prev) : x(x), y(y), prev(prev) { prev->next = this; }
	};

	/*! Construct a new polygon from an std::vector

		Takes an std::vector where every x is at an even and every y at the succeeding odd index,
		starting from 0. The vertices are expected to be listed in clockwise(!) orientation. Constructs
		a new Polygon using fmt::Polygon::Vertex by copy-assignment.
	*/
	Polygon(const std::vector<T_vert>& vec);

	/*! Update the vertices of the polygon from a vector

		Takes an std::vector where every x is at an even and every y at the succeeding odd index,
		starting from 0. The vertices are expected to be listed in clockwise(!) orientation. Updates the
		vertices of this polygon by copy-assignment. This may enlarge or shrink the polygon. The
		original polygon is directly modified and not preserved.

		This clears all diagonals of the polygon.
		@throws std::invalid_argument If the vector has less than three vertices or has an odd number of
		elements.
	*/
	void set_vertices(const std::vector<T_vert>& vec);

	/*! Clear all existing diagonals in the polygon

		Updates the inner linked lists of the polygon to reflect the vertice's order in the vector. This
		effectively clears all diagonals inserted by triangulating or partitioning the polygon.
	 */
	void clear_diagonals();

	/*! Append a vertex to the polygon

		Takes a fmt::Polygon::Vertex and appends it to the vertex vector of the polygon. Memory
		management wil be performed by the polygon.
	*/
	void push_back(Vertex* v);

	//! Get the number of vertices in the polygon
	T_ind size() { return poly.size(); }

	//! Get the vertex at the specified index
	Vertex* at(const T_ind i) const { return poly[i]; };
	//! Get the vertex at the specified index
	Vertex* operator[](const T_ind i) const { return poly[i]; }

	/*! Compute or retrieve a set of indices forming a triangulation of the polygon.

		If a valid set of indices exists for the current form of the polygon, it is returned.
		Else, a new set of indices is constructed from the current set of diagonals if they are valid.
		Else, a new set of diagonals is computed first. The result is cached and returned.

		TODO: Implement incremental calculations

		@return A vector of indices, where each successive three elements indicate the indices of a
		triangle.
	*/
	const std::vector<T_ind>& get_indices();

	/*! Computes diagonals for the current polygon.

		@param force If true, diagonals are computed even if the current state is valid. Defaults to
		false.
	*/
	void compute_diagonals(const bool force = false);

	/*! Partitions the polygon into monotone parts.

		The polygon must not have any prior diagonals, else the partitioning will lead to unexpected
		results or crash.

		Note that this method is not meant to be called directly, although it is exposed for use cases
		which require separate partitioning and triangulation for timing or other reasons. In general,
		it is best to triangulate a polygon by calling its member function fmt::Polygon::get_indices.

		@param frac_starts Denominator to guess amount of start vertices for initial array allocation.
		10 means 1/10th of all vertices are start vertices. Defaults to 8.
		@param frac_merges Denominator to guess amount of merge vertices for initial array allocation.
		10 means 1/10th of all vertices are merge vertices. Defaults to 10.
		@param frac_splits Denominator to guess amounf of split vertices for initial array allocation.
		10 means 1/10th of all vertices are split vertices. Defaults to 10.
		@param frac_stops Denominator to guess amounf of stop vertices for initial array allocation. 10
		means 1/10th of all vertices are stop vertices. Defaults to 10.

		@returns A pointer to a vector containing all monotone parts to be passed to fmt::triangulate.
		The caller is responsible for memory management of the vector.
	*/
  // TODO: Base default frac values on actual math
	std::vector<MonoPart<T_ind>>* partition(T_ind frac_starts = 8, T_ind frac_merges = 10,
																					T_ind frac_splits = 10, T_ind frac_stops = 8);

	/*! Take a vector of monotone parts and triangulate them into a set of indices.

		@param parts Pointer to a vector of monotone parts as given by Polygon::partition
	 */
	void triangulate(std::vector<MonoPart<T_ind>>* parts);

	// friend std::vector<fmt::MonoPart<T_ind>>* partition(Polygon& poly, T_ind frac_starts, T_ind
	// frac_splits);

	//! Destroy the polygon
	~Polygon();
};
} // namespace fmt

// Include implementation files
#include "polygon.ipp"
#include "bst.ipp"
#include "rb_interval.ipp"
#include "partition.ipp"
#include "triangulate.ipp"

#endif // end include guard
