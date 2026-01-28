#pragma once

#include "ReactorTypes.h"

namespace ReactorHeatControl {
  // Update heat target and tick heat behavior for the current mode.
  void tick(Mode mode);
}
