#ifndef PDN_Header_pdn_types
#define PDN_Header_pdn_types

#include <cstddef>
#include <cstdint>
#include <memory>
#include <array>
#include <utility>
#include <concepts>
#include <type_traits>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <limits>
#include <cmath>
#include <optional>

#include "pdn_unicode_base.h"
#include "pdn_proxy.h"
#include "pdn_exception.h"

namespace pdn::types
{
	using i8      = ::std::int8_t;
	using i16     = ::std::int16_t;
	using i32     = ::std::int32_t;
	using i64     = ::std::int64_t;
	using u8      = ::std::uint8_t;
	using u16     = ::std::uint16_t;
	using u32     = ::std::uint32_t;
	using u64     = ::std::uint64_t;
	using f32     = float;
	using f64     = double;
	using boolean = bool;
}

namespace pdn::types::concepts
{
	template <typename int_t>
	concept pdn_sint
		 = ::std::same_as<int_t, i8>
		|| ::std::same_as<int_t, i16>
		|| ::std::same_as<int_t, i32>
		|| ::std::same_as<int_t, i64>;

	template <typename int_t>
	concept pdn_uint
		 = ::std::same_as<int_t, u8>
		|| ::std::same_as<int_t, u16>
		|| ::std::same_as<int_t, u32>
		|| ::std::same_as<int_t, u64>;

	template <typename t>
	concept pdn_integral = pdn_sint<t> || pdn_uint<t>;

	template <typename fp_t>
	concept pdn_fp
		 = ::std::same_as<fp_t, f32>
		|| ::std::same_as<fp_t, f64>;

	template <typename bool_t>
	concept pdn_bool = ::std::same_as<bool_t, boolean>;
}

namespace pdn::types::type_traits
{
	template <typename int_t>                               struct simu_cpp_int        { using type = i32; }; // default cppint = i32
	template <typename int_t> requires (sizeof(int_t) == 1) struct simu_cpp_int<int_t> { using type = i8;  };
	template <typename int_t> requires (sizeof(int_t) == 2) struct simu_cpp_int<int_t> { using type = i16; };
	template <typename int_t> requires (sizeof(int_t) == 4) struct simu_cpp_int<int_t> { using type = i32; };
	template <typename int_t> requires (sizeof(int_t) == 8) struct simu_cpp_int<int_t> { using type = i64; };
	template <typename int_t> using simu_cpp_int_t    =   typename simu_cpp_int<int_t>::type;

	template <typename int_t>                               struct simu_cpp_uint        { using type = u32; }; // default cppuint = u32
	template <typename int_t> requires (sizeof(int_t) == 1) struct simu_cpp_uint<int_t> { using type = u8;  };
	template <typename int_t> requires (sizeof(int_t) == 2) struct simu_cpp_uint<int_t> { using type = u16; };
	template <typename int_t> requires (sizeof(int_t) == 4) struct simu_cpp_uint<int_t> { using type = u32; };
	template <typename int_t> requires (sizeof(int_t) == 8) struct simu_cpp_uint<int_t> { using type = u64; };
	template <typename int_t> using simu_cpp_uint_t   =   typename simu_cpp_uint<int_t>::type;
}

namespace pdn::types
{
	using cppint  = type_traits::simu_cpp_int_t <int>;
	using cppuint = type_traits::simu_cpp_uint_t<unsigned int>;

