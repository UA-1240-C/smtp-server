#include "StandartSmtpResponses.h"

std::string ToString(SmtpResponseCode code) {
	static const std::unordered_map<SmtpResponseCode, std::string> smtpResponses = {
        // Informational responses
        { SmtpResponseCode::SERVER_CONNECTION_ERROR, "101 Server connection error\r\n" },
        { SmtpResponseCode::SYSTEM_STATUS, "211 System status\r\n" },
        { SmtpResponseCode::HELP_MESSAGE, "214 Help message\r\n" },

        // Success responses
        { SmtpResponseCode::SERVER_READY, "220 Client was successfully connected.\r\n" },
        { SmtpResponseCode::CLOSING_TRANSMISSION_CHANNEL, "221 The server closes the transmission channel\r\n" },
        { SmtpResponseCode::AUTH_SUCCESSFUL, "235 Authentication successful\r\n" },
				{ SmtpResponseCode::REGISTER_SUCCESSFUL, "236 Registration successful\r\n" },
        { SmtpResponseCode::OK, "250 OK\r\n" },
        { SmtpResponseCode::USER_NOT_LOCAL, "251 User not local, will forward\r\n" },
        { SmtpResponseCode::CANNOT_VERIFY_USER, "252 Cannot verify user\r\n" },
        { SmtpResponseCode::AUTH_MECHANISM_ACCEPTED, "334 Authentication mechanism accepted\r\n" },
        { SmtpResponseCode::START_MAIL_INPUT, "354 Start mail input(End data with <CR><LF>.<CR><LF>)\r\n" },

        // Transient Negative Completion responses
				{ SmtpResponseCode::USER_ALREADY_EXISTS, "411 User already exists\r\n" },
        { SmtpResponseCode::SERVER_UNAVAILABLE, "421 Server unavailable\r\n" },
        { SmtpResponseCode::MAILBOX_EXCEEDED_STORAGE, "422 Mailbox exceeded storage limit\r\n" },
        { SmtpResponseCode::FILE_OVERLOAD, "431 File overload\r\n" },
        { SmtpResponseCode::NO_RESPONSE_FROM_SERVER, "441 No response from server\r\n" },
        { SmtpResponseCode::CONNECTION_DROPPED, "442 Connection dropped\r\n" },
        { SmtpResponseCode::INTERNAL_LOOP, "446 Internal loop occurred\r\n" },
        { SmtpResponseCode::MAILBOX_UNAVAILABLE, "450 Mailbox unavailable\r\n" },
        { SmtpResponseCode::LOCAL_ERROR, "451 Local error\r\n" },
        { SmtpResponseCode::INSUFFICIENT_STORAGE, "452 Insufficient system storage\r\n" },
        { SmtpResponseCode::TLS_TEMPORARILY_UNAVAILABLE, "454 TLS temporarily unavailable\r\n" },
        { SmtpResponseCode::PARAMETERS_CANNOT_BE_ACCOMMODATED, "455 Parameters cannot be accommodated\r\n" },
				{ SmtpResponseCode::REQUIRED_FIELDS_MISSING, "456 Required fields missing\r\n" },
        { SmtpResponseCode::SPAM_FILTER_ERROR, "471 Spam filter error\r\n" },

        // Permanent Negative Completion responses
        { SmtpResponseCode::SYNTAX_ERROR, "500 Syntax error\r\n" },
        { SmtpResponseCode::SYNTAX_ERROR_IN_PARAMETERS, "501 Syntax error in parameters\r\n" },
        { SmtpResponseCode::COMMAND_NOT_IMPLEMENTED, "502 Command not implemented\r\n" },
        { SmtpResponseCode::BAD_SEQUENCE, "503 Bad sequence of commands\r\n" },
        { SmtpResponseCode::COMMAND_PARAMETER_NOT_IMPLEMENTED, "504 Command parameter not implemented\r\n" },
        { SmtpResponseCode::INVALID_EMAIL_ADDRESS, "510 Invalid email address\r\n" },
        { SmtpResponseCode::DNS_ERROR, "512 DNS error\r\n" },
        { SmtpResponseCode::MAILING_SIZE_LIMIT_EXCEEDED, "523 Mailing size limit exceeded\r\n" },
        { SmtpResponseCode::AUTHENTICATION_PROBLEM, "530 Authentication problem\r\n" },
        { SmtpResponseCode::AUTHENTICATION_FAILED, "535 Authentication failed\r\n" },
				{ SmtpResponseCode::REGISTRATION_FAILED, "536 Registration failed\r\n" },
        { SmtpResponseCode::ENCRYPTION_REQUIRED, "538 Encryption required\r\n" },
        { SmtpResponseCode::MESSAGE_REJECTED_BY_SPAM_FILTER, "541 Message rejected by spam filter\r\n" },
        { SmtpResponseCode::MAILBOX_UNAVAILABLE_550, "550 Mailbox unavailable\r\n" },
        { SmtpResponseCode::USER_NOT_LOCAL_551, "551 User not local\r\n" },
        { SmtpResponseCode::MAILBOX_FULL, "552 Mailbox full\r\n" },
        { SmtpResponseCode::INCORRECT_MAIL_ADDRESS, "553 Incorrect mail address\r\n" },
        { SmtpResponseCode::TRANSACTION_FAILED, "554 Transaction failed\r\n" },
        { SmtpResponseCode::PARAMETERS_NOT_RECOGNIZED, "555 Parameters not recognized\r\n" }
    };

    auto it = smtpResponses.find(code);
    if (it != smtpResponses.end()) {
        return it->second;
    }
	throw std::invalid_argument("Invalid SMTP response code");
}
