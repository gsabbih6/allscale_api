#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "allscale/api/user/operator/pfor.h"
#include "allscale/api/user/data/vector.h"

#include "allscale/utils/string_utils.h"

namespace allscale {
namespace api {
namespace user {

	// --- basic parallel loop usage ---


	TEST(PFor,Basic) {
		const int N = 200;

		// -- initialize data --

		std::array<int,N> data;
		data.fill(0);

		// check that all are 0
		for(const auto& cur : data) {
			EXPECT_EQ(0,cur);
		}

		// -- direct execution --

		// increase all by 1
		pfor(0,N,[&](int i) {
			data[i]++;
		});

		// check that all have been updated
		for(const auto& cur : data) {
			EXPECT_EQ(1,cur);
		}


		// -- delayed execution --

		// increase all by 1
		auto As = pfor(0,N,[&](const int& i) {
			data[i]++;
		});

		// trigger execution
		As.wait();

		// check that all have been updated
		for(const auto& cur : data) {
			EXPECT_EQ(2,cur);
		}

	}

	template<typename Iter>
	void testIntegral() {
		const int N = 100;

		std::vector<int> data(N);
		for(std::size_t i = 0; i<N; ++i) {
			data[i] = 0;
		}

		for(std::size_t i = 0; i<N; ++i) {
			EXPECT_EQ(0,data[i]);
		}

		pfor(Iter(0),Iter(100),[&](Iter i){
			data[i] = 1;
		});

		for(std::size_t i = 0; i<N; ++i) {
			EXPECT_EQ(1,data[i]);
		}
	}

	TEST(PFor,Integrals) {

		testIntegral<char>();
		testIntegral<short>();
		testIntegral<int>();
		testIntegral<long>();
		testIntegral<long long>();

		testIntegral<unsigned char>();
		testIntegral<unsigned short>();
		testIntegral<unsigned int>();
		testIntegral<unsigned long>();
		testIntegral<unsigned long long>();

		testIntegral<char16_t>();
		testIntegral<char32_t>();
		testIntegral<wchar_t>();

		testIntegral<int8_t>();
		testIntegral<int16_t>();
		testIntegral<int32_t>();
		testIntegral<int64_t>();

		testIntegral<uint8_t>();
		testIntegral<uint16_t>();
		testIntegral<uint32_t>();
		testIntegral<uint64_t>();

		testIntegral<std::size_t>();
	}

	TEST(PFor,Container) {
		const int N = 200;

		// create data
		std::vector<int> data(N);

		// initialize data
		pfor(data,[](int& x) {
			x = 10;
		});

		// check state
		for(const auto& cur : data) {
			EXPECT_EQ(10,cur);
		}

		auto As = pfor(data,[](int& x) { x = 20; });

		As.wait();

		// check state
		for(const auto& cur : data) {
			EXPECT_EQ(20,cur);
		}

	}


	TEST(PFor, Array) {

		const int N = 100;

		using Point = std::array<int,3>;
		using Grid = std::array<std::array<std::array<int,N>,N>,N>;

		Point zero = {{0,0,0}};
		Point full = {{N,N,N}};

		// create data
		Grid* data = new Grid();

		// initialize the data
		for(int i=0; i<N; i++) {
			for(int j=0; j<N; j++) {
				for(int k=0; k<N; k++) {
					(*data)[i][j][k] = 5;
				}
			}
		}

		// update data in parallel
		pfor(zero,full,[&](const Point& p){
			(*data)[p[0]][p[1]][p[2]]++;
		});

		// check that all has been covered
		for(int i=0; i<N; i++) {
			for(int j=0; j<N; j++) {
				for(int k=0; k<N; k++) {
					EXPECT_EQ(6,(*data)[i][j][k])
							<< "Position: " << i << "/" << j << "/" << k;
				}
			}
		}

		delete data;

	}


