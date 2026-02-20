#include "argparser.hpp"
#include "GCClient.hpp"
#include "server.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
	struct LaunchServerConfig
	{
		int count = 1;
		std::string serverAddress;
		int players = SERVER_NUM_CLIENTS;
		int maxPlayers = SERVER_MAX_CLIENTS;
		int bots = SERVER_NUM_FAKE_CLIENTS;
		std::string hostname = SERVER_NAME;
		std::string map = SERVER_MAP;
		std::string region = SERVER_REGION;
		bool secure = SERVER_VAC_STATES;
		std::string tags = SERVER_TAG;
		std::string description = SERVER_DESCRIPTION;
	};

	std::string Trim(const std::string& value)
	{
		auto begin = value.find_first_not_of(" \t\r\n");
		if (begin == std::string::npos)
			return "";

		auto end = value.find_last_not_of(" \t\r\n");
		return value.substr(begin, end - begin + 1);
	}

	std::string EscapeArg(const std::string& value)
	{
		std::string escaped;
		escaped.reserve(value.size() + 2);
		escaped += '"';

		for (char ch : value)
		{
			if (ch == '"')
				escaped += '\\';
			escaped += ch;
		}

		escaped += '"';
		return escaped;
	}

	bool ExtractSectionBody(const std::string& text, const std::string& key, std::string& out)
	{
		auto keyPos = text.find('"' + key + '"');
		if (keyPos == std::string::npos)
			return false;

		auto bracketStart = text.find('[', keyPos);
		if (bracketStart == std::string::npos)
			return false;

		int depth = 0;
		for (size_t i = bracketStart; i < text.size(); ++i)
		{
			if (text[i] == '[')
				++depth;
			else if (text[i] == ']')
			{
				--depth;
				if (depth == 0)
				{
					out = text.substr(bracketStart + 1, i - bracketStart - 1);
					return true;
				}
			}
		}

		return false;
	}

	std::vector<std::string> ExtractObjectBlocks(const std::string& section)
	{
		std::vector<std::string> objects;
		int depth = 0;
		size_t start = 0;

		for (size_t i = 0; i < section.size(); ++i)
		{
			if (section[i] == '{')
			{
				if (depth == 0)
					start = i;
				++depth;
			}
			else if (section[i] == '}')
			{
				--depth;
				if (depth == 0)
					objects.push_back(section.substr(start, i - start + 1));
			}
		}

		return objects;
	}

	bool ExtractString(const std::string& text, const std::string& key, std::string& value)
	{
		std::regex re('"' + key + R"("\s*:\s*"([^"]*)")");
		std::smatch match;
		if (!std::regex_search(text, match, re))
			return false;

		value = match[1].str();
		return true;
	}

	bool ExtractInt(const std::string& text, const std::string& key, int& value)
	{
		std::regex re('"' + key + R"("\s*:\s*(-?\d+))");
		std::smatch match;
		if (!std::regex_search(text, match, re))
			return false;

		value = std::stoi(match[1].str());
		return true;
	}

	bool ExtractBool(const std::string& text, const std::string& key, bool& value)
	{
		std::regex re('"' + key + R"("\s*:\s*(true|false))");
		std::smatch match;
		if (!std::regex_search(text, match, re))
			return false;

		value = match[1].str() == "true";
		return true;
	}

	std::vector<std::string> LoadTokens(const std::string& path)
	{
		std::ifstream file(path);
		if (!file.is_open())
		{
			printf("Warning: can't open %s, servers will use anonymous login when token is missing\n", path.c_str());
			return {};
		}

		std::vector<std::string> tokens;
		for (std::string line; std::getline(file, line);)
		{
			line = Trim(line);
			if (!line.empty())
				tokens.push_back(line);
		}

		return tokens;
	}

	bool RunConfigMode(int argc, char** argv)
	{
		std::string configPath;
		std::string tokenPath = "tokens.txt";
		std::string version = "2.0.0.0";

		for (int i = 1; i < argc; ++i)
		{
			std::string arg = argv[i];
			if (arg == "-config" && i + 1 < argc)
				configPath = argv[++i];
			else if (arg == "-tokens" && i + 1 < argc)
				tokenPath = argv[++i];
			else if (arg == "-version" && i + 1 < argc)
				version = argv[++i];
		}

		if (configPath.empty())
			return false;

		std::ifstream cfg(configPath);
		if (!cfg.is_open())
		{
			printf("Failed to open config file: %s\n", configPath.c_str());
			return true;
		}

		std::stringstream buffer;
		buffer << cfg.rdbuf();
		auto cfgText = buffer.str();

		int startPort = 27015;
		ExtractInt(cfgText, "start_port", startPort);
		ExtractString(cfgText, "version", version);

		std::string serversSection;
		if (!ExtractSectionBody(cfgText, "servers", serversSection))
		{
			printf("Config file has no valid \"servers\" array\n");
			return true;
		}

		auto objects = ExtractObjectBlocks(serversSection);
		if (objects.empty())
		{
			printf("Config file has an empty \"servers\" array\n");
			return true;
		}

		auto tokens = LoadTokens(tokenPath);
		std::vector<std::thread> workers;
		int launchIndex = 0;
		int currentPort = startPort;
		const std::string binaryPath = argv[0];

		for (const auto& objectText : objects)
		{
			LaunchServerConfig config;
			ExtractInt(objectText, "count", config.count);
			ExtractString(objectText, "server_address", config.serverAddress);
			ExtractInt(objectText, "players", config.players);
			ExtractInt(objectText, "max_players", config.maxPlayers);
			ExtractInt(objectText, "bots", config.bots);
			ExtractString(objectText, "hostname", config.hostname);
			ExtractString(objectText, "map", config.map);
			ExtractString(objectText, "region", config.region);
			ExtractBool(objectText, "secure", config.secure);
			ExtractString(objectText, "tags", config.tags);
			ExtractString(objectText, "description", config.description);

			if (config.count < 1)
				config.count = 1;

			for (int i = 0; i < config.count; ++i)
			{
				std::string token;
				if (launchIndex < static_cast<int>(tokens.size()))
					token = tokens[launchIndex];

				std::string command = EscapeArg(binaryPath);
				command += " -port " + std::to_string(currentPort);
				command += " -version " + version;
				command += " -hostname " + EscapeArg(config.hostname);
				command += " -map " + EscapeArg(config.map);
				command += " -region " + EscapeArg(config.region);
				command += " -tags " + EscapeArg(config.tags);
				command += " -description " + EscapeArg(config.description);
				command += " -players " + std::to_string(config.players);
				command += " -maxplayers " + std::to_string(config.maxPlayers);
				command += " -bots " + std::to_string(config.bots);
				if (config.secure)
					command += " -vac";
				if (!config.serverAddress.empty())
					command += " -rdip " + EscapeArg(config.serverAddress);
				if (!token.empty())
					command += " -gslt " + token;

				printf("Starting server on port %d\n", currentPort);
				workers.emplace_back([cmd = std::move(command)]()
				{
					auto result = std::system(cmd.c_str());
					if (result != 0)
						printf("Server process exited with code %d\n", result);
				});

				++launchIndex;
				++currentPort;
			}
		}

		for (auto& worker : workers)
			worker.join();

		return true;
	}
}

