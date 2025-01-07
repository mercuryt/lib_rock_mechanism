#include <atomic>
class AtomicBool
{
	std::atomic_char value;
public:
	AtomicBool(bool v) : value(v) { }
	AtomicBool() : value(false) { }
	void set(bool v) { value = v; }
	void toggle(std::memory_order order = std::memory_order_seq_cst) { value.fetch_xor(1, order); }
	operator bool() const { return value; }
	bool operator=(bool v) { set(v); return v; }
	bool operator==(bool v) { return v == value; }
};