	template <typename char_t>
	class character
	{
	private:
		static_assert(sizeof(char_t) == 1 || sizeof(char_t) == 2 || sizeof(char_t) == 4);
		static constexpr auto max_size = 4 / sizeof(char_t);
		using data_type = ::std::array<char_t, max_size>;
	public:
		using small_size_type = u32;
		using value_type      = char_t;
		using size_type       = typename data_type::size_type;
		using difference_type = ptrdiff_t;
		using pointer         = value_type*;
		using const_pointer   = const value_type*;
		using reference       = value_type&;
		using const_reference = const value_type&;
	public:
		constexpr character() : sz{ 1 } {}
		template <typename it_t>
		constexpr character(it_t first, size_type count) noexcept
		{
			auto c = small_size_type(count > max_size ? max_size : count);
			auto data_it = cont.begin();
			for (small_size_type i = 0; i < c; ++i)
			{
				*data_it = *first;
				++first;
				++data_it;
			}
			sz = c;
		}
		constexpr character(const character& rhs) : cont{ rhs.cont }, sz{ rhs.sz } {}
		constexpr character& operator= (const character& rhs) noexcept
		{
			cont = rhs.cont;
			sz = rhs.sz;
			return *this;
		}
		const value_type* data() const noexcept
		{
			return cont.data();
		}
		size_type size() const noexcept
		{
			return sz;
		}
		template <typename traits = ::std::char_traits<char_t>>
		constexpr auto to_string_view() const noexcept -> ::std::basic_string_view<char_t, traits>
		{
			return { data(), size()};
		}
	private:
		data_type cont{};
		small_size_type sz{};
	};
}

namespace pdn::types
{
	template <typename char_t>
	struct entity;
}

namespace pdn::types
{
	template <typename char_t> using string = ::std::basic_string<char_t>;
	template <typename char_t> using list   = ::std::vector<entity<char_t>>;
	template <typename char_t> using object = ::std::unordered_map<string<char_t>, entity<char_t>>;
}

namespace pdn::types::dev_util
{
	template <typename char_t>
	using entity_variant = ::std::variant<
		i8,  i16, i32, i64,
		u8,  u16, u32, u64,
		f32, f64,
		boolean,
		character<char_t>,
		proxy<string<char_t>>,
		proxy<list<char_t>>,
		proxy<object<char_t>>>;
}

namespace pdn::types::dev_util
{
	template <typename t, typename char_t>
	concept basic_types
		 = concepts::pdn_integral<t>
		|| concepts::pdn_fp<t>
		|| concepts::pdn_bool<t>
		|| ::std::same_as<t, character<char_t>>;
}

namespace pdn
{
	template <typename char_t>
	class const_refer
	{
	public:
		using char_type    = char_t;
		using size_type    = ::std::size_t;
		using entity       = types::entity<char_type>;
		using i64          = types::i64;
		using u64          = types::u64;
		using f64          = types::f64;
		using boolean      = types::boolean;
		using character    = types::character<char_type>;
		using string       = types::string<char_type>;
		using list         = types::list<char_type>;
		using object       = types::object<char_type>;
		using string_proxy = proxy<string>;
		using list_proxy   = proxy<list>;
		using object_proxy = proxy<object>;
	public:
		auto sint_val  () const -> i64           { return ptr ? ptr->sint_val  () : i64{}; }
		auto uint_val  () const -> u64           { return ptr ? ptr->uint_val  () : u64{}; }
		auto fp_val    () const -> f64           { return ptr ? ptr->fp_val    () : f64{}; }
		auto bool_val  () const -> boolean       { return ptr ? ptr->bool_val  () : boolean{}; }
		auto char_val  () const -> character     { return ptr ? ptr->char_val  () : character{}; }
		auto string_val() const -> const string& { return ptr ? ptr->string_val() : entity::null_string_val(); }
		auto list_val  () const -> const list&   { return ptr ? ptr->list_val  () : entity::null_list_val(); }
		auto object_val() const -> const object& { return ptr ? ptr->object_val() : entity::null_object_val(); }

