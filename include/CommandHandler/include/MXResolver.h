#pragma once

#ifndef MXRESOLVER_H
#define MXRESOLVER_H

#include <vector>
#include <string>

struct MXRecord {
	std::string host;
	int priority;
};

class MXResolver {
public:
	std::vector<MXRecord> ResolveMX(const std::string& domain);
	std::string ExtractDomain(const std::string& email);
};

#endif //MXRESOLVER_H
