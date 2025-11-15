#ifndef CHATBOT_H
#define CHATBOT_H

#include "AI/networkmanager.h"

class ChatBot : public NetworkManager
{
    Q_OBJECT
public:
    ChatBot(){};
    void requestChat(const QString& userText);
signals:
    void chatReplyReady(const QString& intent, const QString& reply);
};

#endif // CHATBOT_H
