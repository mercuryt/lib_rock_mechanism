#pragma once
#include <array>
#include "../geometry/point3D.h"

namespace adjacentOffsets
{
	static inline std::array<Offset3D, 6> direct{{
	//	below		north		east		south		west		above
		Offset3D::create(0,0,-1),	Offset3D::create(0,1,0),	Offset3D::create(1,0,0),	Offset3D::create(0,-1,0),	Offset3D::create(-1,0,0),	Offset3D::create(0,0,1)
	}};
	static inline const std::array<Offset3D, 26> all{{
		Offset3D::create(-1,-1,-1),	Offset3D::create(0,-1,-1),	Offset3D::create(1,-1,-1),
		Offset3D::create(-1,0,-1),	Offset3D::create(0,0,-1),	Offset3D::create(1,0,-1),
		Offset3D::create(-1,1,-1),	Offset3D::create(0,1,-1),	Offset3D::create(1,1,-1),

		Offset3D::create(-1,-1,0),	Offset3D::create(0,-1,0),	Offset3D::create(1,-1,0),
		Offset3D::create(-1,0,0),								Offset3D::create(1,0,0),
		Offset3D::create(-1,1,0),	Offset3D::create(0,1,0),	Offset3D::create(1,1,0),

		Offset3D::create(-1,-1,1),	Offset3D::create(0,-1,1),	Offset3D::create(1,-1,1),
		Offset3D::create(-1,0,1),	Offset3D::create(0,0,1),	Offset3D::create(1,0,1),
		Offset3D::create(-1,1,1),	Offset3D::create(0,1,1),	Offset3D::create(1,1,1),
	}};
	static inline const std::array<Offset3D, 24> allExceptDirectlyAboveAndBelow{{
		Offset3D::create(-1,-1,-1),	Offset3D::create(0,-1,-1),	Offset3D::create(1,-1,-1),
		Offset3D::create(-1,0,-1),								Offset3D::create(1,0,-1),
		Offset3D::create(-1,1,-1),	Offset3D::create(0,1,-1),	Offset3D::create(1,1,-1),

		Offset3D::create(-1,-1,0),	Offset3D::create(0,-1,0),	Offset3D::create(1,-1,0),
		Offset3D::create(-1,0,0),								Offset3D::create(1,0,0),
		Offset3D::create(-1,1,0),	Offset3D::create(0,1,0),	Offset3D::create(1,1,0),

		Offset3D::create(-1,-1,1),	Offset3D::create(0,-1,1),	Offset3D::create(1,-1,1),
		Offset3D::create(-1,0,1),								Offset3D::create(1,0,1),
		Offset3D::create(-1,1,1),	Offset3D::create(0,1,1),	Offset3D::create(1,1,1),
	}};
	static std::array<Offset3D, 18> directAndEdge{{
		Offset3D::create(-1,0,-1),
		Offset3D::create(0,1,-1),	Offset3D::create(0,0,-1),	Offset3D::create(0,-1,-1),
		Offset3D::create(1,0,-1),

		Offset3D::create(-1,-1,0),	Offset3D::create(-1,0,0),	Offset3D::create(0,-1,0),
		Offset3D::create(1,1,0),	Offset3D::create(0,1,0),
		Offset3D::create(1,-1,0),	Offset3D::create(1,0,0),	Offset3D::create(0,-1,0),

		Offset3D::create(-1,0,1),
		Offset3D::create(0,1,1),	Offset3D::create(0,0,1),	Offset3D::create(0,-1,1),
		Offset3D::create(1,0,1),
	}};
	static std::array<Offset3D, 12> edge{{
		Offset3D::create(-1,0,-1),	Offset3D::create(0,-1,-1),
		Offset3D::create(1,0,-1),	Offset3D::create(0,1,-1),

		Offset3D::create(-1,-1,0),	Offset3D::create(1,1,0),
		Offset3D::create(1,-1,0),	Offset3D::create(-1,1,0),

		Offset3D::create(-1,0,1),	Offset3D::create(0,-1,1),
		Offset3D::create(0,1,1),	Offset3D::create(1,0,1),
	}};
	static std::array<Offset3D, 4> edgeWithSameZ{{
		Offset3D::create(-1,-1,0),	Offset3D::create(1,1,0),
		Offset3D::create(1,-1,0),	Offset3D::create(-1,1,0),
	}};
	static std::array<Offset3D, 20> edgeAndCorner{{
		Offset3D::create(-1,-1,-1),
		Offset3D::create(-1,-1,0),
		Offset3D::create(-1,-1,1),
		Offset3D::create(-1,0,-1),
		Offset3D::create(-1,0,1),
		Offset3D::create(-1,1,-1),
		Offset3D::create(-1,1,0),
		Offset3D::create(-1,1,1),
		Offset3D::create(0,-1,-1),
		Offset3D::create(0,-1,1),
		Offset3D::create(0,1,-1),
		Offset3D::create(0,1,1),
		Offset3D::create(1,-1,-1),
		Offset3D::create(1,-1,0),
		Offset3D::create(1,-1,1),
		Offset3D::create(1,0,-1),
		Offset3D::create(1,0,1),
		Offset3D::create(1,1,-1),
		Offset3D::create(1,1,0),
		Offset3D::create(1,1,1),
	}};
	static std::array<Offset3D, 4> directWithSameZ{{
		Offset3D::create(-1,0,0),	Offset3D::create(1,0,0),
		Offset3D::create(0,-1,0),	Offset3D::create(0,1,0)
	}};
}