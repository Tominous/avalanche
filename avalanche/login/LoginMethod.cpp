#include "LoginMethod.h"

#include "generator/IncrementalGenerator.h"
#include "generator/RandomGenerator.h"
#include "../Factory.h"
#include <json/json.h>
#include <iostream>

namespace avalanche {

using GeneratorFactory = Factory<AuthGenerator>;
static const GeneratorFactory generatorFactory = GeneratorFactory::MethodRegistry {
    { IncrementalGenerator::s_Name, []() -> std::unique_ptr<AuthGenerator> { return std::make_unique<IncrementalGenerator>(); } },
    { RandomGenerator::s_Name, []() -> std::unique_ptr<AuthGenerator> { return std::make_unique<RandomGenerator>(); } },
};


LoginMethod::LoginMethod()
    : m_Port(25565)
{

}

void LoginMethod::ParseServer(const std::string& server) {
    m_Host = server;

    auto pos = m_Host.find(':');
    if (pos != std::string::npos) {
        m_Port = atoi(m_Host.substr(pos + 1).c_str());
        m_Host = m_Host.substr(0, pos);
    }
}

bool LoginMethod::ReadJSON(const Json::Value& node) {
    auto&& serverNode = node["server"];
    auto&& methodNode = node["method"];
    auto&& generatorNode = node["generator"];

    if (!serverNode.isString() || !methodNode.isObject())
        return false;

    if (serverNode.isString())
        ParseServer(serverNode.asString());

    if (generatorNode.isObject()) {
        auto&& generatorMethodNode = generatorNode["method"];

        if (generatorMethodNode.isString()) {
            m_Generator = generatorFactory.Create(generatorMethodNode.asString());

            if (m_Generator)
                m_Generator->ReadJSON(generatorNode);
        }
    }

    return ReadMethodJSON(methodNode);
}

bool LoginMethod::ReadOptions(std::unordered_map<std::string, std::string> options) {
    auto usernameIter = options.find("username");
    auto passwordIter = options.find("password");
    auto serverIter = options.find("server");
    
    if (m_Generator == nullptr)
        m_Generator = generatorFactory.Create(IncrementalGenerator::s_Name);

    IncrementalGenerator* gen = dynamic_cast<IncrementalGenerator*>(m_Generator.get());

    if (gen != nullptr) {
        std::string username = gen->GetBaseName();
        std::string password = gen->GetPassword();

        if (usernameIter != options.end())
            username = usernameIter->second;

        if (passwordIter != options.end())
            password = passwordIter->second;

        gen->Initialize(username, password, gen->GetIndex());

        if (username.empty())
            return false;
    }

    if (serverIter != options.end())
        ParseServer(serverIter->second);

    return !m_Host.empty();
}

} // ns avalanche
