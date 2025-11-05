#pragma once

namespace svc {
  // Starts the service logic and blocks until stop() is called.
  // Returns true on normal shutdown.
  bool run();

  // Signals the service to stop and unblock run().
  void stop();
}
