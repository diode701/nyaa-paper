#pragma once

namespace nyaa::util {

/// NyaaLauncher.detectNvidia: the NVIDIA kernel module is loaded, or
/// nvidia-smi runs. Decides whether the NVIDIA offload option is offered.
bool detectNvidia();

} // namespace nyaa::util
