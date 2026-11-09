#pragma once

namespace BMP280 {
bool begin();
bool read(float &temperatureC, float &pressureHpa);
}
