#include "sensor_skill.h"
#include "hal/hal_exception.h"

namespace roboclaw::skills {

void SensorSkill::registerSensor(const std::string& name, std::shared_ptr<hal::ISensor> sensor) {
    sensors_[name] = sensor;
}

nlohmann::json SensorSkill::readSensor(const std::string& name) {
    auto it = sensors_.find(name);
    if (it == sensors_.end()) {
        throw hal::SensorException(name, "Sensor not registered");
    }

    if (!it->second->isAvailable()) {
        throw hal::SensorException(name, "Sensor not available");
    }

    return it->second->readData();
}

nlohmann::json SensorSkill::readAll() {
    nlohmann::json result;
    for (const auto& [name, sensor] : sensors_) {
        if (sensor->isAvailable()) {
            result[name] = sensor->readData();
        }
    }
    return result;
}

bool SensorSkill::isAvailable(const std::string& name) {
    auto it = sensors_.find(name);
    return it != sensors_.end() && it->second->isAvailable();
}

} // namespace roboclaw::skills
