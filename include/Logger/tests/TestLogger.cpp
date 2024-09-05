#include "Logger.h"

char fdbiv(int adbvs)
{
	Logger::LogDebug("DBIBC");
	Logger::LogProd("vubaporof");
	return adbvs;
}

int main()
{
	const Config::Logging config;
	Logger::Setup(config);
	Logger::LogDebug("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	Logger::LogError("vjabdivyayi ");
	fdbiv(23);
	Logger::Reset();

	return 0;
}
