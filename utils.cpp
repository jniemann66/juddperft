#include "utils.h"

#include <map>
#include <regex>
#include <sstream>

namespace juddperft {

std::string Utils::memorySizeWithBinaryPrefix(size_t bytes)
{
	int shift = 60;
	size_t n = 0;
	if (bytes) {
		while (shift >= 0) {
			n = (bytes >> shift) & 0x3ff;
			if (n) {
				break;
			}
			shift -= 10;
		}

		static const std::map<int, std::string> m
		{
			{60, "EiB"},
			{50, "PiB"},
			{40, "TiB"},
			{30, "GiB"},
			{20, "MiB"},
			{10, "KiB"},
			{0, "B"}
		};

		std::stringstream ss;
		ss << n << " " << m.at(shift);
		return ss.str();
	}
	return "0 B";
}

size_t Utils::bytes(const std::string& memorySizeWithBinaryPrefix, bool *ok)
{
	size_t n; // retval

	static const std::map<std::string, size_t> p2
	{
		{"K", 1ull << 10},
		{"M", 1ull << 20},
		{"G", 1ull << 30},
		{"T", 1ull << 40},
		{"P", 1ull << 50},
		{"E", 1ull << 60}
	};

	static const std::map<std::string, size_t> p10
	{
		{"K", 1e3},
		{"M", 1e6},
		{"G", 1e9},
		{"T", 1e12},
		{"P", 1e15},
		{"E", 1e18},
	};

	static const std::regex rx{"(\\d+)(?:\\s*(M|G|T|P|E)+(i)?B?)?"};
	std::smatch rxm;
	if (std::regex_search(memorySizeWithBinaryPrefix, rxm, rx)) {
		if (rxm.size() > 0) {
			n = std::max(0ull, std::stoull(rxm[1].str()));
			const std::string u = rxm[2].str();
			bool power_of_2 = false;
			if (rxm.size() > 1 && rxm[2].matched) {
				if (rxm.size() > 2 && rxm[3].matched) {
					const std::string b = rxm[3].str();
					power_of_2 = (b == "i");
				}
			}

			try {
				n = n * (power_of_2 ? p2.at(u) : p10.at(u));
			}
			catch (std::out_of_range& e) {
				// ..
			}

			if (ok) {
				*ok = true;
			}
			return n;
		}
	}

	if (ok) {
		*ok = false;
	}
	return 0ull;
}


} // namespace juddperft
