#include <atomic>
class AtomicBool
{
	std::atomic_char value;
public:
	AtomicBool(bool v) : value(v) { }
	AtomicBool() : value(false) { }
	void set(bool v) { value = v; }
	void toggle() { value ^= 1; }
	operator bool() const { return value; }
	bool operator=(bool v) { set(v); return v; }
	bool operator==(bool v) { return v == value; }
};

