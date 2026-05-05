#pragma once

#include <queue>
#include <string>

enum class MessageId {
    Quit,
    ForcedQuit,
    Error,
    Load,
    Unload,
    Reload,
    Start,
    Stop,
    Pause,
    Resume,
    StepIn
};

struct MessageLoad {
    std::string rom_path;
};

struct MessageError {
    std::string msg;
};

struct Message {
    Message(MessageId id) : id{id} {}

    Message(MessageId id, MessageError &&err) : id{id}, err{err} {}

    Message(MessageId id, MessageLoad &&load) : id{id}, load{load} {}

    ~Message() {
        switch (id) {
        case MessageId::Error:
            err.msg.~basic_string();

            break;
        case MessageId::Load:
            load.rom_path.~basic_string();

            break;
        case MessageId::Quit:
        case MessageId::ForcedQuit:
        case MessageId::Unload:
        case MessageId::Reload:
        case MessageId::Start:
        case MessageId::Stop:
        case MessageId::Pause:
        case MessageId::Resume:
        case MessageId::StepIn:
            break;
        }
    }

    MessageId id;

    union {
        MessageLoad load;
        MessageError err;
    };
};

using MessageQueue = std::queue<Message>;