		template <types::dev_util::basic_types<char_type> type>
		::std::optional<type> get_optional() const { return ptr ? ptr->template get_optional<type>() : ::std::nullopt; }
		auto i8_opt    () const { return ptr ? ptr->i8_opt  () : ::std::nullopt; }
		auto i16_opt   () const { return ptr ? ptr->i16_opt () : ::std::nullopt; }
		auto i32_opt   () const { return ptr ? ptr->i32_opt () : ::std::nullopt; }
		auto i64_opt   () const { return ptr ? ptr->i64_opt () : ::std::nullopt; }
		auto u8_opt    () const { return ptr ? ptr->u8_opt  () : ::std::nullopt; }
		auto u16_opt   () const { return ptr ? ptr->u16_opt () : ::std::nullopt; }
		auto u32_opt   () const { return ptr ? ptr->u32_opt () : ::std::nullopt; }
		auto u64_opt   () const { return ptr ? ptr->u64_opt () : ::std::nullopt; }
		auto f32_opt   () const { return ptr ? ptr->f32_opt () : ::std::nullopt; }
		auto f64_opt   () const { return ptr ? ptr->f64_opt () : ::std::nullopt; }
		auto bool_opt  () const { return ptr ? ptr->bool_opt() : ::std::nullopt; }
		auto char_opt  () const { return ptr ? ptr->char_opt() : ::std::nullopt; }
		auto string_ptr() const -> const string* { return ptr ? ptr->string_ptr() : nullptr; }
		auto list_ptr  () const -> const list*   { return ptr ? ptr->list_ptr  () : nullptr; }
		auto object_ptr() const -> const object* { return ptr ? ptr->object_ptr() : nullptr; }

		auto i(size_type index)   const -> const_refer { return ptr ? const_refer{ ptr->i(index) } : const_refer{}; }
		auto m(const string& key) const -> const_refer { return ptr ? const_refer{ ptr->m(key) }   : const_refer{}; }

		auto operator[](size_type index)   const -> const_refer { return i(index); }
		auto operator[](const string& key) const -> const_refer { return m(key); }

		const entity* get() const noexcept { return ptr; }
		const entity* operator->() const noexcept { return ptr; }

		explicit operator bool() const noexcept { return ptr; }

		const_refer() = default;
		const_refer(const const_refer& m) : ptr{ m.ptr } {}
		explicit const_refer(entity& e) : ptr{ &e } {}
		const_refer& operator=(const const_refer& m) = default;
	protected:
		entity* ptr_val() const noexcept { return ptr; }
	private:
		entity* ptr{};
	};

	template <typename char_t>
	class refer : public const_refer<char_t>
	{
	private:
		using base_type = const_refer<char_t>;
	public:
		using typename base_type::char_type;
		using typename base_type::size_type;
		using typename base_type::entity;
		using typename base_type::string;
		using typename base_type::list;
		using typename base_type::object;
	public:
		auto string_ptr() -> string* { return get() ? get()->string_ptr() : nullptr; }
		auto list_ptr  () -> list*   { return get() ? get()->list_ptr  () : nullptr; }
		auto object_ptr() -> object* { return get() ? get()->object_ptr() : nullptr; }

		auto i(size_type index)   -> refer { return get() ? refer{ get()->i(index) } : refer{}; }
		auto m(const string& key) -> refer { return get() ? refer{ get()->m(key) }   : refer{}; }

		auto operator[](size_type index)   -> refer { return i(index); }
		auto operator[](const string& key) -> refer { return m(key); }

		entity* get() noexcept { return ptr(); }
		entity* operator->() noexcept { return ptr(); }

		using base_type::base_type;
	private:
		entity* ptr() const noexcept { return this->base_type::ptr_val(); }
	};
}

namespace pdn::types
{
	template <typename char_t = unicode::utf_8_code_unit_t>
	struct entity : public dev_util::entity_variant<char_t>
	{
	public:
		using dev_util::entity_variant<char_t>::entity_variant;
		using char_type    = char_t;
		using size_type    = ::std::size_t;
		using i8           = types::i8;
		using i16          = types::i16;
		using i32          = types::i32;
		using i64          = types::i64;
		using u8           = types::u8;
		using u16          = types::u16;
		using u32          = types::u32;
		using u64          = types::u64;
		using f32          = types::f32;
		using f64          = types::f64;
		using boolean      = types::boolean;
		using character    = types::character<char_type>;
		using string       = types::string<char_type>;
		using list         = types::list<char_type>;
		using object       = types::object<char_type>;
		using string_proxy = proxy<string>;
		using list_proxy   = proxy<list>;
		using object_proxy = proxy<object>;

