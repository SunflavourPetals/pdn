#include <iosfwd>
#include <string>

inline std::ostream& operator<<(std::ostream& o, std::u8string_view sv)
{
	o << std::string_view{ (const char*)sv.data(), sv.size() };
	return o;
}
