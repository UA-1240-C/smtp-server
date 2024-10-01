#ifndef MIME_PARSER_H
#define MIME_PARSER_H

#include <string>
#include <vector>
#include <tuple>
#include <functional>

#include <vmime/vmime.hpp>

namespace ISXMIMEParser
{
/**
 * @class MIMEParser
 * @brief Parse incoming MIME message to retrieve needed information  
*/
class MIMEParser
{
public:

    /**
     * @brief Constructs a MIMEParser object.
     * 
     * Following constructor initialize messageParser with data for future parsing.
     * 
     * @param data Reference to a string containing data received from the client.
     */
    MIMEParser(const std::string& data);

    /**
     * @brief Destructs the MIMEParser objec.
     */
    ~MIMEParser();

    /**
     * @brief Checking if incoming data structured as MIME or not.
     *
     * @param data Reference to a string containing data received from the client. 
     */
    static bool IsMIME(const std::string& data);

    /**
     * @brief Retrieves sender's email address and name, if mentioned, from MIME message. 
     *
     * @return std::tuple<std::string, std::string> Email and name of sender.
     *  
     * @throws std::runtime_error If an error occurs while processing the data.
     */
    std::tuple<std::string, std::string> RetrieveSender();

    /**
     * @brief Retrieves recipients's email address and name, if mentioned, from MIME message. 
     *
     * @return std::vector<std::tuple<std::string, std::string>> List of emails and names of recipients.
     *  
     * @throws std::runtime_error If an error occurs while processing the data.
     */
    std::vector<std::tuple<std::string, std::string>> RetrieveRecipients();

    /**
     * @brief Retrieves CCs's email address and name, if mentioned, from MIME message. 
     *
     * @return std::vector<std::tuple<std::string, std::string>> List of emails and names of CCs.
     *  
     * @throws std::runtime_error If an error occurs while processing the data.
     */
    std::vector<std::tuple<std::string, std::string>> RetrieveCCs();

    /**
     * @brief Retrieves BCCs's email address and name, if mentioned, from MIME message. 
     *
     * @return std::vector<std::tuple<std::string, std::string>> List of emails and names of BCCs.
     *  
     * @throws std::runtime_error If an error occurs while processing the data.
     */
    std::vector<std::tuple<std::string, std::string>> RetrieveBCCs();

    /**
     * @brief Retrieves subject of email from MIME message. 
     * 
     * @return std::string The subject of email.
     * 
     * @throws std::runtime_error If an error occurs while processing the data.
     */
    std::string RetrieveSubject();

    /**
     * @brief Retrieves body of email from MIME message. 
     * 
     * @return std::string The body of email.
     * 
     * @throws std::runtime_error If an error occurs while processing the data.
     */
    std::string RetrieveBody();

    /**
     * @brief Retrieves attachments pinned to email from MIME message.
     * 
     * @return std::vector<std::string> List of attachments.
     * 
     * @throws std::runtime_error If an error occurs while processing the data.
     * 
     * @details The function return list of strings which need to be decoded and parsed before recreating original file.
     *          Structure of string following: <Content-Type>|<filename.filetype>|<data stored in attachment>.
     *          Strings encoded with the Base64 algorithm.
     */
    std::vector<std::string> RetrieveAttachments();


private:

    /**
     * @brief Generalized function to retrieve destination address from MIME message. 
     * 
     * @param receivers Callback function to be used to retrieve destination addresses.
     * 
     * @return std::vector<std::tuple<std::string, std::string>> List of addresses and names of receivers of incoming message.
     * 
     * @throws std::runtime_error If an error occurs while processing the data.
     */
    std::vector<std::tuple<std::string, std::string>> RetrieveDestination(std::function<const vmime::addressList()> receivers);

    vmime::shared_ptr<vmime::messageParser> parser;     ///< Pointer to the messageParser to parse incoming MIME message.
};
}



#endif