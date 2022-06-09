#include "event.h"

std::vector<std::byte> Bomb_placed_event::to_bytes() {
    std::shared_ptr<std::vector<std::byte>> msg(new std::vector<std::byte>);

    msg->push_back(uint8_to_byte(code));
    insert_vector(msg, uint32_to_bytes(id));
    insert_vector(msg, position_to_bytes(position));

    return *msg;
}

std::vector<std::byte> Bomb_exploded_event::to_bytes() {
    std::shared_ptr<std::vector<std::byte>> msg(new std::vector<std::byte>);

    msg->push_back(uint8_to_byte(code));
    insert_vector(msg, uint32_to_bytes(id));
    insert_vector(msg, uint8_list_to_bytes(robots_destroyed));
    insert_vector(msg, position_list_to_bytes(blocks_destroyed));

    return *msg;  

}

std::vector<std::byte> Block_placed_event::to_bytes() {
    std::shared_ptr<std::vector<std::byte>> msg(new std::vector<std::byte>);

    msg->push_back(uint8_to_byte(code));
    insert_vector(msg, position_to_bytes(position));

    return *msg;
}

std::vector<std::byte> Player_moved_event::to_bytes() {
    std::shared_ptr<std::vector<std::byte>> msg(new std::vector<std::byte>);

    msg->push_back(uint8_to_byte(code));
    msg->push_back(uint8_to_byte(id));
    insert_vector(msg, position_to_bytes(position));

    return *msg;   
}

std::vector<std::byte> Turn_info::to_bytes() {
    std::shared_ptr<std::vector<std::byte>> msg(new std::vector<std::byte>);

    msg->push_back(uint8_to_byte(Server_message_code::Turn));

    insert_vector(msg, uint16_to_bytes(turn));

    insert_vector(msg, uint32_to_bytes((uint32_t)events.size()));
    for (auto event : events) {
        insert_vector(msg, event->to_bytes());
    }

    return *msg;
}