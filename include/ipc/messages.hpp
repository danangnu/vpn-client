#pragma once
#include <string>

// JSON strings on the wire. Keep it simple for now.
namespace msgs {
  // Example Connect request JSON:
  // {
  //   "method":"Connect",
  //   "params":{
  //     "endpoint":"wg1.example.com:51820",
  //     "peerPublicKey":"<base64>",
  //     "privateKey":"<base64>",
  //     "allowedIps":"0.0.0.0/0, ::/0",
  //     "mtu":1420,
  //     "dns":["1.1.1.1","2606:4700:4700::1111"],
  //     "country":"SG",
  //     "serverId":"wg-sg-12"
  //   }
  // }

  inline const char* ok()               { return R"({"ok":true})"; }
  inline const char* not_ok()           { return R"({"ok":false})"; }
  inline const char* status_running()   { return R"({"state":"running"})"; }
  inline const char* status_connected() { return R"({"state":"connected"})"; }
  inline const char* status_idle()      { return R"({"state":"idle"})"; }
}
