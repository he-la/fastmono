#include <chrono>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <ostream>
#include <utility>
#include <vector>

/* Note: Random polygon generation depends on CGAL (www.cgal.org).
	 You do not need Qt5 and libQGLViewer for the CGAL Core library.
*/

#include <CGAL/Polygon_2.h>
#include <CGAL/Random.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/algorithm.h>
#include <CGAL/point_generators_2.h>
#include <CGAL/random_polygon_2.h>

/*#ifdef CGAL_USE_GMP
	#include <CGAL/Gmpz.h>
	typedef CGAL::Gmpz RT;
	#else*/
#include <CGAL/double.h>
//#warn CGAL::GMP is not supported. This may cause problems in polygon generation for degenerate
//point sets.
typedef double RT;
//#endif

typedef CGAL::Simple_cartesian<RT> K;
typedef K::Point_2 Point_2;
typedef std::list<Point_2> Container;
typedef CGAL::Polygon_2<K, Container> Polygon_2;
typedef CGAL::Creator_uniform_2<int, Point_2> Creator;
typedef CGAL::Random_points_in_square_2<Point_2, Creator> Point_generator;

#include "polypartition/polypartition.h" // ear clipping and monotone. Must be anti-clockwise.

//#include "seidel/interface.h" // seidel (C library). Must be anti-clockwise. Requires vertex array
//where arr[0] is unused.
// NOTE: SEIDEL DOES NOT WORK

#include "../polygon.hpp" // FastMono interface

using namespace std;

constexpr double RADIUS = 1;

// Generate random n-vertex polygon
TPPLPoly* rpg(unsigned int size) { // we apologise for the inconvenience
	Polygon_2 polygon;
	list<Point_2> point_set;

	point_set.clear();
	CGAL::copy_n_unique(Point_generator(RADIUS), size, back_inserter(point_set));

	CGAL::random_polygon_2(point_set.size(), back_inserter(polygon), point_set.begin());

	// Copy points into TPPLPoly
	TPPLPoly* poly = new TPPLPoly();
	poly->Init(size);
	unsigned int i = 0;
	for (auto it = polygon.vertices_begin(); it != polygon.vertices_end(); ++it) {
		/*#ifdef CGAL_USE_GMP
			(*poly)[i].x = (it->x().to_double());
			(*poly)[i].y = (it->y().to_double());
			#else*/
		(*poly)[i].x = (it->x());
		(*poly)[i].y = (it->y());
		//#endif
		++i;
	}
	return poly;
}

#define ITERATIONS 10000
unsigned int N = 100;
int main(int argc, char* argv[]) {
	chrono::duration<double> diff;

	list<TPPLPoly>* triangles = new list<TPPLPoly>;
	TPPLPartition pp;
	filebuf fb;
	if (argc < 2)
		fb.open("_log.csv", ios::out);
	else
		fb.open(argv[1], ios::out);

	ostream csvstream(&fb);

	csvstream << "N,GEN,EC,MONO,FMT" << endl;
	const ostream* ptr = &csvstream;

	if (argc == 3) N = stoi(argv[2]);

	while (N < USHRT_MAX / 2) {
		double _t_gen = 0;
		double _t_ec = 0;
		double _t_mono = 0;
		double _t_fmt = 0;

		for (int i = 0; i < ITERATIONS; ++i) {
			cout << '.';

			auto start = chrono::steady_clock::now();
			TPPLPoly* tppl_poly = rpg(N);
			auto end = chrono::steady_clock::now();
			diff = end - start;
			_t_gen = diff.count();
			cout << "G";

			triangles->clear();

			start = chrono::steady_clock::now();
			pp.Triangulate_EC(tppl_poly, triangles);
			end = chrono::steady_clock::now();
			diff = end - start;
			_t_ec = diff.count();
			cout << "E";

			triangles->clear();

			// try {
			start = chrono::steady_clock::now();
			pp.Triangulate_MONO(tppl_poly, triangles);
			end = chrono::steady_clock::now();
			diff = end - start;
			_t_mono = diff.count();
			//} catch(const exception& e) {
			// cout << endl << "Exception: ";
			// cout << e.what() << endl;
			//}
			cout << "M";

			vector<float> v_cw;
			v_cw.reserve(tppl_poly->GetNumPoints() * 2);
			for (int j = tppl_poly->GetNumPoints() - 1; j >= 0; --j) {
				v_cw.push_back(static_cast<float>((*tppl_poly)[j].x));
				v_cw.push_back(static_cast<float>((*tppl_poly)[j].y));
			}

			fmt::Polygon<float, unsigned int> fmt_poly = fmt::Polygon<float, unsigned int>(v_cw);

			cout << ":";

			start = chrono::steady_clock::now();
			fmt_poly.get_indices();
			end = chrono::steady_clock::now();
			diff = end - start;
			_t_fmt = diff.count();

			cout << '=';
			csvstream << N << "," << _t_gen << "," << _t_ec << "," << _t_mono << "," << _t_fmt << endl;

			delete tppl_poly;
		}

		cout << endl << "====== DONE WITH N=" << N << ". NEXT IS N=" << N * 1.5 << "=====" << endl;

		N *= 1.05;
	}
	delete triangles;
	fb.close();
	return 0;
}
