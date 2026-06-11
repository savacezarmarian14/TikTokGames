#include "input/TikTokAdapter.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace tw {
namespace {

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;

std::string socketErrorMessage() {
    return "WinSock error " + std::to_string(WSAGetLastError());
}

bool isInterruptedSocketError() {
    const int error = WSAGetLastError();
    return error == WSAEINTR || error == WSAEWOULDBLOCK;
}

SocketHandle socketFromAtomicValue(std::intptr_t value) {
    return static_cast<SocketHandle>(value);
}

std::intptr_t socketToAtomicValue(SocketHandle socket) {
    return static_cast<std::intptr_t>(socket);
}
#else
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;

std::string socketErrorMessage() {
    return std::strerror(errno);
}

bool isInterruptedSocketError() {
    return errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK;
}

SocketHandle socketFromAtomicValue(std::intptr_t value) {
    return static_cast<SocketHandle>(value);
}

std::intptr_t socketToAtomicValue(SocketHandle socket) {
    return static_cast<std::intptr_t>(socket);
}
#endif

std::string trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string unescapeJsonString(const std::string& value) {
    std::string out;
    out.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '\\' || i + 1 >= value.size()) {
            out.push_back(value[i]);
            continue;
        }

        const char next = value[++i];
        switch (next) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            default:
                out.push_back(next);
                break;
        }
    }

    return out;
}

bool looksLikeJsonObject(const std::string& json) {
    const auto value = trim(json);
    return value.size() >= 2 && value.front() == '{' && value.back() == '}';
}

std::optional<std::string> extractStringField(const std::string& json, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"((?:\\\\.|[^\\\"\\\\])*)\\\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return std::nullopt;
    }

    return unescapeJsonString(match[1].str());
}