	TEST(PFor, Vector) {

		const int N = 100;

		using Point = data::Vector<int,3>;
		using Grid = std::array<std::array<std::array<int,N>,N>,N>;

		Point zero = 0;
		Point full = N;

		// create data
		Grid* data = new Grid();

		// initialize the data
		for(int i=0; i<N; i++) {
			for(int j=0; j<N; j++) {
				for(int k=0; k<N; k++) {
					(*data)[i][j][k] = 5;
				}
			}
		}

		// update data in parallel
		pfor(zero,full,[&](const Point& p){
			(*data)[p[0]][p[1]][p[2]]++;
		});

		// check that all has been covered
		for(int i=0; i<N; i++) {
			for(int j=0; j<N; j++) {
				for(int k=0; k<N; k++) {
					EXPECT_EQ(6,(*data)[i][j][k])
							<< "Position: " << i << "/" << j << "/" << k;
				}
			}
		}

		delete data;

	}



	// --- loop iteration sync ---

	TEST(Pfor, SyncOneOnOne) {

		const int N = 10000;
		const bool enable_log = false;

//		const int N = 10;
//		const bool enable_log = true;

		std::mutex outLock;
		auto log = [&](const std::string& str, int i) {
			if (!enable_log) return;
			std::lock_guard<std::mutex> lock(outLock);
			std::cerr << str << i << "\n";
		};

		std::vector<int> data(N);

		auto As = pfor(0,N,[&](int i) {
			log("A",i);
			data[i] = 0;
		});

		auto Bs = pfor(0,N,[&](int i) {
			log("B",i);
			EXPECT_EQ(0,data[i]) << "Index: " << i;
			data[i] = 1;
		}, one_on_one(As));

		auto Cs = pfor(0,N,[&](int i) {
			log("C",i);
			EXPECT_EQ(1,data[i]) << "Index: " << i;
			data[i] = 2;
		}, one_on_one(Bs));

		Cs.wait();

		for(int i=0; i<N; i++) {
			EXPECT_EQ(2, data[i]) << "Index: " << i;
		}
	}


	TEST(Pfor, SyncOneOnOne_DifferentSize) {

		const int N = 10000;

		std::vector<int> data(N+20);

		auto As = pfor(0,N,[&](int i) {
			data[i] = 0;
		});

		// test a smaller one
		auto Bs = pfor(0,N-1,[&](int i) {
			EXPECT_EQ(0,data[i]) << "Index: " << i;
			data[i] = 1;
		}, one_on_one(As));

		// and an even smaller one
		auto Cs = pfor(0,N-2,[&](int i) {
			EXPECT_EQ(1,data[i]) << "Index: " << i;
			data[i] = 2;
		}, one_on_one(Bs));

		// and a bigger one
		auto Ds = pfor(0,N+20,[&](int i) {
			if (i < N-2) EXPECT_EQ(2,data[i]) << "Index: " << i;
			else if (i < N-1) EXPECT_EQ(1,data[i]) << "Index: " << i;
			else if (i < N) EXPECT_EQ(0,data[i]) << "Index: " << i;
			data[i] = 3;
		}, one_on_one(Cs));

		Ds.wait();

		for(int i=0; i<N+20; i++) {
			EXPECT_EQ(3, data[i]) << "Index: " << i;
		}
	}


	TEST(Pfor, SyncNeighbor) {
		const int N = 10000;

		std::vector<int> dataA(N);
		std::vector<int> dataB(N);

		auto As = pfor(0,N,[&](int i) {
			dataA[i] = 1;
		});

		auto Bs = pfor(0,N,[&](int i) {

			// the neighborhood of i has to be completed in A
			if (i != 0) {
				EXPECT_EQ(1,dataA[i-1]) << "Index: " << i;
			}

			EXPECT_EQ(1,dataA[i])   << "Index: " << i;

			if (i != N-1) {
				EXPECT_EQ(1,dataA[i+1]) << "Index: " << i;
			}

			dataB[i] = 2;
		}, neighborhood_sync(As));

		auto Cs = pfor(0,N,[&](int i) {

			// the neighborhood of i has to be completed in B
			if (i != 0) {
				EXPECT_EQ(2,dataB[i-1]) << "Index: " << i;
			}

			EXPECT_EQ(2,dataB[i])   << "Index: " << i;

			if (i != N-1) {
				EXPECT_EQ(2,dataB[i+1]) << "Index: " << i;
			}

			dataA[i] = 3;
		}, neighborhood_sync(Bs));

		// trigger execution
		Cs.wait();

		// check result
		for(int i=0; i<N; i++) {
			EXPECT_EQ(3, dataA[i]);
			EXPECT_EQ(2, dataB[i]);
		}
	}


