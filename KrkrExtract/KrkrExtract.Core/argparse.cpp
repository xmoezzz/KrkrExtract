#include "argparse.h"

namespace argparse
{
	std::ostream &operator<<(std::ostream &os, const ArgumentParser::Result &r) {
		os << r.what();
		return os;
	}
}

