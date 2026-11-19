// Tests for pdn::byte_swap_chain.
//
// The chain turns a byte istream into a byte source backed by two adjacent buffers. It must
// deliver every byte exactly once across buffer wraps (including '\0' and every other byte
// value), report eof() only after the last byte, and turn stream failures into stop reasons
// instead of throwing. get()/eof() never touch the stream; only to_next() may request a stop.

#include <cassert>
#include <cstddef>
#include <ios>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <pdn/pdn_adjacent_2_buffer.h>
#include <pdn/pdn_byte_swap_chain.h>
#include <pdn/pdn_stop_reason.h>

namespace
{
	using chain_type = pdn::byte_swap_chain<std::istringstream>;

	auto drain(pdn::byte_swap_chain<std::istringstream>&& chain) -> std::string
	{
		auto it = pdn::make_byte_iterator(::std::move(chain));
		auto end = pdn::make_byte_iterator_end();
		auto result = std::string{};
		while (it != end)
		{
			result.push_back(*it);
			++it;
		}
		assert(!it.stop_requested());
		return result;
	}

	auto make_text(std::size_t length) -> std::string
	{
		auto text = std::string{};
		for (auto i = std::size_t{}; i != length; ++i)
		{
			text.push_back(static_cast<char>('a' + i % 26));
		}
		return text;
	}

	void test_delivers_every_byte_across_wraps()
	{
		for (auto chunk : { 1u, 2u, 3u, 4u, 5u, 16u })
		{
			for (auto length : { 0u, 1u, 2u, 3u, 4u, 5u, 8u, 9u, 16u, 17u, 33u })
			{
				auto text    = make_text(length);
				auto stream  = std::istringstream{ text };
				auto chain   = pdn::make_byte_swap_chain(stream, chunk);
				assert(chain.chunk_size() == chunk);
				assert(drain(::std::move(chain)) == text);
			}
		}
	}

	void test_every_byte_value_is_preserved()
	{
		auto text = std::string{};
		for (auto i = 0; i != 256; ++i)
		{
			text.push_back(static_cast<char>(i));
		}
		auto stream = std::istringstream{ text };
		assert(drain(pdn::make_byte_swap_chain(stream, 7)) == text);
	}

	void test_empty_stream_is_eof_without_stop()
	{
		auto stream = std::istringstream{ "" };
		auto chain  = pdn::make_byte_swap_chain(stream, 4);
		assert(chain.eof());
		assert(!chain.stop_requested());
		assert(chain.stop_reason() == pdn::stop_reason_code{});
	}

	void test_default_chunk_size_is_16KiB()
	{
		auto stream = std::istringstream{ "x" };
		auto chain  = pdn::make_byte_swap_chain(stream);
		assert(chain.chunk_size() == 16 * 1024);
	}

	void test_bad_stream_at_construction_becomes_stop()
	{
		auto stream = std::istringstream{ "x" };
		stream.setstate(std::ios::badbit);
		auto chain = pdn::make_byte_swap_chain(stream, 4);
		assert(chain.stop_requested());
		assert(chain.stop_reason() == pdn::stop_reason_code::bad_stream);
	}

	void test_failed_stream_at_construction_becomes_stop()
	{
		auto stream = std::istringstream{ "x" };
		stream.setstate(std::ios::failbit);
		auto chain = pdn::make_byte_swap_chain(stream, 4);
		assert(chain.stop_requested());
		assert(chain.stop_reason() == pdn::stop_reason_code::stream_operation_failed);
	}

	void test_explicit_buffer_is_used()
	{
		auto stream = std::istringstream{ "abcdefg" };
		auto chain  = chain_type{ stream, pdn::detail::adjacent_2_buffer<char>{ 3 } };
		assert(chain.chunk_size() == 3);
		auto text = std::string{};
		while (!chain.eof() && !chain.stop_requested())
		{
			text.push_back(chain.get());
			chain.to_next();
		}
		assert(text == "abcdefg");
		assert(!chain.stop_requested());
	}

	struct counting_deleter
	{
		int* count{};
		void operator()(char* ptr) const noexcept
		{
			if (ptr)
			{
				++*count;
				delete[] ptr;
			}
		}
	};

	using custom_chain_type  = pdn::byte_swap_chain<std::istringstream, counting_deleter>;
	using custom_buffer_type = pdn::detail::adjacent_2_buffer<char, counting_deleter>;

