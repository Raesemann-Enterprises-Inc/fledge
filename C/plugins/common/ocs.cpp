/*
 * Fledge OSIsoft OCS integration.
 * Implements the integration for the specific functionalities exposed by OCS
 *
 * Copyright (c) 2020 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Stefano Simonelli
 */

#include <string>
#include <vector>
#include <utility>

#include <ocs.h>
#include <string_utils.h>
#include <logger.h>
#include <simple_https.h>
#include <rapidjson/document.h>
#include "rapidjson/error/en.h"

using namespace std;
using namespace rapidjson;

OCS::OCS()
{
}

// Destructor
OCS::~OCS()
{
}

/**
 * Extracts the OCS token from the JSON returned by the OCS API
 *
 * @param response  JSON message generated by the OCS API containing the OCS token
 * @return          The OCS token to be used for authentication in API calls
 *
 */
std::string OCS::extractToken(const string& response)
{
	Document JSon;
	string token;

	ParseResult ok = JSon.Parse(response.c_str());
	if (!ok)
	{
		Logger::getLogger()->error("OCS token extract, invalid json - HTTP response :%s:", response.c_str());
	}
	else
	{
		if (JSon.HasMember("access_token"))
		{
			token = JSon["access_token"].GetString();
		}
	}

	return(token);
}

/**
 * Calls the OCS API to retrieve the authentication token related to the the clientId and clientSecret
 *
 * @param clientId      Client Id code assigned by OCS using its GUI to the specific connection
 * @param clientSecret  Client Secret code assigned by OCS using its gui to the specific connection
 * @return              The OCS token to be used for authentication in API calls
 *
 */
std::string OCS::retrieveToken(const string& clientId, const string& clientSecret)
{
	string token;
	string response;
	string payload;

	HttpSender *endPoint;
	vector<pair<string, string>> header;
	int httpCode;

	endPoint = new SimpleHttps(OCS_HOST,
							   TIMEOUT_CONNECT,
							   TIMEOUT_REQUEST,
							   RETRY_SLEEP_TIME,
							   MAX_RETRY);

	header.push_back( std::make_pair("Content-Type", "application/x-www-form-urlencoded"));
	header.push_back( std::make_pair("Accept", " text/plain"));

	payload =  PAYLOAD_RETRIEVE_TOKEN;

	StringReplace(payload, "CLIENT_ID_PLACEHOLDER",        urlEncode(clientId));
	StringReplace(payload, "CLIENT_SECRET_ID_PLACEHOLDER", urlEncode(clientSecret));

	// Anonymous auth
	string authMethod = "a";
	endPoint->setAuthMethod (authMethod);

	try
	{
		httpCode = endPoint->sendRequest("POST",
										 URL_RETRIEVE_TOKEN,
										 header,
										 payload);

		response = endPoint->getHTTPResponse();

		if (httpCode >= 200 && httpCode <= 399)
		{
			token = extractToken(response);
			Logger::getLogger()->debug("OCS authentication token :%s:" ,token.c_str() );
		}
		else
		{
			Logger::getLogger()->warn("Error in retrieving the authentication token from OCS - http :%d: :%s: ", httpCode, response.c_str());
		}

	}
	catch (exception &ex)
	{
		Logger::getLogger()->warn("Error in retrieving the authentication token from OCS - error :%s: ", ex.what());
	}

	delete endPoint;

	return token;
}
