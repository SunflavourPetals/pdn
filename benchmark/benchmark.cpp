#include <pdn/spdn.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

namespace
{
	using clock_type = std::chrono::steady_clock;

	// Keep the result of every operation observable so the optimizer cannot
	// remove the work being measured.
	volatile std::uint64_t checksum = 0;

	void append_ascii(std::u8string& out, std::string_view text)
	{
		out.append(reinterpret_cast<const char8_t*>(text.data()), text.size());
	}

	void append_uint(std::u8string& out, std::size_t value)
	{
		append_ascii(out, std::to_string(value));
	}

	std::u8string make_scalars(std::size_t count, bool typed)
	{
		std::u8string out;
		out.reserve(count * 24);

		for (std::size_t i = 0; i < count; ++i)
		{
			out += u8"k";
			append_uint(out, i);
			out += u8": ";

			if (typed)
			{
				switch (i % 3)
				{
				case 0:
					out += u8"i32 ";
					break;
				case 1:
					out += u8"u64 ";
					break;
				default:
					out += u8"f64 ";
					break;
				}
			}

			append_uint(out, i);
			out += u8";\n";
		}

		return out;
	}

	std::u8string make_list(std::size_t count)
	{
		std::u8string out;
		out.reserve(count * 8 + 32);
		out += u8"items: [\n";

		for (std::size_t i = 0; i < count; ++i)
		{
			append_uint(out, i);
			out += u8",\n";
		}

		out += u8"];\n";
		return out;
	}

	void append_node(std::u8string& out, int depth, std::size_t& id)
	{
		out += u8"{ id: ";
		append_uint(out, id++);
		out += u8";";

		if (depth > 0)
		{
			out += u8" children: [";
			for (int i = 0; i < 3; ++i)
			{
				if (i != 0)
				{
					out += u8", ";
				}
				append_node(out, depth - 1, id);
			}
			out += u8"];";
		}

		out += u8" }";
	}

	std::u8string make_nested(int depth)
	{
		std::u8string out;
		out.reserve(64 * 1024);
		out += u8"root: ";

		std::size_t id = 0;
		append_node(out, depth, id);
		out += u8";\n";
		return out;
	}

	std::u8string make_strings(std::size_t count)
	{
		std::u8string out;
		out.reserve(count * 64);

		for (std::size_t i = 0; i < count; ++i)
		{
			out += u8"s";
			append_uint(out, i);
			// The value exercises quote, backslash and newline escapes.
			out += u8": \"line\\nwith \\\"quote\\\" and \\\\ slash\";\n";
		}

		return out;
	}

	std::u8string make_comments(std::size_t count)
	{
		std::u8string out;
		out.reserve(count * 48);
		out += u8"// generated comment corpus\n";

		for (std::size_t i = 0; i < count; ++i)
		{
			out += u8"/* block comment */ k";
			append_uint(out, i);
			out += u8": ";
			append_uint(out, i);
			out += u8";\n";

			if (i % 10 == 0)
			{
				out += u8"// line comment\n";
			}
		}

		return out;
	}

	// The generated corpus is ASCII-only, so widening one code unit at a time
	// is enough to compare the UTF-8/UTF-16/UTF-32 input paths.
	std::u16string to_utf16(const std::u8string& input)
	{
		std::u16string out;
		out.reserve(input.size());
		for (char8_t c : input)
		{
			out.push_back(static_cast<char16_t>(c));
		}
		return out;
	}

	std::u32string to_utf32(const std::u8string& input)
	{
		std::u32string out;
		out.reserve(input.size());
		for (char8_t c : input)
		{
			out.push_back(static_cast<char32_t>(c));
		}
		return out;
	}

	struct measurement
	{
		std::size_t iterations;
		double seconds;
	};

	template <class F>
	measurement measure(F&& operation, double target_seconds, std::size_t max_iterations = 2'000'000)
	{
		for (int i = 0; i < 3; ++i)
		{
			operation();
		}

		const auto calibration_start = clock_type::now();
		operation();
		const auto calibration_end = clock_type::now();
		const double single = std::chrono::duration<double>(calibration_end - calibration_start).count();

		std::size_t iterations = 1;
		if (single > 0.0)
		{
			const double wanted = target_seconds / single;
			if (wanted > 1.0)
			{
				iterations = static_cast<std::size_t>(
					std::min(wanted, static_cast<double>(max_iterations)));
			}
		}
		else
		{
			iterations = 1000;
		}

		const auto start = clock_type::now();
		for (std::size_t i = 0; i < iterations; ++i)
		{
			operation();
		}
		const auto end = clock_type::now();

		return { iterations, std::chrono::duration<double>(end - start).count() };
	}