	// make_byte_swap_chain owns the "the chain always gets a usable buffer" contract: a zero sized request
	// becomes 16 KiB, so byte_swap_chain itself may assert on an empty buffer.
	void test_factory_replaces_zero_buffer_size()
	{
		auto stream = std::istringstream{ "xyz" };
		auto chain  = pdn::make_byte_swap_chain(stream, 0);
		assert(chain.chunk_size() == 16 * 1024);
		assert(drain(::std::move(chain)) == "xyz");
	}

	void test_custom_deleter_buffer_is_used()
	{
		auto freed = 0;
		auto raw   = new char[8];
		auto text  = std::string{};
		{
			auto stream = std::istringstream{ "0123456789" };
			auto chain  = custom_chain_type{ stream, custom_buffer_type{ 4, raw, counting_deleter{ &freed } } };
			assert(chain.chunk_size() == 4);
			while (!chain.eof() && !chain.stop_requested())
			{
				text.push_back(chain.get());
				chain.to_next();
			}
			assert(!chain.stop_requested());
		}
		assert(text == "0123456789");
		assert(freed == 1);
	}

	void test_custom_deleter_cannot_allocate_a_buffer()
	{
		static_assert(std::is_constructible_v<chain_type, std::istringstream&>);
		static_assert(std::is_constructible_v<chain_type, std::istringstream&, pdn::detail::adjacent_2_buffer<char>&&>);
		static_assert(!std::is_constructible_v<custom_chain_type, std::istringstream&>);
		static_assert(std::is_constructible_v<custom_chain_type, std::istringstream&, custom_buffer_type&&>);
	}

	void test_advancing_past_eof_changes_nothing()
	{
		auto stream = std::istringstream{ "ab" };
		auto chain  = pdn::make_byte_swap_chain(stream, 4);
		chain.to_next();
		chain.to_next();
		assert(chain.eof());
		chain.to_next();
		chain.to_next();
		assert(chain.eof());
		assert(!chain.stop_requested());
	}

	// A device that delivers some bytes and then fails for good.
	class failing_streambuf : public std::streambuf
	{
	public:
		explicit failing_streambuf(std::string data) : data_{ ::std::move(data) } {}
	protected:
		auto xsgetn(char* dest, std::streamsize count) -> std::streamsize override
		{
			auto produced = std::streamsize{};
			while (produced != count && delivered_ != data_.size())
			{
				dest[produced++] = data_[delivered_++];
			}
			if (produced != count)
			{
				throw std::runtime_error{ "device failure" };
			}
			return produced;
		}
	private:
		std::string data_;
		std::size_t delivered_{};
	};

	void test_device_failure_mid_stream_requests_stop()
	{
		auto buffer = failing_streambuf{ "abc" };
		auto stream = std::istream{ &buffer };
		auto chain  = pdn::byte_swap_chain{ stream, pdn::detail::adjacent_2_buffer<char>{ 2 } };
		auto text   = std::string{};
		while (!chain.eof() && !chain.stop_requested())
		{
			text.push_back(chain.get());
			chain.to_next();
		}
		assert(chain.stop_requested());
		assert(chain.stop_reason() == pdn::stop_reason_code::bad_stream);
		assert(text == "ab");
	}

	void test_types_and_value_category()
	{
		static_assert(std::same_as<chain_type::size_type, std::size_t>);
		static_assert(std::same_as<chain_type::char_type, char>);
		static_assert(std::same_as<chain_type::value_type, char>);
		static_assert(std::same_as<decltype(::std::declval<const chain_type&>().get()), const char&>);
		static_assert(!std::is_copy_constructible_v<chain_type>);
		static_assert(std::is_move_constructible_v<chain_type>);
		static_assert(std::is_move_assignable_v<chain_type>);
	}
}

auto main() -> int
{
	test_delivers_every_byte_across_wraps();
	test_every_byte_value_is_preserved();
	test_empty_stream_is_eof_without_stop();
	test_default_chunk_size_is_16KiB();
	test_bad_stream_at_construction_becomes_stop();
	test_failed_stream_at_construction_becomes_stop();
	test_explicit_buffer_is_used();
	test_factory_replaces_zero_buffer_size();
	test_custom_deleter_buffer_is_used();
	test_custom_deleter_cannot_allocate_a_buffer();
	test_advancing_past_eof_changes_nothing();
	test_device_failure_mid_stream_requests_stop();
	test_types_and_value_category();
	return 0;
}