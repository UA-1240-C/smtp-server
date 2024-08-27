#ifndef STANDARTSMTPRESPONSES_H
#define STANDARTSMTPRESPONSES_H

#include <unordered_map>
#include <string>
#include <stdexcept>

/**
 * @enum SmtpResponseCode
 * @brief Represents SMTP response codes used in the protocol.
 *
 * This enum class defines various SMTP response codes that are used to indicate the result
 * of the command execution in the SMTP protocol. Each code is associated with a specific
 * meaning as defined by the SMTP protocol specification.
 */
enum class SmtpResponseCode {
	// Informational responses
	SERVER_CONNECTION_ERROR = 101,
	SYSTEM_STATUS = 211,
	HELP_MESSAGE = 214,

	// Success responses
	SERVER_READY = 220,
	CLOSING_TRANSMISSION_CHANNEL = 221,
	AUTH_SUCCESSFUL = 235,
	REGISTER_SUCCESSFUL = 236,		///< Custom SMTP response code
	OK = 250,
	USER_NOT_LOCAL = 251,
	CANNOT_VERIFY_USER= 252,
	AUTH_MECHANISM_ACCEPTED = 334,
	START_MAIL_INPUT = 354,

	// Transient Negative Completion responses
	USER_ALREADY_EXISTS = 411,		///< Custom SMTP response code
	SERVER_UNAVAILABLE = 421,
	MAILBOX_EXCEEDED_STORAGE = 422,
	FILE_OVERLOAD = 431,
	NO_RESPONSE_FROM_SERVER = 441,
	CONNECTION_DROPPED = 442,
	INTERNAL_LOOP = 446,
	MAILBOX_UNAVAILABLE = 450,
	LOCAL_ERROR = 451,
	INSUFFICIENT_STORAGE = 452,
	TLS_TEMPORARILY_UNAVAILABLE = 454,
	PARAMETERS_CANNOT_BE_ACCOMMODATED = 455,
	REQUIRED_FIELDS_MISSING = 456,
	SPAM_FILTER_ERROR = 471,

	// Permanent Negative Completion responses
	SYNTAX_ERROR = 500,
	SYNTAX_ERROR_IN_PARAMETERS = 501,
	COMMAND_NOT_IMPLEMENTED = 502,
	BAD_SEQUENCE = 503,
	COMMAND_PARAMETER_NOT_IMPLEMENTED = 504,
	INVALID_EMAIL_ADDRESS = 510,
	DNS_ERROR = 512,
	MAILING_SIZE_LIMIT_EXCEEDED = 523,
	AUTHENTICATION_PROBLEM = 530,
	AUTHENTICATION_FAILED = 535,
	REGISTRATION_FAILED = 536,
	ENCRYPTION_REQUIRED = 538,
	MESSAGE_REJECTED_BY_SPAM_FILTER = 541,
	MAILBOX_UNAVAILABLE_550 = 550,
	USER_NOT_LOCAL_551 = 551,
	MAILBOX_FULL = 552,
	INCORRECT_MAIL_ADDRESS = 553,
	TRANSACTION_FAILED = 554,
	PARAMETERS_NOT_RECOGNIZED = 555
};

/**
 * @brief Converts an SMTP response code to its corresponding string representation.
 *
 * This function takes an `SmtpResponseCode` enum value and returns its string
 * representation. It is useful for logging and displaying human-readable
 * information about the response code.
 *
 * @param code The SMTP response code to convert.
 * @return A string representation of the SMTP response code.
 */
std::string ToString(SmtpResponseCode code);

#endif //STANDARTSMTPRESPONSES_H
