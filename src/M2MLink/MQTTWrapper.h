#ifndef __C3P_MQTT_WRAPPER_H
#define __C3P_MQTT_WRAPPER_H

#include "../StringBuilder.h"
#include "../CppPotpourri.h"

class MQTTMessage {
  public:
    char*         topic;
    StringBuilder data;      // Reassembled payload
    int           qos;
    bool          retain;
    bool          dup;
    int           msg_id;

    MQTTMessage() :
      topic(nullptr),
      data(),
      qos(0),
      retain(false),
      dup(false),
      msg_id(0) {}

    void wipe() {
      data.clear();
      _partial_packet.clear();
    }


  private:
    StringBuilder _partial_packet;
};


#endif // __C3P_MQTT_WRAPPER_H