	TEST(Pfor, SyncNeighbor_DifferentSize) {
		const int N = 10000;

		std::vector<int> dataA(N+20);
		std::vector<int> dataB(N+20);

		auto As = pfor(0,N,[&](int i) {
			dataA[i] = 1;
		});

		auto Bs = pfor(0,N-1,[&](int i) {

			// the neighborhood of i has to be completed in A
			if (i != 0) {
				EXPECT_EQ(1,dataA[i-1]) << "Index: " << i;
			}

			EXPECT_EQ(1,dataA[i])   << "Index: " << i;

			EXPECT_EQ(1,dataA[i+1]) << "Index: " << i;

			dataB[i] = 2;
		}, neighborhood_sync(As));

		auto Cs = pfor(0,N-2,[&](int i) {

			// the neighborhood of i has to be completed in B
			if (i != 0) {
				EXPECT_EQ(2,dataB[i-1]) << "Index: " << i;
			}

			EXPECT_EQ(2,dataB[i])   << "Index: " << i;

			EXPECT_EQ(2,dataB[i+1]) << "Index: " << i;

			dataA[i] = 3;
		}, neighborhood_sync(Bs));

		// also try a larger range
		auto Ds = pfor(0,N+20,[&](int i) {

			// the neighborhood of i has to be completed in A
			if (i != 0 && i <= N-2 ) {
				EXPECT_EQ(3,dataA[i-1]) << "Index: " << i;
			} else if ( i != 0 && i < N ) {
				EXPECT_EQ(1,dataA[i-1]) << "Index: " << i;
			}

			if (i < N-2) {
				EXPECT_EQ(3,dataA[i])   << "Index: " << i;
			} else if (i < N) {
				EXPECT_EQ(1,dataA[i])   << "Index: " << i;
			}

			if (i != N-1 && i < N-3) {
				EXPECT_EQ(3,dataA[i+1]) << "Index: " << i;
			} else if (i != N-1 && i < N) {
				EXPECT_EQ(1,dataA[i+1]) << "Index: " << i;
			}

			dataB[i] = 4;

		}, neighborhood_sync(Cs));

		// trigger execution
		Ds.wait();

		// check result
		for(int i=0; i<N-2; i++) {
			EXPECT_EQ(3, dataA[i]);
		}
		for(int i=N-2; i<N-1; i++) {
			EXPECT_EQ(1, dataA[i]);
		}
		for(int i=0; i<N+20; i++) {
			EXPECT_EQ(4, dataB[i]);
		}
	}

	// --- stencil variants --.

	const int N = 10000;
	const int T = 100;

	TEST(Pfor, Stencil_Barrier) {

		int* A = new int[N];
		int* B = new int[N];

		// start with an initialization
		pfor(0,N,[A,B](int i){
			A[i] = 0;
			B[i] = -1;
		});

		// run the time loop
	    for(int t=0; t<T; ++t) {
	        pfor(1,N-1,[A,B,t](int i) {

	        	if (i != 1)  EXPECT_EQ(t,A[i-1]);
	        	EXPECT_EQ(t,A[i]);
	        	if (i != N-2) EXPECT_EQ(t,A[i+1]);

	        	EXPECT_EQ(t-1,B[i]);

	        	B[i]=t+1;

	        });
	        std::swap(A,B);
	    }

	    // check the final state
	    pfor(1,N-1,[A](int i){
	    	EXPECT_EQ(T,A[i]);
	    });

	    delete [] A;
		delete [] B;
	}

