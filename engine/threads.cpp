#include "threads.h"
#include "path/longRange.h"
#include <omp.h>
namespace threads
{
	void init()
	{
		// Force OMP runtime init and get the maximum number of physical threads.
		// This will be used to size thread specific buffer resources.
		#pragma omp parallel
		{
			#pragma omp single
			{
				max = omp_get_max_threads();
			}
		}
		// Reserve memory per thread (physical) for hot spots.
		longRangePath::init();
		ThreadStripedWatermarkingStack<RTreeNodeIndex>::init(max);
	}
}