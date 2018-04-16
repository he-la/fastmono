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
#include <ratio>
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
// point sets.
//#endif

typedef CGAL::Simple_cartesian<float> K;
typedef K::Point_2 Point_2;
typedef std::list<Point_2> Container;
typedef CGAL::Polygon_2<K, Container> Polygon_2;
typedef CGAL::Creator_uniform_2<float, Point_2> Creator;
typedef CGAL::Random_points_in_square_2<Point_2, Creator> Point_generator;

#include "polypartition/src/polypartition.h" // ear clipping and monotone. Must be anti-clockwise.

//#include "seidel/interface.h" // seidel (C library). Must be anti-clockwise. Requires vertex array
// where arr[0] is unused.
// NOTE: SEIDEL DOES NOT WORK

#include "../polygon.hpp" // FastMono interface

using namespace std;

constexpr float RADIUS = 100;

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

constexpr unsigned int ITERATIONS = 10000;
unsigned int N = 100;

int main(int argc, char* argv[]) {

	const char* filename = argc < 2 ? "data.csv" : argv[1];
	cout << "Starting benchmark at N=" << N << endl
			 << "Data will be written to " << filename
			 << (argc < 2 ? " (pass filename as argument to change)" : "") << endl
			 << "Time is reported in ms." << endl
			 << endl;

	typedef chrono::duration<double, milli> diff;

	list<TPPLPoly>* triangles = new list<TPPLPoly>;
	TPPLPartition pp;
	filebuf fb;
	fb.open(filename, ios::out);

	ostream csvstream(&fb);

	csvstream << "N,GEN,EC,MONO,FMT" << endl;

	if (argc == 3) N = stoi(argv[2]);

	while (N < USHRT_MAX / 2) {
		for (unsigned int i = 0; i < ITERATIONS; ++i) {

			auto start = chrono::steady_clock::now();
			TPPLPoly* tppl_poly = rpg(N);
			auto end = chrono::steady_clock::now();
			diff _t_gen = end - start;

			triangles->clear();

			start = chrono::steady_clock::now();
			pp.Triangulate_EC(tppl_poly, triangles);
			end = chrono::steady_clock::now();
			diff _t_ec = end - start;

			triangles->clear();

			// try {
			start = chrono::steady_clock::now();
			pp.Triangulate_MONO(tppl_poly, triangles);
			end = chrono::steady_clock::now();
			diff _t_mono = end - start;
			//} catch(const exception& e) {
			// cout << endl << "Exception: ";
			// cout << e.what() << endl;
			//}

			vector<float> v_cw;
			v_cw.reserve(tppl_poly->GetNumPoints() * 2);
			for (int j = tppl_poly->GetNumPoints() - 1; j >= 0; --j) {
				v_cw.push_back(static_cast<float>((*tppl_poly)[j].x));
				v_cw.push_back(static_cast<float>((*tppl_poly)[j].y));
			}

			fmt::Polygon<float, unsigned int> fmt_poly = fmt::Polygon<float, unsigned int>(v_cw);

			start = chrono::steady_clock::now();
			fmt_poly.get_indices();
			end = chrono::steady_clock::now();
			diff _t_fmt = end - start;

			csvstream << N << "," << _t_gen.count() << "," << _t_ec.count() << "," << _t_mono.count()
								<< "," << _t_fmt.count() << endl;

			delete tppl_poly;
		}

		cout << "\rDone with N=" << N << ". Now benchmarking N=" << (unsigned int) (N * 1.05) << flush;
		N *= 1.05;
	}
	delete triangles;
	fb.close();
	return 0;
}
