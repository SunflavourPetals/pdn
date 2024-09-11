#ifndef PDN_Header_pdn_type_code
#define PDN_Header_pdn_type_code

namespace pdn
{
	enum class type_code
	{
		unknown,
		i8,
		i16,
		i32,
		i64,
		u8,
		u16,
		u32,
		u64,
		f32,
		f64,
		boolean,
		character,
		string,
		list,
		object,
	};
}

#endif
