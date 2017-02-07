#pragma once
#include <uuid/uuid.h>
#include <string>

namespace UUID
{
	static std::string generate()
	{
		uuid_t out;
		std::string uuid(37, '\0');
		uuid_generate(out);
		uuid_unparse(out, &uuid[0]);
		return uuid;		
	}
};