	public:
		auto  ref()       & ->       refer<char_type> { return       refer<char_type>{ *this }; }
		auto  ref() const & -> const_refer<char_type> { return const_refer<char_type>{ const_cast<entity&>(*this) }; }
		auto cref() const & -> const_refer<char_type> { return const_refer<char_type>{ const_cast<entity&>(*this) }; }

		template <dev_util::basic_types<char_type> type>
		::std::optional<type> get_optional() const
		{
			return ::std::visit([](const auto& arg) -> ::std::optional<type>
			{
				if constexpr (::std::same_as<type, ::std::decay_t<decltype(arg)>>) return ::std::make_optional<type>(arg);
				return ::std::nullopt;
			}, *this);
		}
		auto i8_opt  () const { return get_optional<i8>(); }
		auto i16_opt () const { return get_optional<i16>(); }
		auto i32_opt () const { return get_optional<i32>(); }
		auto i64_opt () const { return get_optional<i64>(); }
		auto u8_opt  () const { return get_optional<u8>(); }
		auto u16_opt () const { return get_optional<u16>(); }
		auto u32_opt () const { return get_optional<u32>(); }
		auto u64_opt () const { return get_optional<u64>(); }
		auto f32_opt () const { return get_optional<f32>(); }
		auto f64_opt () const { return get_optional<f64>(); }
		auto bool_opt() const { return get_optional<boolean>(); }
		auto char_opt() const { return get_optional<character>(); }

		string* string_ptr()
		{
			auto prp = ::std::get_if<string_proxy>(this);
			return prp ? (*prp ? &(**prp) : nullptr) : nullptr;
		}
		const string* string_ptr() const
		{
			auto prp = ::std::get_if<string_proxy>(this);
			return prp ? (*prp ? &(**prp) : nullptr) : nullptr;
		}
		list* list_ptr()
		{
			auto prp = ::std::get_if<list_proxy>(this);
			return prp ? (*prp ? &(**prp) : nullptr) : nullptr;
		}
		const list* list_ptr() const
		{
			auto prp = ::std::get_if<list_proxy>(this);
			return prp ? (*prp ? &(**prp) : nullptr) : nullptr;
		}
		object* object_ptr()
		{
			auto prp = ::std::get_if<object_proxy>(this);
			return prp ? (*prp ? &(**prp) : nullptr) : nullptr;
		}
		const object* object_ptr() const
		{
			auto prp = ::std::get_if<object_proxy>(this);
			return prp ? (*prp ? &(**prp) : nullptr) : nullptr;
		}

