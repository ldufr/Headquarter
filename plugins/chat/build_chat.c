#ifndef __STDC__
# define __STDC__ 1
#endif

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include <client/Headquarter.h>
#include <common/macro.h>

#ifdef _WIN32
# define DllExport __declspec(dllexport)
#else
# define DllExport
#endif

static CallbackEntry chat_message_cb;
static void on_chat_message(Event *event, void *param)
{
    // We didn't register any other event to this callback, so it shouldn't
    // be possible to ever be called with an other event type.
    assert(event->type == EventType_ChatMessage);

    char message[256];
    size_t message_len = MIN(sizeof(message), event->ChatMessage.message.length);
    for (size_t i = 0; i < message_len; ++i) {
        uint16_t value = event->ChatMessage.message.buffer[i];
        if ((value & ~0x7F) != 0) {
            value = '?';
        }

        message[i] = (char)value;
    }

    char sender[32];
    size_t sender_len = MIN(sizeof(sender), event->ChatMessage.sender.length);
    for (size_t i = 0; i < sender_len; ++i) {
        uint16_t value = event->ChatMessage.sender.buffer[i];
        if ((value & ~0x7F) != 0) {
            value = '?';
        }

        sender[i] = (char)value;
    }

    // The maximum number of characters for a character name is 20 in the game
    // so we right allign all sender name to the right at 20 characters.
    LogInfo("%20.*s: '%.*s'\n", (int)sender_len, sender, (int)message_len, message);
}

DllExport bool PluginEntry(PluginObject *plugin)
{
    CallbackEntry_Init(&chat_message_cb, on_chat_message, NULL);
    if (!RegisterEvent(EventType_ChatMessage, &chat_message_cb))
        return false;

    return true;
}
