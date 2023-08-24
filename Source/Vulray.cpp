#include "Vulray/Vulray.h"

namespace vr
{
    VulrayLogCallback LogCallback = nullptr;

    namespace detail
    {
        char _gStringFormatBuffer[1024];
    }

} // namespace vr