		i64 sint_val() const
		{
			return ::std::visit([](const auto& arg) -> i64
			{
				using arg_t = ::std::decay_t<decltype(arg)>;
				using namespace concepts;
				if constexpr (pdn_sint<arg_t> || pdn_bool<arg_t>)
				{
					return arg;
				}
				else if constexpr (pdn_fp<arg_t>)
				{
					constexpr auto max = ::std::numeric_limits<i64>::max();
					constexpr auto min = ::std::numeric_limits<i64>::min();
					if (arg >= arg_t(max)) return max;
					if (arg <= arg_t(min)) return min;
					if (::std::isnan(arg)) return 0;
					return i64(arg);
				}
				else if constexpr (pdn_uint<arg_t>)
				{
					constexpr auto max = ::std::numeric_limits<i64>::max();
					return arg > u64(max) ? max : i64(arg);
				}
				else
				{
					return 0;
				}
			}, *this);
		}
		u64 uint_val() const
		{
			return ::std::visit([](const auto& arg) -> u64
			{
				using arg_t = ::std::decay_t<decltype(arg)>;
				using namespace concepts;
				if constexpr (pdn_uint<arg_t> || pdn_bool<arg_t>)
				{
					return arg;
				}
				else if constexpr (pdn_sint<arg_t> || pdn_fp<arg_t>)
				{
					if constexpr (pdn_fp<arg_t>)
					{
						constexpr auto max = ::std::numeric_limits<u64>::max();
						if (arg > arg_t(max)) return max;
						if (::std::isnan(arg)) return 0;
					}
					return arg < 0 ? 0 : u64(arg);
				}
				else
				{
					return 0;
				}
			}, *this);
		}
		f64 fp_val() const
		{
			return ::std::visit([](const auto& arg) -> f64
			{
				using arg_t = ::std::decay_t<decltype(arg)>;
				using namespace concepts;
				if constexpr (pdn_bool<arg_t> || pdn_integral<arg_t> || pdn_fp<arg_t>)
				{
					return arg;
				}
				else
				{
					return 0;
				}
			}, *this);
		}
		boolean bool_val() const
		{
			return ::std::visit([](const auto& arg) -> boolean
			{
				using arg_t = ::std::decay_t<decltype(arg)>;
				using namespace concepts;
				if constexpr (pdn_bool<arg_t> || pdn_integral<arg_t> || pdn_fp<arg_t>)
				{
					return arg;
				}
				else
				{
					return false;
				}
			}, *this);
		}
		character char_val() const
		{
			return ::std::visit([](const auto& arg) -> character
			{
				using arg_t = ::std::decay_t<decltype(arg)>;
				using namespace concepts;
				if constexpr (::std::same_as<arg_t, character>)
				{
					return arg;
				}
				else
				{
					return character{};
				}
			}, *this);
		}
		const string& string_val() const
		{
			return ::std::visit([](const auto& arg) -> const string&
			{
				using arg_t = ::std::decay_t<decltype(arg)>;
				using namespace concepts;
				if constexpr (::std::same_as<arg_t, string_proxy>)
				{
					return arg ? *arg : null_string_val();
				}
				else
				{
					return null_string_val();
				}
			}, *this);
		}
		const list& list_val() const
		{
			return ::std::visit([](const auto& arg) -> const list&
			{
				using arg_t = ::std::decay_t<decltype(arg)>;
				using namespace concepts;
				if constexpr (::std::same_as<arg_t, list_proxy>)
				{
					return arg ? *arg : null_list_val();
				}
				else
				{
					return null_list_val();
				}
			}, *this);
		}
		const object& object_val() const
		{
			return ::std::visit([](const auto& arg) -> const object&
			{
				using arg_t = ::std::decay_t<decltype(arg)>;
				using namespace concepts;
				if constexpr (::std::same_as<arg_t, object_proxy>)
				{
					return arg ? *arg : null_object_val();
				}
				else
				{
					return null_object_val();
				}
			}, *this);
		}
		const_refer<char_type> i(size_type index) const
		{
			if (auto prp = ::std::get_if<list_proxy>(this); prp && *prp) // pointer to proxy of list
			{
				if (list& arr = **prp; index < arr.size())
				{
					return const_refer<char_type>{ arr[index] };
				}
			}
			return const_refer<char_type>{};
		}
		refer<char_type> i(size_type index)
		{
			if (auto prp = ::std::get_if<list_proxy>(this); prp && *prp) // pointer to proxy of list
			{
				if (list& arr = **prp; index < arr.size())
				{
					return refer<char_type>{ arr[index] };
				}
			}
			return refer<char_type>{};
		}
		const_refer<char_type> m(const string& key) const
		{
			if (auto prp = ::std::get_if<object_proxy>(this); prp && *prp) // pointer to proxy of object
			{
				object& o = **prp;
				if (auto it = o.find(key); it != o.end())
				{
					return const_refer<char_type>{ it->second };
				}
			}
			return const_refer<char_type>{};
		}
		refer<char_type> m(const string& key)
		{
			if (auto prp = ::std::get_if<object_proxy>(this); prp && *prp) // pointer to proxy of object
			{
				object& o = **prp;
				if (auto it = o.find(key); it != o.end())
				{
					return refer<char_type>{ it->second };
				}
			}
			return refer<char_type>{};
		}

		auto get_i8()  const -> const i8&  { return ::std::get<i8>(*this); }
		auto get_i8()        ->       i8&  { return ::std::get<i8>(*this); }

		auto get_i16() const -> const i16& { return ::std::get<i16>(*this); }
		auto get_i16()       ->       i16& { return ::std::get<i16>(*this); }

