// All of the includes required for Strontium are housed here.
#define GLFW_INCLUDE_NONE

// Include guard.
#pragma once

// STL and standard includes.
#include <iostream>
#include <math.h>
#include <algorithm>
#include <fstream>
#include <stdarg.h>
#include <string>
#include <string.h>
#include <vector>
#include <array>
#include <map>
#include <queue>
#include <stack>
#include <utility>
#include <chrono>
#include <bitset>

// GLM includes. TODO: Move to Math?
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/quaternion.hpp"
#undef GLM_ENABLE_EXPERIMENTAL

// Data Structure includes.
#include "robin-hood/include/robin_hood.h"