#include <ares.h>
#include <boost/asio.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <arpa/nameser.h>

#include "MXResolver.h"

std::vector<MXRecord> MXResolver::ResolveMX(const std::string& domain)
{
	ares_channel channel;
	int status = ares_init(&channel);
	if (status != ARES_SUCCESS) {
		std::cerr << "Failed to initialize c-ares: " << ares_strerror(status) << std::endl;
		return {};
	}

	std::vector<MXRecord> mx_records;
	ares_query(channel, domain.c_str(), ns_c_in, ns_t_mx,
			[](void* arg, int status, int timeouts, unsigned char* abuf, int alen) {
			if (status != ARES_SUCCESS) {
					std::cerr << "DNS lookup failed: " << ares_strerror(status) << std::endl;
					return;
			}

			auto* mx_records = static_cast<std::vector<MXRecord>*>(arg);
			ares_mx_reply* mx_result = nullptr;
			if (ares_parse_mx_reply(abuf, alen, &mx_result) == ARES_SUCCESS) {
					ares_mx_reply* mx_reply = mx_result;
					while (mx_reply) {
							mx_records->emplace_back(MXRecord{mx_reply->host, mx_reply->priority});
							mx_reply = mx_reply->next;
					}
					ares_free_data(mx_result);
			} else {
					std::cerr << "Failed to parse MX response" << std::endl;
			}
	}, &mx_records);

	fd_set read_fds, write_fds;
	timeval tv;
	int nfds;

	while (true) {
		FD_ZERO(&read_fds);
		FD_ZERO(&write_fds);
		nfds = ares_fds(channel, &read_fds, &write_fds);
		if (nfds == 0) break;

		timeval* tvp = ares_timeout(channel, nullptr, &tv);
		select(nfds, &read_fds, &write_fds, nullptr, tvp);
		ares_process(channel, &read_fds, &write_fds);
	}

	ares_destroy(channel);
	return mx_records;
}

std::string MXResolver::ExtractDomain(const std::string& email)
{
	// Find the position of the '@' character in the email address
	std::size_t at_pos = email.find('@');

	// If '@' is found, return the substring after it, otherwise return an empty string
	if (at_pos != std::string::npos) {
		return email.substr(at_pos + 1);
	}
	return "";
}