	void print_header()
	{
		std::cout << std::left << std::setw(30) << "case"
			<< std::right << std::setw(12) << "bytes"
			<< std::setw(10) << "iters"
			<< std::setw(12) << "total_ms"
			<< std::setw(12) << "ns/op"
			<< std::setw(12) << "MiB/s"
			<< std::setw(14) << "checksum" << '\n';
	}

	void print_row(const std::string& name, std::size_t bytes,
		const measurement& result, std::uint64_t checksum_delta)
	{
		const double total_ms = result.seconds * 1000.0;
		const double ns_per_op = result.seconds * 1e9 / static_cast<double>(result.iterations);
		const double mib_per_s = (static_cast<double>(bytes) * static_cast<double>(result.iterations)) /
			(result.seconds * 1024.0 * 1024.0);

		std::cout << std::left << std::setw(30) << name
			<< std::right << std::setw(12) << bytes
			<< std::setw(10) << result.iterations
			<< std::setw(12) << std::fixed << std::setprecision(2) << total_ms
			<< std::setw(12) << std::setprecision(1) << ns_per_op
			<< std::setw(12) << std::setprecision(2) << mib_per_s
			<< std::setw(14) << checksum_delta << '\n';
	}

	template <class char_t, class tag_t>
	void run_parse_case(const std::string& name, const std::basic_string<char_t>& input, tag_t tag)
	{
		const auto view = std::basic_string_view<char_t>{ input.data(), input.size() };

		auto operation = [&view, tag, &name]
		{
			const auto result = pdn::parse(view, tag);
			if (!result)
			{
				std::cerr << name << ": " << result.what() << '\n';
				std::exit(1);
			}
			checksum = checksum + static_cast<std::uint64_t>(result->as_object().size()) + 1;
		};

		operation();
		const std::uint64_t before = checksum;
		const auto result = measure(operation, 0.30);
		print_row(name, input.size() * sizeof(char_t), result, checksum - before);
	}

	void run_serialize_case(const std::string& name, const std::u8string& input)
	{
		const auto parsed = pdn::parse(
			std::u8string_view{ input.data(), input.size() }, pdn::utf8_tag);
		if (!parsed)
		{
			std::cerr << name << ": " << parsed.what() << '\n';
			std::exit(1);
		}

		const auto& object = parsed->as_object();
		const auto serializer = pdn::make_u8serializer();
		const auto sample = serializer.serialize(object);

		auto operation = [&object, &serializer]
		{
			const auto serialized = serializer.serialize(object);
			checksum = checksum + static_cast<std::uint64_t>(serialized.size()) + 1;
		};

		operation();
		const std::uint64_t before = checksum;
		const auto result = measure(operation, 0.30);
		print_row(name, sample.size(), result, checksum - before);
	}

	void run_roundtrip_case(const std::string& name, const std::u8string& input)
	{
		const auto view = std::u8string_view{ input.data(), input.size() };
		const auto sample = pdn::parse(view, pdn::utf8_tag);
		if (!sample)
		{
			std::cerr << name << ": " << sample.what() << '\n';
			std::exit(1);
		}

		const auto serializer = pdn::make_u8serializer();

		auto operation = [&view, &serializer, &name]
		{
			const auto first = pdn::parse(view, pdn::utf8_tag);
			if (!first)
			{
				std::cerr << name << ": " << first.what() << '\n';
				std::exit(1);
			}

			const auto serialized = serializer.serialize(first->as_object());
			const auto second = pdn::parse(
				std::u8string_view{ serialized.data(), serialized.size() }, pdn::utf8_tag);
			if (!second)
			{
				std::cerr << name << ": " << second.what() << '\n';
				std::exit(1);
			}

			checksum = checksum + static_cast<std::uint64_t>(second->as_object().size()) + 1;
		};

		operation();
		const std::uint64_t before = checksum;
		const auto result = measure(operation, 0.30);
		print_row(name, input.size(), result, checksum - before);
	}