int main(int argc, char** argv)
{
	if (RunConfigMode(argc, argv))
		return 0;

	ArgParser parser;

	parser.AddOption("-version", "CS2 server version", OptionAttr::RequiredWithValue, OptionValueType::STRING);
	parser.AddOption("-port", "Server listening port", OptionAttr::RequiredWithValue, OptionValueType::INT16U);
	parser.AddOption("-gslt", "Game server logon token", OptionAttr::OptionalWithValue, OptionValueType::STRING);
	parser.AddOption("-rdip", "Redirect IP address (e.g. 127.0.0.1:27015)", OptionAttr::OptionalWithValue, OptionValueType::STRING);
	parser.AddOption("-vac", "Enable VAC?", OptionAttr::OptionalWithoutValue, OptionValueType::NONE);
	parser.AddOption("-mirror", "Enable mirroring server info from redrecting server?", OptionAttr::OptionalWithoutValue, OptionValueType::NONE);
	parser.AddOption("-hostname", "Server name shown in browser", OptionAttr::OptionalWithValue, OptionValueType::STRING, SERVER_NAME);
	parser.AddOption("-map", "Server map shown in browser", OptionAttr::OptionalWithValue, OptionValueType::STRING, SERVER_MAP);
	parser.AddOption("-region", "Server region code", OptionAttr::OptionalWithValue, OptionValueType::STRING, SERVER_REGION);
	parser.AddOption("-tags", "Server tags shown in browser", OptionAttr::OptionalWithValue, OptionValueType::STRING, SERVER_TAG);
	parser.AddOption("-description", "Server description shown in browser", OptionAttr::OptionalWithValue, OptionValueType::STRING, SERVER_DESCRIPTION);
	parser.AddOption("-players", "Fake online players", OptionAttr::OptionalWithValue, OptionValueType::INT8U, "", SERVER_NUM_CLIENTS);
	parser.AddOption("-maxplayers", "Fake max players", OptionAttr::OptionalWithValue, OptionValueType::INT8U, "", SERVER_MAX_CLIENTS);
	parser.AddOption("-bots", "Fake bot players", OptionAttr::OptionalWithValue, OptionValueType::INT8U, "", SERVER_NUM_FAKE_CLIENTS);

	try
	{
		parser.ParseArgument(argc, argv);
	}
	catch (const std::exception& e)
	{
		printf("%s\n", e.what());
		return -1;
	}

	if (parser.HasOption("-mirror") && !parser.HasOption("-rdip"))
	{
		printf("When -mirror is enabled, you have to provide a redirect server socket by option -rdip\n");
		return -1;
	}

	Server sv(parser);
	sv.InitializeServer();
	sv.RunServer();
	return 0;
}
