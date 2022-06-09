#ifndef EVENT_H
#define EVENT_H

#include "byte-parser.h"

class Event {
protected:
    Event_code code;
public:
    Event_code get_code() {return code;};

    virtual std::vector<std::byte> to_bytes() = 0;
};

class Bomb_placed_event : public Event {
private:
    bomb_id id;
    position_t position;
public:
    Bomb_placed_event(bomb_id id, position_t position) : id(id), position(position) {
        code = Event_code::Bomb_placed;
    }

    bomb_id get_id() {return id;}

    position_t get_position() {return position;}

    std::vector<std::byte> to_bytes();

};

class Bomb_exploded_event : public Event {
private:
    bomb_id id;
    std::vector<player_id> robots_destroyed;
    std::vector<position_t> blocks_destroyed;

public:
    Bomb_exploded_event(bomb_id id, std::vector<player_id> robots_destroyed, std::vector<position_t> blocks_destroyed)
        : id(id), robots_destroyed(robots_destroyed), blocks_destroyed(blocks_destroyed) {
            code = Event_code::Bomb_exploded;
        }
    
    bomb_id get_id() {return id;}
    std::vector<player_id> get_robots_destroyed() {return robots_destroyed;}
    std::vector<position_t> get_blocks_destroyed() {return blocks_destroyed;}

    std::vector<std::byte> to_bytes();

};

class Player_moved_event : public Event {
private:
    player_id id;
    position_t position;
public:
    Player_moved_event(player_id id, position_t position) : id(id), position(position) {
        code = Event_code::Player_moved;
    }
    player_id get_id() {return id;}

    position_t get_position() {return position;}

    std::vector<std::byte> to_bytes();

};

class Block_placed_event : public Event {
private:
    position_t position;
public:
    Block_placed_event(position_t position) : position(position) {
        code = Event_code::Block_placed;
    }

    position_t get_position() {return position;}

    std::vector<std::byte> to_bytes();

};

class Turn_info {
private:
    uint16_t turn;
    std::vector<std::shared_ptr<Event>> events;
public:
    Turn_info(uint16_t turn) : turn(turn) {}

    void add_event(std::shared_ptr<Event> event) {
        events.push_back(event);
    }

    uint16_t get_turn() {return turn;}
    std::vector<std::shared_ptr<Event>> get_events() {return events;}

    std::vector<std::byte> to_bytes();   
};

#endif