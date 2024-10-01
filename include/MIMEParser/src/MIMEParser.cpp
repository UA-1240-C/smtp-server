#include <exception>

#include "MIMEParser.h"
#include "Logger.h"
#include "Base64.h"

namespace ISXMIMEParser
{

MIMEParser::MIMEParser(const std::string& data)
{
    Logger::LogDebug("Entering MIMEParser constructor");
    Logger::LogTrace("Constructor MIMEParser parameter: const std::string reference");

    parser = vmime::make_shared<vmime::messageParser>(data);

    Logger::LogDebug("Exiting MIMEParser::MIMEParser");
}

MIMEParser::~MIMEParser()
{
    Logger::LogDebug("Entering MIMEParser destructor");
    Logger::LogDebug("Exiting MIMEParser destructor");
}

bool MIMEParser::IsMIME(const std::string& data)
{
    Logger::LogDebug("Entering MIMEParser::IsMIME");
    Logger::LogTrace("MIMEParser::IsMIME parameter: std::string reference");

    vmime::shared_ptr<vmime::message> mime_msg = vmime::make_shared<vmime::message>();
    mime_msg->parse(data);

    vmime::shared_ptr<vmime::header> header = mime_msg->getHeader();
    
    Logger::LogDebug("Exiting MIMEParser::IsMIME");
    
    return header->hasField(vmime::fields::MIME_VERSION) || header->hasField(vmime::fields::CONTENT_TYPE);
}

std::tuple<std::string, std::string> MIMEParser::RetrieveSender()
{
    Logger::LogDebug("Entering MIMEParser::RetrieveSender");
    Logger::LogTrace("MIMEParser::RetrieveSender parameter: void");

    Logger::LogDebug("Exiting MIMEParser::RetrieveSender");
    
    try
    {
        return {parser->getExpeditor().getEmail().toString(), parser->getExpeditor().getName().generate()};
    }
    catch(vmime::exception& e)
    {
        throw std::runtime_error(e.what());
    }
}

std::vector<std::tuple<std::string, std::string>> MIMEParser::RetrieveRecipients()
{
    Logger::LogDebug("Entering MIMEParser::RetrieveRecipients");
    Logger::LogTrace("MIMEParser::RetrieveRecipients parameter: void");

    Logger::LogDebug("Exiting MIMEParser::RetrieveRecipients");
    
    try
    {
        return this->RetrieveDestination(std::bind(&vmime::messageParser::getRecipients, parser));
    }
    catch(vmime::exception& e)
    {
        throw std::runtime_error(e.what());
    }
}  

std::vector<std::tuple<std::string, std::string>> MIMEParser::RetrieveCCs()
{
    Logger::LogDebug("Entering MIMEParser::RetrieveCCs");
    Logger::LogTrace("MIMEParser::RetrieveCCs parameter: void");

    Logger::LogDebug("Exiting MIMEParser::RetrieveCCs");
    
    try
    {
        return this->RetrieveDestination(std::bind(&vmime::messageParser::getCopyRecipients, parser));
    }
    catch(vmime::exception& e)
    {
        throw std::runtime_error(e.what());
    }
}


std::vector<std::tuple<std::string, std::string>> MIMEParser::RetrieveBCCs()
{
    Logger::LogDebug("Entering MIMEParser::RetrieveBCCs");
    Logger::LogTrace("MIMEParser::RetrieveBCCs parameter: void");

    Logger::LogDebug("Exiting MIMEParser::RetrieveBCCs");
    
    try
    {
        return this->RetrieveDestination(std::bind(&vmime::messageParser::getBlindCopyRecipients, parser));
    }
    catch(vmime::exception& e)
    {
        throw std::runtime_error(e.what());
    }
}

std::string MIMEParser::RetrieveSubject()
{
    Logger::LogDebug("Entering MIMEParser::RetrieveSubject");
    Logger::LogTrace("MIMEParser::RetrieveSubject parameter: void");

    Logger::LogDebug("Exiting MIMEParser::RetrieveSubject");
   
    try
    {
        return parser->getSubject().generate();
    }
    catch(vmime::exception& e)
    {
        throw std::runtime_error(e.what());
    }    
}

std::string MIMEParser::RetrieveBody()
{
    Logger::LogDebug("Entering MIMEParser::RetrieveBody");
    Logger::LogTrace("MIMEParser::RetrieveBody parameter: void");

    std::string body;
    
    try
    {
        vmime::utility::outputStreamStringAdapter out_str(body);
        for(auto&& text: parser->getTextPartList())
        {
            text->getText()->extractRaw(out_str);
        }

        Logger::LogDebug("Exiting MIMEParser::RetrieveBody");
    }
    catch(vmime::exception& e)
    {
        throw std::runtime_error(e.what());
    }  
    
    return body;
}

std::vector<std::string> MIMEParser::RetrieveAttachments()
{
    Logger::LogDebug("Entering MIMEParser::RetrieveAttachments");
    Logger::LogTrace("MIMEParser::RetrieveAttachments parameter: void");

    std::vector<std::string> attachments;

    try
    {
        for(auto&& att: parser->getAttachmentList())
        {
            std::string att_string = att->getType().getType()+"/"+att->getType().getSubType()+"|"+att->getName().getBuffer()+"|";
            std::string att_data;
            vmime::utility::outputStreamStringAdapter out_str(att_data);
            att->getData()->extractRaw(out_str);
            att_string += att_data;
            attachments.push_back(ISXBase64::Base64Encode(att_string));
        }
    }
    catch(vmime::exception& e)
    {
        throw std::runtime_error(e.what());
    }

    Logger::LogDebug("Exiting MIMEParser::RetrieveAttachments");
    
    return attachments;
}

std::vector<std::tuple<std::string, std::string>> MIMEParser::RetrieveDestination(std::function<const vmime::addressList()> receivers)
{
    Logger::LogDebug("Entering MIMEParser::RetrieveDestination");
    Logger::LogTrace("MIMEParser::RetrieveDestination parameter: std::function<const vmime::addressList()>");

    std::vector<std::tuple<std::string, std::string>> recipients;

    try
    {
        for(auto&& recipient: receivers().toMailboxList()->getMailboxList())
        {
            recipients.push_back({recipient->getEmail().generate(), recipient->getName().generate()});
        }
    }
    catch(vmime::exception& e)
    {
        throw std::runtime_error(e.what());
    }

    Logger::LogDebug("Exiting MIMEParser::RetrieveDestination");
    
    return recipients;
}

}