std::optional<int> extractIntField(const std::string& json, const std::string& key) {
    const std::regex numberPattern("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+)");
    std::smatch numberMatch;
    if (std::regex_search(json, numberMatch, numberPattern)) {
        try {
            return std::stoi(numberMatch[1].str());
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    const auto stringValue = extractStringField(json, key);
    if (!stringValue) {
        return std::nullopt;
    }

    try {
        return std::stoi(*stringValue);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<std::string> extractObjectField(const std::string& json, const std::string& key) {
    const std::regex keyPattern("\\\"" + key + "\\\"\\s*:");
    std::smatch match;
    if (!std::regex_search(json, match, keyPattern)) {
        return std::nullopt;
    }

    std::size_t pos = static_cast<std::size_t>(match.position() + match.length());
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }

    if (pos >= json.size() || json[pos] != '{') {
        return std::nullopt;
    }

    const std::size_t begin = pos;
    int depth = 0;
    bool inString = false;
    bool escaped = false;

    for (; pos < json.size(); ++pos) {
        const char c = json[pos];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (inString && c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"') {
            inString = !inString;
            continue;
        }

        if (inString) {
            continue;
        }

        if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0) {
                return json.substr(begin, pos - begin + 1);
            }
        }
    }

    return std::nullopt;
}

std::optional<Team> parseTeamName(const std::string& rawName) {
    const std::string name = toLower(trim(rawName));

    if (name == "red") {
        return Team::Red;
    }
    if (name == "blue") {
        return Team::Blue;
    }
    if (name == "purple") {
        return Team::Purple;
    }
    if (name == "green") {
        return Team::Green;
    }

    return std::nullopt;
}

int teamToTowerId(Team team) {
    return static_cast<int>(team);
}

std::optional<GameCommand> parseActionString(const std::string& action, int repeatCount) {
    std::istringstream stream(action);
    std::string commandName;
    stream >> commandName;

    commandName = toLower(commandName);
    repeatCount = std::max(1, repeatCount);

    if (commandName == "spawn") {
        std::string sourceName;
        std::string targetName;
        stream >> sourceName >> targetName;

        if (sourceName.empty() || targetName.empty()) {
            return std::nullopt;
        }

        const auto sourceTeam = parseTeamName(sourceName);
        const auto targetTeam = parseTeamName(targetName);
        if (!sourceTeam || !targetTeam) {
            return std::nullopt;
        }

        const int sourceId = teamToTowerId(*sourceTeam);
        const int targetId = teamToTowerId(*targetTeam);
        if (sourceId == targetId) {
            return std::nullopt;
        }

        return GameCommand{
            CommandType::SpawnAttack,
            *sourceTeam,
            sourceId,
            targetId,
            repeatCount
        };
    }

    if (commandName == "heal") {
        std::string teamName;
        stream >> teamName;

        const auto team = parseTeamName(teamName);
        if (!team) {
            return std::nullopt;
        }

        const int towerId = teamToTowerId(*team);
        return GameCommand{
            CommandType::SpawnHeal,
            *team,
            towerId,
            towerId,
            repeatCount
        };
    }

    if (commandName == "reset") {
        return GameCommand{
            CommandType::ResetGame,
            Team::None,
            InvalidId,
            InvalidId,
            1
        };
    }

    return std::nullopt;
}

std::optional<ExternalInputCommand> parseExternalInputMessage(const std::string& json) {
    if (!looksLikeJsonObject(json)) {
        std::cerr << "[network-input] Warning: ignored invalid JSON datagram.\n";
        return std::nullopt;
    }

    const auto messageType = extractStringField(json, "message_type");
    if (!messageType) {
        std::cerr << "[network-input] Warning: ignored JSON without message_type.\n";
        return std::nullopt;
    }

    if (*messageType != "game_event") {
        std::cerr << "[network-input] Warning: ignored unsupported message_type='" << *messageType << "'.\n";
        return std::nullopt;
    }

    const auto action = extractStringField(json, "action");
    if (!action || trim(*action).empty()) {
        std::cerr << "[network-input] Warning: ignored game_event without action.\n";
        return std::nullopt;
    }

    ExternalInputMetadata metadata;
    if (const auto eventObject = extractObjectField(json, "event")) {
        metadata.eventId = extractStringField(*eventObject, "id").value_or("");
        metadata.eventType = extractStringField(*eventObject, "type").value_or("");
    }
    if (const auto userObject = extractObjectField(json, "user")) {
        metadata.username = extractStringField(*userObject, "username").value_or("");
        metadata.displayName = extractStringField(*userObject, "display_name").value_or("");
    }
    if (const auto payloadObject = extractObjectField(json, "payload")) {
        metadata.giftName = extractStringField(*payloadObject, "gift_name").value_or("");
        metadata.repeatCount = extractIntField(*payloadObject, "repeat_count").value_or(1);
    }

    const auto command = parseActionString(*action, metadata.repeatCount);
    if (!command) {
        std::cerr << "[network-input] Warning: ignored unsupported action='" << *action << "'.\n";
        return std::nullopt;
    }

    return ExternalInputCommand{*command, metadata};
}

bool commandMatchesTowerCount(const GameCommand& command, std::size_t towerCount) {
    auto isValidTowerId = [towerCount](int towerId) {
        return towerId >= 0 && static_cast<std::size_t>(towerId) < towerCount;
    };

    if (command.type == CommandType::ResetGame) {
        return true;
    }

    if (command.type == CommandType::SpawnHeal) {
        return isValidTowerId(command.targetTowerId);
    }

    if (command.type == CommandType::SpawnAttack) {
        return isValidTowerId(command.sourceTowerId) &&
               isValidTowerId(command.targetTowerId) &&
               command.sourceTowerId != command.targetTowerId;
    }

    return false;
}

void closeSocket(std::atomic<std::intptr_t>& socketFd) {
    const SocketHandle fd = socketFromAtomicValue(socketFd.exchange(socketToAtomicValue(kInvalidSocket)));
    if (fd != kInvalidSocket) {
#ifdef _WIN32
        ::closesocket(fd);
#else
        ::close(fd);
#endif
    }
}

} // namespace

TikTokAdapter::TikTokAdapter(NetworkInputConfig config)
    : config_(std::move(config)) {
    start();
}

TikTokAdapter::~TikTokAdapter() {
    stop();
}

void TikTokAdapter::start() {
    if (!config_.enabled || running_) {
        return;
    }

    running_ = true;
    worker_ = std::thread(&TikTokAdapter::listenLoop, this);
}

void TikTokAdapter::stop() {
    if (!running_ && !worker_.joinable()) {
        return;
    }

    running_ = false;
    closeSocket(socketFd_);

    if (worker_.joinable()) {
        worker_.join();
    }
}

