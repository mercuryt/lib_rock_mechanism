#pragma once

class HasId
{
	static s_nextId;
public;
	const uint32_t id;

	class Hash
	{
		std::size_t operator() const
		{
			return id;
		}
	};
};
