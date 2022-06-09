#include "byte-parser.h"

void insert_vector(std::shared_ptr<std::vector<std::byte>> dest, std::vector<std::byte> src) {
    dest->insert(dest->end(), src.begin(), src.end());
}

std::byte uint8_to_byte(uint8_t u8) {
    return std::byte(u8);
}

std::vector<std::byte> uint16_to_bytes(uint16_t u16) {
    std::cout << "u16: " << u16 << "\n";
    std::vector<std::byte> message;
    message.push_back((std::byte) (u16 >> 8));
    message.push_back((std::byte) u16);

    return message;
}

std::vector<std::byte> uint32_to_bytes(uint32_t u32) {
    std::vector<std::byte> message;
    message.push_back((std::byte) (u32 >> 24));
    message.push_back((std::byte) (u32 >> 16));
    message.push_back((std::byte) (u32 >> 8));
    message.push_back((std::byte) u32);

    return message;
}

std::vector<std::byte> string_to_bytes(std::string str) {
    std::byte message[str.size() + 1];

    message[0] = uint8_to_byte((uint8_t) str.size());

    std::memcpy(message + 1, str.data(), str.size());

    return std::vector<std::byte>(message, message + str.size() + 1);
}

std::vector<std::byte> player_to_bytes(player_t player) {
    std::vector<std::byte> message;

    std::vector<std::byte> name = string_to_bytes(player.name);
    message.insert(message.end(), name.begin(), name.end());

    std::vector<std::byte> address = string_to_bytes(player.address);
    message.insert(message.end(), address.begin(), address.end());

    return message;
}

std::vector<std::byte> position_to_bytes(position_t position) {
    std::vector<std::byte> message;

    std::vector<std::byte> x = uint16_to_bytes(position.x);
    message.insert(message.end(), x.begin(), x.end());

    std::vector<std::byte> y = uint16_to_bytes(position.y);
    message.insert(message.end(), y.begin(), y.end());

    return message;
}

std::vector<std::byte> bomb_to_bytes(bomb_t bomb) {
    std::vector<std::byte> message;

    std::vector<std::byte> position = position_to_bytes(bomb.position);
    message.insert(message.end(), position.begin(), position.end());

    std::vector<std::byte> timer = uint16_to_bytes(bomb.timer);
    message.insert(message.end(), timer.begin(), timer.end());

    return message;
}

std::vector<std::byte> position_set_to_bytes(std::set<position_t> positions) {
    std::vector<std::byte> message;

    std::vector<std::byte> list_size = uint32_to_bytes((uint32_t) positions.size());
    message.insert(message.end(), list_size.begin(), list_size.end());

    for (auto position : positions) {
        std::vector<std::byte> position_bytes = position_to_bytes(position);
        message.insert(message.end(), position_bytes.begin(), position_bytes.end());
    }

    return message;
}

std::vector<std::byte> bomb_set_to_bytes(std::map<bomb_id, bomb_t> bombs) {
    std::vector<std::byte> message;

    std::vector<std::byte> list_size = uint32_to_bytes((uint32_t) bombs.size());
    message.insert(message.end(), list_size.begin(), list_size.end());

    for (auto bomb : bombs) {
        std::vector<std::byte> bomb_bytes = bomb_to_bytes(bomb.second);
        message.insert(message.end(), bomb_bytes.begin(), bomb_bytes.end());
    }

    return message;
}

std::vector<std::byte> player_map_to_bytes(std::map<player_id, player> player_map) {
    std::vector<std::byte> message;

    std::vector<std::byte> map_size = uint32_to_bytes((uint32_t) player_map.size());
    message.insert(message.end(), map_size.begin(), map_size.end());

    for (auto &player : player_map) {
        message.push_back(uint8_to_byte(player.first));

        std::vector<std::byte> player_bytes = player_to_bytes(player.second);
        message.insert(message.end(), player_bytes.begin(), player_bytes.end());
    }

    return message;
}

std::vector<std::byte> player_position_map_to_bytes(std::map<player_id, position_t> positions) {
    std::vector<std::byte> message;

    std::vector<std::byte> map_size = uint32_to_bytes((uint32_t) positions.size());
    message.insert(message.end(), map_size.begin(), map_size.end());

    for (auto &player : positions) {
        message.push_back(uint8_to_byte(player.first));

        std::vector<std::byte> position = position_to_bytes(player.second);
        message.insert(message.end(), position.begin(), position.end());
    }

    return message;
}

std::vector<std::byte> score_map_to_bytes(std::map<player_id, score> scores) {
    std::vector<std::byte> message;

    std::vector<std::byte> map_size = uint32_to_bytes((uint32_t) scores.size());
    message.insert(message.end(), map_size.begin(), map_size.end());

    for (auto &player : scores) {
        message.push_back(uint8_to_byte(player.first));

        std::vector<std::byte> score_bytes = uint32_to_bytes(player.second);
        message.insert(message.end(), score_bytes.begin(), score_bytes.end());
    }

    return message;
}

// added with server implementation

std::vector<std::byte> uint8_list_to_bytes(std::vector<uint8_t> elements) {
    std::shared_ptr<std::vector<std::byte>> msg(new std::vector<std::byte>);

    insert_vector(msg, uint32_to_bytes((uint32_t)elements.size()));

    for (auto element : elements) {
        msg->push_back(uint8_to_byte(element));
    }

    return *msg;
}

std::vector<std::byte> position_list_to_bytes(std::vector<position_t> positions) {
    std::shared_ptr<std::vector<std::byte>> msg(new std::vector<std::byte>);

    insert_vector(msg, uint32_to_bytes((uint32_t) positions.size()));

    for (auto position : positions) {
        insert_vector(msg, position_to_bytes(position));
    }

    return *msg;    
}