		auto get_i32() const -> const i32& { return ::std::get<i32>(*this); }
		auto get_i32()       ->       i32& { return ::std::get<i32>(*this); }

		auto get_i64() const -> const i64& { return ::std::get<i64>(*this); }
		auto get_i64()       ->       i64& { return ::std::get<i64>(*this); }

		auto get_u8()  const -> const u8&  { return ::std::get<u8>(*this); }
		auto get_u8()        ->       u8&  { return ::std::get<u8>(*this); }

		auto get_u16() const -> const u16& { return ::std::get<u16>(*this); }
		auto get_u16()       ->       u16& { return ::std::get<u16>(*this); }

		auto get_u32() const -> const u32& { return ::std::get<u32>(*this); }
		auto get_u32()       ->       u32& { return ::std::get<u32>(*this); }

		auto get_u64() const -> const u64& { return ::std::get<u64>(*this); }
		auto get_u64()       ->       u64& { return ::std::get<u64>(*this); }

		auto get_f32() const -> const f32& { return ::std::get<f32>(*this); }
		auto get_f32()       ->       f32& { return ::std::get<f32>(*this); }

		auto get_f64() const -> const f64& { return ::std::get<f64>(*this); }
		auto get_f64()       ->       f64& { return ::std::get<f64>(*this); }

		auto get_bool() const -> const boolean& { return ::std::get<boolean>(*this); }
		auto get_bool()       ->       boolean& { return ::std::get<boolean>(*this); }

		auto get_char() const -> const character& { return ::std::get<character>(*this); }
		auto get_char()       ->       character& { return ::std::get<character>(*this); }

		auto get_string() -> string&
		{
			auto& pr = ::std::get<string_proxy>(*this);
			if (!pr)
			{
				throw null_proxy_error{ "[pdn] null proxy error in get_string" };
			}
			return *pr;
		}
		auto get_string() const -> const string&
		{
			auto& pr = ::std::get<string_proxy>(*this);
			if (!pr)
			{
				throw null_proxy_error{ "[pdn] null proxy error in get_string const" };
			}
			return *pr;
		}

		auto get_list() -> list&
		{
			auto& pr = ::std::get<list_proxy>(*this);
			if (!pr)
			{
				throw null_proxy_error{ "[pdn] null proxy error in get_list" };
			}
			return *pr;
		}
		auto get_list() const -> const list&
		{
			auto& pr = ::std::get<list_proxy>(*this);
			if (!pr)
			{
				throw null_proxy_error{ "[pdn] null proxy error in get_list const" };
			}
			return *pr;
		}

		auto get_object() -> object&
		{
			auto& pr = ::std::get<object_proxy>(*this);
			if (!pr)
			{
				throw null_proxy_error{ "[pdn] null proxy error in get_object" };
			}
			return *pr;
		}
		auto get_object() const -> const object&
		{
			auto& pr = ::std::get<object_proxy>(*this);
			if (!pr)
			{
				throw null_proxy_error{ "[pdn] null proxy error in get_object const" };
			}
			return *pr;
		}

		auto operator[](size_type index) const -> const entity& { return get_list()[index]; }
		auto operator[](size_type index)       ->       entity& { return get_list()[index]; }

		auto operator[](const string& key) const -> const entity&
		{
			auto& o = get_object();
			auto it = o.find(key);
			if (it == o.end())
			{
				throw ::std::out_of_range{ "[pdn] key is not member of object" };
			}
			return it->second;
		}
		auto operator[](const string& key) -> entity&
		{
			auto& o = get_object();
			auto it = o.find(key);
			if (it == o.end())
			{
				throw ::std::out_of_range{ "[pdn] key is not member of object" };
			}
			return it->second;
		}

		static const string& null_string_val()
		{
			static const string null_val{};
			return null_val;
		}
		static const list& null_list_val()
		{
			static const list null_val{};
			return null_val;
		}
		static const object& null_object_val()
		{
			static const object null_val{};
			return null_val;
		}
	};
}

#endif