	TEST(Pfor, Stencil_Fine_Grained) {

		int* A = new int[N];
		int* B = new int[N];

		// start with an initialization
		auto ref = pfor(0,N,[A,B](int i){
			A[i] = 0;
			B[i] = -1;
		});

		// run the time loop
	    for(int t=0; t<T; ++t) {
	        ref = pfor(1,N-1,[A,B,t](int i) {

	        	if (i != 1)  EXPECT_EQ(t,A[i-1]);
	        	EXPECT_EQ(t,A[i]);
	        	if (i != N-2) EXPECT_EQ(t,A[i+1]);

	        	EXPECT_EQ(t-1,B[i]);

	        	B[i]=t+1;

	        },neighborhood_sync(ref));
	        std::swap(A,B);
	    }

	    // check the final state
	    pfor(1,N-1,[A](int i){
	    	EXPECT_EQ(T,A[i]);
	    },neighborhood_sync(ref));

	    delete [] A;
	    delete [] B;
	}

	TEST(Range,GrowAndShrink) {

		using range = detail::range<int>;

		range limit(0,5);
		range a(1,2);

		EXPECT_EQ("[0,5)",toString(limit));
		EXPECT_EQ("[1,2)",toString(a));

		EXPECT_EQ("[0,3)",toString(a.grow(limit)));
		EXPECT_EQ("[0,4)",toString(a.grow(limit).grow(limit)));
		EXPECT_EQ("[0,5)",toString(a.grow(limit).grow(limit).grow(limit)));
		EXPECT_EQ("[0,5)",toString(a.grow(limit).grow(limit).grow(limit).grow(limit)));

		EXPECT_EQ("[0,4)",toString(a.grow(limit,2)));
		EXPECT_EQ("[0,5)",toString(a.grow(limit,3)));
		EXPECT_EQ("[0,5)",toString(a.grow(limit,4)));



		EXPECT_EQ("[2,2)",toString(a.shrink()));
		EXPECT_EQ("[1,4)",toString(limit.shrink()));
		EXPECT_EQ("[2,3)",toString(limit.shrink().shrink()));
		EXPECT_EQ("[3,3)",toString(limit.shrink().shrink().shrink()));

		EXPECT_EQ("[2,3)",toString(limit.shrink(2)));
		EXPECT_EQ("[3,3)",toString(limit.shrink(3)));

	}

	TEST(Range,GrowAndShrink_2D) {

		using range = detail::range<std::array<int,2>>;

		range limit({{0,2}},{{5,7}});
		range a({{1,4}},{{2,5}});

		EXPECT_EQ("[[0,2],[5,7])",toString(limit));
		EXPECT_EQ("[[1,4],[2,5])",toString(a));

		EXPECT_EQ("[[0,3],[3,6])",toString(a.grow(limit)));
		EXPECT_EQ("[[0,2],[4,7])",toString(a.grow(limit).grow(limit)));
		EXPECT_EQ("[[0,2],[5,7])",toString(a.grow(limit).grow(limit).grow(limit)));
		EXPECT_EQ("[[0,2],[5,7])",toString(a.grow(limit).grow(limit).grow(limit).grow(limit)));

		EXPECT_EQ("[[0,2],[4,7])",toString(a.grow(limit,2)));
		EXPECT_EQ("[[0,2],[5,7])",toString(a.grow(limit,3)));
		EXPECT_EQ("[[0,2],[5,7])",toString(a.grow(limit,4)));


		EXPECT_EQ("[[2,5],[2,5])",toString(a.shrink()));

		EXPECT_EQ("[[1,3],[4,6])",toString(limit.shrink()));
		EXPECT_EQ("[[2,4],[3,5])",toString(limit.shrink().shrink()));
		EXPECT_EQ("[[3,5],[3,5])",toString(limit.shrink().shrink().shrink()));
		EXPECT_EQ("[[4,6],[4,6])",toString(limit.shrink().shrink().shrink().shrink()));

		EXPECT_EQ("[[1,3],[4,6])",toString(limit.shrink()));
		EXPECT_EQ("[[2,4],[3,5])",toString(limit.shrink(2)));
		EXPECT_EQ("[[3,5],[3,5])",toString(limit.shrink(3)));
		EXPECT_EQ("[[4,6],[4,6])",toString(limit.shrink(4)));

	}

} // end namespace user
} // end namespace api
} // end namespace allscale