	std::filesystem::path write_temp_corpus(const std::u8string& input)
	{
		const auto stamp = clock_type::now().time_since_epoch().count();
		const auto name = "pdn_benchmark_" + std::to_string(stamp) + ".spdn";

		// Some sandboxes make the system temp directory read-only for child
		// processes; fall back to the build directory before giving up.
		const std::filesystem::path candidates[] =
		{
			std::filesystem::temp_directory_path() / name,
			std::filesystem::current_path() / name
		};

		for (const auto& path : candidates)
		{
			std::ofstream out(path, std::ios::binary | std::ios::trunc);
			if (!out)
			{
				continue;
			}

			out.write(reinterpret_cast<const char*>(input.data()),
				static_cast<std::streamsize>(input.size()));
			out.close();
			if (out)
			{
				return path;
			}
		}

		std::cerr << "skipping file case: cannot create a temporary corpus\n";
		return {};
	}

	void run_file_case(const std::string& name, const std::filesystem::path& path,
		std::size_t bytes)
	{
		auto operation = [&path, &name]
		{
			const auto result = pdn::parse(path, pdn::utf8_tag);
			if (!result)
			{
				std::cerr << name << ": " << result.what() << '\n';
				std::exit(1);
			}
			checksum = checksum + static_cast<std::uint64_t>(result->as_object().size()) + 1;
		};

		operation();
		const std::uint64_t before = checksum;
		const auto result = measure(operation, 0.30);
		print_row(name, bytes, result, checksum - before);
	}

	void print_usage(const char* program)
	{
		std::cout << "usage: " << program << " [--no-file] [--help]\n";
	}
}

int main(int argc, char** argv)
{
	bool with_file = true;
	for (int i = 1; i < argc; ++i)
	{
		const std::string_view arg = argv[i];
		if (arg == "--no-file")
		{
			with_file = false;
		}
		else if (arg == "--help")
		{
			print_usage(argv[0]);
			return 0;
		}
		else
		{
			std::cerr << "unknown option: " << arg << '\n';
			print_usage(argv[0]);
			return 2;
		}
	}

	std::cout << "pdn benchmark\n";
#ifdef PDN_ENABLE_ORDERED_MAP
	std::cout << "object map: ordered\n";
#else
	std::cout << "object map: unordered\n";
#endif
#if defined(_MSC_VER)
	std::cout << "compiler: MSVC " << _MSC_VER << '\n';
#elif defined(__GNUC__)
	std::cout << "compiler: GCC " << __GNUC__ << '.' << __GNUC_MINOR__ << '\n';
#else
	std::cout << "compiler: unknown\n";
#endif
	std::cout << "parse target: 0.30 s per case\n\n";

	print_header();

	const std::u8string empty;
	run_parse_case("parse/empty", empty, pdn::utf8_tag);

	const auto scalars_1k = make_scalars(1'000, false);
	run_parse_case("parse/scalars-1k", scalars_1k, pdn::utf8_tag);

	const auto scalars_10k = make_scalars(10'000, false);
	run_parse_case("parse/scalars-10k", scalars_10k, pdn::utf8_tag);
	run_parse_case("parse/scalars-10k-utf16", to_utf16(scalars_10k), pdn::utf16_tag);
	run_parse_case("parse/scalars-10k-utf32", to_utf32(scalars_10k), pdn::utf32_tag);

	const auto list_100k = make_list(100'000);
	run_parse_case("parse/list-100k", list_100k, pdn::utf8_tag);

	const auto nested = make_nested(6);
	run_parse_case("parse/nested", nested, pdn::utf8_tag);

	const auto strings = make_strings(5'000);
	run_parse_case("parse/strings-5k", strings, pdn::utf8_tag);

	const auto comments = make_comments(5'000);
	run_parse_case("parse/comments-5k", comments, pdn::utf8_tag);

	const auto typed = make_scalars(5'000, true);
	run_parse_case("parse/typed-5k", typed, pdn::utf8_tag);

	if (with_file)
	{
		const auto path = write_temp_corpus(scalars_10k);
		if (!path.empty())
		{
			run_file_case("parse/file-scalars-10k", path, scalars_10k.size());

			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
	}

	run_serialize_case("serialize/scalars-10k", scalars_10k);
	run_serialize_case("serialize/nested", nested);
	run_serialize_case("serialize/strings-5k", strings);

	run_roundtrip_case("roundtrip/scalars-10k", scalars_10k);

	std::cout << "\nchecksum: " << checksum << '\n';
	return 0;
}