void TikTokAdapter::enqueue(ExternalInputCommand command) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push(std::move(command));
}

std::vector<GameCommand> TikTokAdapter::poll(std::size_t towerCount) {
    std::vector<ExternalInputCommand> externalCommands;

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        while (!queue_.empty()) {
            externalCommands.push_back(std::move(queue_.front()));
            queue_.pop();
        }
    }

    std::vector<GameCommand> commands;
    commands.reserve(externalCommands.size());

    for (const auto& externalCommand : externalCommands) {
        if (!commandMatchesTowerCount(externalCommand.command, towerCount)) {
            std::cerr << "[network-input] Warning: ignored command for unavailable tower. "
                      << "Run the game with enough towers for this action.\n";
            continue;
        }

        if (!externalCommand.metadata.eventId.empty() || !externalCommand.metadata.username.empty()) {
            std::clog << "[network-input] queued action"
                      << " event_id=" << (externalCommand.metadata.eventId.empty() ? "-" : externalCommand.metadata.eventId)
                      << " user=" << (externalCommand.metadata.username.empty() ? "-" : externalCommand.metadata.username)
                      << " gift=" << (externalCommand.metadata.giftName.empty() ? "-" : externalCommand.metadata.giftName)
                      << " repeat=" << externalCommand.metadata.repeatCount
                      << "\n";
        }

        commands.push_back(externalCommand.command);
    }

    return commands;
}

void TikTokAdapter::listenLoop() {
#ifdef _WIN32
    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[network-input] Warning: failed to initialize WinSock: "
                  << socketErrorMessage() << "\n";
        running_ = false;
        return;
    }
#endif

    const SocketHandle fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == kInvalidSocket) {
        std::cerr << "[network-input] Warning: failed to create UDP socket: "
                  << socketErrorMessage() << "\n";
        running_ = false;
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }
    socketFd_.store(socketToAtomicValue(fd));

    int reuseAddress = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuseAddress), sizeof(reuseAddress));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(config_.port);

    if (config_.host.empty() || config_.host == "0.0.0.0") {
        address.sin_addr.s_addr = htonl(INADDR_ANY);
    } else if (::inet_pton(AF_INET, config_.host.c_str(), &address.sin_addr) != 1) {
        std::cerr << "[network-input] Warning: invalid bind host '" << config_.host << "'.\n";
        running_ = false;
        closeSocket(socketFd_);
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "[network-input] Warning: failed to bind UDP "
                  << config_.host << ":" << config_.port << ": "
                  << socketErrorMessage() << "\n";
        running_ = false;
        closeSocket(socketFd_);
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    std::clog << "[network-input] Listening for local JSON game_event UDP messages on "
              << config_.host << ":" << config_.port << "\n";

    std::vector<char> buffer(std::max<std::size_t>(1, config_.maxDatagramSize));

    while (running_) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(fd, &readSet);

        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000;

#ifdef _WIN32
        const int ready = ::select(0, &readSet, nullptr, nullptr, &timeout);
#else
        const int ready = ::select(fd + 1, &readSet, nullptr, nullptr, &timeout);
#endif
        if (!running_) {
            break;
        }

        if (ready < 0) {
            if (isInterruptedSocketError()) {
                continue;
            }
            std::cerr << "[network-input] Warning: select failed: " << socketErrorMessage() << "\n";
            break;
        }

        if (ready == 0 || !FD_ISSET(fd, &readSet)) {
            continue;
        }

        const int received = ::recvfrom(fd, buffer.data(), static_cast<int>(buffer.size()), 0, nullptr, nullptr);
        if (received < 0) {
            if (isInterruptedSocketError()) {
                continue;
            }
            std::cerr << "[network-input] Warning: recvfrom failed: " << socketErrorMessage() << "\n";
            continue;
        }

        if (received == 0) {
            continue;
        }

        const std::string payload(buffer.data(), static_cast<std::size_t>(received));
        if (auto command = parseExternalInputMessage(payload)) {
            enqueue(std::move(*command));
        }
    }

    closeSocket(socketFd_);
#ifdef _WIN32
    WSACleanup();
#endif
}

} // namespace tw
