#include "Utils/Utilities.h"

#include <sstream>

namespace Strontium
{
namespace Utilities
  {
    std::string
    colourToHex(const glm::vec4& colour)
    {
      int r = (int)(colour.r * 255.0f);
      int g = (int)(colour.g * 255.0f);
      int b = (int)(colour.b * 255.0f);
      int a = (int)(colour.a * 255.0f);
  
      unsigned long out = ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) + (a & 0xff);
      std::stringstream stream;
      stream << "#" << std::hex << out;
      return std::string(stream.str()).substr(0, 9);
    }
  }
}