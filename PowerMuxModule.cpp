#include <cstdint> 
#include <iostream>
#include <fstream>
#include <array>

#include "json.hpp"

using json = nlohmann::ordered_json;

struct Connector
{
    bool isActive;
    // Data from PowerMuxModule to ConnectorModule
    float EVSEMaxCurrent;
    float EVSEMaxVoltage;

    float EVSEMinCurrent;
    float EVSEMinVoltage;

    float EVSEPresentCurrent;
    float EVSEPresentVoltage;

    float EVSEMaxPower;
    // Data from ConnectorModule to PowerMuxModule
    //PLCModule::ControlPilotState controlPilotState;
    //PLCModule::StateMachineState stateMachineState;

    float EVMaxCurrent;
    float EVMaxVoltage;

    float EVTargetCurrent;
    float EVTargetVoltage;

    float EVMaxPower;

};


enum class FaultBits : uint32_t
{
    NO_FAULT = 0,
    INPUT_UNDER_VOLTAGE = 1,
    INPUT_OVER_VOLTAGE = 2,
    OUTPUT_OVER_VOLTAGE = 3,
    OUTPUT_OVER_CURRENT = 4,
    HIGH_TEMPERATURE = 5,
    FAN_FAULT = 6,
    HARDWARE_FAULT = 7,
    BUS_EXCEPTION = 8,
    SCI_COMM_EXCEPTION = 9,
    DISCHARGE_FAULT = 10,
    PFC_SHUTDOWN_EXCEPTION = 11,
    OUTPUT_UNDER_VOLTAGE_WARNING = 12,
    OUTPUT_OVER_VOLTAGE_WARNING = 13,
    POWER_LIMIT_HIGH_TEMP = 14,
    SHORT_CIRCUIT_FAULT = 15
};
enum class ChargingModuleState : uint8_t
{
    NORMAL_OFF = 0x00,
    ON = 0x01,
    FAULT_OFF = 0x11
};

enum class ProfilingType : uint8_t
{
    INCREASE = 0x00,
    DECREASE = 0x01
};

enum class ConnectorType : uint8_t
{
    DEFAULT = 0x00,
    Connector1 = 0x01,
    Connector2 = 0x02,
    Connector3 = 0x03,
    Connector4 = 0x04,
    Connector5 = 0x05,
    Connector6 = 0x06,
    Connector7 = 0x07,
    Connector8 = 0x08,
    Connector9 = 0x09,
    Connector10 = 0x0A,
    Connector11 = 0x0B,
    Connector12 = 0x0C
};

struct ModuleStatus
{
    bool isActive;
    bool isAlive;
    ConnectorType Connector;
    ChargingModuleState state;
    uint32_t moduleAddress;
    float MaxVoltage;
    float MaxCurrent;
    float MinVoltage;
    float MinCurrent;
    float MaxPower;
    float MinPower;
    float MaxTemperature;
    float MinTemperature;
    float PhaseAVoltage;
    float PhaseBVoltage;
    float PhaseCVoltage;
    float temperature;
    float inputVoltage;
    float inputCurrent;
    float outputVoltage;
    float outputCurrent;
    bool isFaultTriggered;
    bool isProfilingOngoing;
    ProfilingType ProfileType;
    std::array<bool, 24> faultBits;

    ModuleStatus() : isActive(false),
        Connector(ConnectorType::DEFAULT),
        state(ChargingModuleState::NORMAL_OFF),
        PhaseAVoltage(0.0f),
        PhaseBVoltage(0.0f),
        PhaseCVoltage(0.0f),
        temperature(0.0f),
        inputVoltage(0.0f),
        inputCurrent(0.0f),
        outputVoltage(0.0f),
        outputCurrent(0.0f),
        isFaultTriggered(false),
        isProfilingOngoing(false),
        ProfileType(ProfilingType::INCREASE),

        // Temp substitutions
        isAlive(true),
        MaxCurrent(30.0f)
    {
        faultBits.fill(false);
    }
};

ModuleStatus pmArray[49]; // 0 : Default , 1-48 : modules
Connector connectorArray[13]; // 0 : Default , 1-12 : connectors

struct ConnectorPairMux {
    ConnectorType  connectorA;
    ConnectorType  connectorB;
    uint16_t muxId;
    bool status;
};

struct PmPairRelayMux {
    uint16_t pmA;
    uint16_t pmB;
    uint16_t muxId;
    bool status;
};

ConnectorPairMux connectorPairMuxTable[] = {
    { ConnectorType::Connector1,  ConnectorType::Connector3,  301, false },
    { ConnectorType::Connector1,  ConnectorType::Connector4,  302, false },
    { ConnectorType::Connector2,  ConnectorType::Connector3,  303, false },
    { ConnectorType::Connector2,  ConnectorType::Connector4,  304, false },

    { ConnectorType::Connector5,  ConnectorType::Connector7,  305, false },
    { ConnectorType::Connector5,  ConnectorType::Connector8,  306, false },
    { ConnectorType::Connector6,  ConnectorType::Connector7,  307, false },
    { ConnectorType::Connector6,  ConnectorType::Connector8,  308, false },

    { ConnectorType::Connector9,  ConnectorType::Connector11, 309, false },
    { ConnectorType::Connector9,  ConnectorType::Connector12, 310, false },
    { ConnectorType::Connector10, ConnectorType::Connector11, 311, false },
    { ConnectorType::Connector10, ConnectorType::Connector12, 312, false },

    { ConnectorType::Connector4,  ConnectorType::Connector5,  401, false },
    { ConnectorType::Connector8,  ConnectorType::Connector9,  402, false },
    { ConnectorType::Connector1,  ConnectorType::Connector12, 403, false }
};


PmPairRelayMux relayMuxTable[] = {
    {1, 3, 201, false},
    {3, 5, 202, false},
    {5, 7, 203, false},

    {9 , 11, 204, false},
    {11, 13, 205, false},
    {13, 15, 206, false},

    {17, 19, 207, false},
    {19, 21, 208, false},
    {21, 23, 209, false},

    {25, 27, 210, false},
    {27, 29, 211, false},
    {29, 31, 212, false},

    {33, 35, 213, false},
    {35, 37, 214, false},
    {37, 39, 215, false},

    {41, 43, 216, false},
    {43, 45, 217, false},
    {45, 47, 218, false}
};

uint16_t relayMuxCount = sizeof(relayMuxTable) / sizeof(relayMuxTable[0]);
uint16_t connectorMuxCount = sizeof(connectorPairMuxTable) / sizeof(connectorPairMuxTable[0]);

//********************************   JSON UTILS START   ********************************************************/

void to_json(json& j, const Connector& c) {
    j = json{
        {"isActive", c.isActive},
        {"EVSEMaxCurrent", c.EVSEMaxCurrent},
        {"EVSEMaxVoltage", c.EVSEMaxVoltage},
        {"EVSEMinCurrent", c.EVSEMinCurrent},
        {"EVSEMinVoltage", c.EVSEMinVoltage},
        {"EVSEPresentCurrent", c.EVSEPresentCurrent},
        {"EVSEPresentVoltage", c.EVSEPresentVoltage},
        {"EVSEMaxPower", c.EVSEMaxPower},
        {"EVMaxCurrent", c.EVMaxCurrent},
        {"EVMaxVoltage", c.EVMaxVoltage},
        {"EVTargetCurrent", c.EVTargetCurrent},
        {"EVTargetVoltage", c.EVTargetVoltage},
        {"EVMaxPower", c.EVMaxPower}
    };
}

void from_json(const nlohmann::json& j, Connector& c) {
    j.at("isActive").get_to(c.isActive);
    j.at("EVSEMaxCurrent").get_to(c.EVSEMaxCurrent);
    j.at("EVSEMaxVoltage").get_to(c.EVSEMaxVoltage);
    j.at("EVSEMinCurrent").get_to(c.EVSEMinCurrent);
    j.at("EVSEMinVoltage").get_to(c.EVSEMinVoltage);
    j.at("EVSEPresentCurrent").get_to(c.EVSEPresentCurrent);
    j.at("EVSEPresentVoltage").get_to(c.EVSEPresentVoltage);
    j.at("EVSEMaxPower").get_to(c.EVSEMaxPower);
    j.at("EVMaxCurrent").get_to(c.EVMaxCurrent);
    j.at("EVMaxVoltage").get_to(c.EVMaxVoltage);
    j.at("EVTargetCurrent").get_to(c.EVTargetCurrent);
    j.at("EVTargetVoltage").get_to(c.EVTargetVoltage);
    j.at("EVMaxPower").get_to(c.EVMaxPower);
}


// ------------ helpers ------------

std::string connectorName(ConnectorType type) {
    switch (type) {
    case ConnectorType::DEFAULT: return "DEFAULT";
    case ConnectorType::Connector1: return "Connector1";
    case ConnectorType::Connector2: return "Connector2";
    case ConnectorType::Connector3: return "Connector3";
    case ConnectorType::Connector4: return "Connector4";
    case ConnectorType::Connector5: return "Connector5";
    case ConnectorType::Connector6: return "Connector6";
    case ConnectorType::Connector7: return "Connector7";
    case ConnectorType::Connector8: return "Connector8";
    case ConnectorType::Connector9: return "Connector9";
    case ConnectorType::Connector10: return "Connector10";
    case ConnectorType::Connector11: return "Connector11";
    case ConnectorType::Connector12: return "Connector12";
    default: return "UNKNOWN";
    }
}

std::string stateToString(ChargingModuleState state) {
    switch (state) {
    case ChargingModuleState::NORMAL_OFF: return "NORMAL_OFF";
    case ChargingModuleState::ON: return "ON";
    case ChargingModuleState::FAULT_OFF: return "FAULT_OFF";
    default: return "UNKNOWN";
    }
}

std::string profilingToString(ProfilingType profiling) {
    switch (profiling) {
    case ProfilingType::INCREASE: return "INCREASE";
    case ProfilingType::DECREASE: return "DECREASE";
    default: return "UNKNOWN";
    }
}

std::vector<std::string> faultBitsToString(const std::array<bool, 24>& faultBits) {
    std::vector<std::string> faults;
    std::array<std::string, 16> faultNames = {
        "NO_FAULT", "INPUT_UNDER_VOLTAGE", "INPUT_OVER_VOLTAGE", "OUTPUT_OVER_VOLTAGE",
        "OUTPUT_OVER_CURRENT", "HIGH_TEMPERATURE", "FAN_FAULT", "HARDWARE_FAULT",
        "BUS_EXCEPTION", "SCI_COMM_EXCEPTION", "DISCHARGE_FAULT", "PFC_SHUTDOWN_EXCEPTION",
        "OUTPUT_UNDER_VOLTAGE_WARNING", "OUTPUT_OVER_VOLTAGE_WARNING", "POWER_LIMIT_HIGH_TEMP",
        "SHORT_CIRCUIT_FAULT"
    };

    for (size_t i = 0; i < faultBits.size(); ++i) {
        if (faultBits[i]) {
            faults.push_back(faultNames[i]);
        }
    }
    return faults;
}

ConnectorType stringToConnector(const std::string& str) {
    if (str == "Connector1") return ConnectorType::Connector1;
    if (str == "Connector2") return ConnectorType::Connector2;
    if (str == "Connector3") return ConnectorType::Connector3;
    if (str == "Connector4") return ConnectorType::Connector4;
    if (str == "Connector5") return ConnectorType::Connector5;
    if (str == "Connector6") return ConnectorType::Connector6;
    if (str == "Connector7") return ConnectorType::Connector7;
    if (str == "Connector8") return ConnectorType::Connector8;
    if (str == "Connector9") return ConnectorType::Connector9;
    if (str == "Connector10") return ConnectorType::Connector10;
    if (str == "Connector11") return ConnectorType::Connector11;
    if (str == "Connector12") return ConnectorType::Connector12;
    return ConnectorType::DEFAULT;
}

// Helper function to convert string to ChargingModuleState
ChargingModuleState stringToState(const std::string& str) {
    if (str == "ON") return ChargingModuleState::ON;
    if (str == "FAULT_OFF") return ChargingModuleState::FAULT_OFF;
    return ChargingModuleState::NORMAL_OFF;
}

// Helper function to convert string to ProfilingType
ProfilingType stringToProfiling(const std::string& str) {
    if (str == "INCREASE") return ProfilingType::INCREASE;
    return ProfilingType::DECREASE;
}

// ------------ functions ------------

// Save connectorArray → JSON file
void saveConnectorArrayToJson(const std::string& filename) {
    json j;
    for (int i = 0; i < 13; i++) {
        ConnectorType type = static_cast<ConnectorType>(i);
        j[connectorName(type)] = connectorArray[i];  // uses to_json
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file for writing: " << filename << "\n";
        return;
    }
    file << j.dump(4);  // pretty print with 4 spaces
}

// Load JSON file → connectorArray
void loadConnectorArrayFromJson(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file for reading: " << filename << "\n";
        return;
    }

    json j;
    file >> j;

    for (int i = 1; i <= 12; i++) {

        //ConnectorType type = static_cast<ConnectorType>(i);
        //std::string key = connectorName(type);

        std::string key = "Connector" + std::to_string(i);

        if (j.contains(key)) {
            connectorArray[i] = j[key].get<Connector>();  // uses from_json
        }
        else {
            std::cout << "ERROR : Connector " << key << " not found in JSON.\n";
        }
    }
}

void createModuleStatusJson(const std::string& filename) {
    json j = json::array();  // JSON array to hold all ModuleStatus objects

    for (int i = 0; i < 49; ++i) {
        json moduleJson;

        // Convert fields to JSON-friendly formats
        moduleJson["isActive"] = pmArray[i].isActive;
        moduleJson["isAlive"] = pmArray[i].isAlive;
        moduleJson["Connector"] = connectorName(pmArray[i].Connector);
        moduleJson["state"] = stateToString(pmArray[i].state);
        //moduleJson["moduleAddress"] = pmArray[i].moduleAddress;
        moduleJson["moduleAddress"] = i;

        moduleJson["MaxVoltage"] = pmArray[i].MaxVoltage;
        moduleJson["MaxCurrent"] = pmArray[i].MaxCurrent;
        moduleJson["MinVoltage"] = pmArray[i].MinVoltage;
        moduleJson["MinCurrent"] = pmArray[i].MinCurrent;
        moduleJson["MaxPower"] = pmArray[i].MaxPower;
        moduleJson["MinPower"] = pmArray[i].MinPower;
        moduleJson["MaxTemperature"] = pmArray[i].MaxTemperature;
        moduleJson["MinTemperature"] = pmArray[i].MinTemperature;
        moduleJson["PhaseAVoltage"] = pmArray[i].PhaseAVoltage;
        moduleJson["PhaseBVoltage"] = pmArray[i].PhaseBVoltage;
        moduleJson["PhaseCVoltage"] = pmArray[i].PhaseCVoltage;
        moduleJson["temperature"] = pmArray[i].temperature;
        moduleJson["inputVoltage"] = pmArray[i].inputVoltage;
        moduleJson["inputCurrent"] = pmArray[i].inputCurrent;
        moduleJson["outputVoltage"] = pmArray[i].outputVoltage;
        moduleJson["outputCurrent"] = pmArray[i].outputCurrent;

        moduleJson["isFaultTriggered"] = pmArray[i].isFaultTriggered;
        moduleJson["isProfilingOngoing"] = pmArray[i].isProfilingOngoing;
        moduleJson["ProfileType"] = profilingToString(pmArray[i].ProfileType);
        moduleJson["faultBits"] = faultBitsToString(pmArray[i].faultBits);

        // Push this module status to the JSON array
        j.push_back(moduleJson);
    }

    // Output JSON to file
    std::ofstream out(filename);
    if (out.is_open()) {
        out << j.dump(4);  // Pretty print with 4 spaces indentation
        out.close();
    }
}

// Function to load data from JSON into pmArray[49]
void loadModuleStatusJson(const std::string& filename, ModuleStatus pmArray[49]) {
    // Read the JSON file
    std::ifstream in(filename);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    json j;
    in >> j;  // Deserialize the JSON into the json object
    in.close();

    // Ensure there are 49 elements in the JSON array
    if (j.size() != 49) {
        throw std::runtime_error("Expected 49 ModuleStatus entries in JSON, found " + std::to_string(j.size()));
    }

    // Populate pmArray from the JSON data
    for (size_t i = 0; i < 49; ++i) {
        const json& moduleJson = j[i];

        // Populate each ModuleStatus
        pmArray[i].isActive = moduleJson.value("isActive", false);
        pmArray[i].isAlive = moduleJson.value("isAlive", false);
        pmArray[i].Connector = stringToConnector(moduleJson.value("Connector", "DEFAULT"));
        pmArray[i].state = stringToState(moduleJson.value("state", "NORMAL_OFF"));
        pmArray[i].moduleAddress = moduleJson.value("moduleAddress", 0);

        // Populate float fields with default 0.0f if missing or invalid
        pmArray[i].MaxVoltage = moduleJson.value("MaxVoltage", 0.0f);
        pmArray[i].MaxCurrent = moduleJson.value("MaxCurrent", 0.0f);
        pmArray[i].MinVoltage = moduleJson.value("MinVoltage", 0.0f);
        pmArray[i].MinCurrent = moduleJson.value("MinCurrent", 0.0f);
        pmArray[i].MaxPower = moduleJson.value("MaxPower", 0.0f);
        pmArray[i].MinPower = moduleJson.value("MinPower", 0.0f);
        pmArray[i].MaxTemperature = moduleJson.value("MaxTemperature", 0.0f);
        pmArray[i].MinTemperature = moduleJson.value("MinTemperature", 0.0f);
        pmArray[i].PhaseAVoltage = moduleJson.value("PhaseAVoltage", 0.0f);
        pmArray[i].PhaseBVoltage = moduleJson.value("PhaseBVoltage", 0.0f);
        pmArray[i].PhaseCVoltage = moduleJson.value("PhaseCVoltage", 0.0f);
        pmArray[i].temperature = moduleJson.value("temperature", 0.0f);
        pmArray[i].inputVoltage = moduleJson.value("inputVoltage", 0.0f);
        pmArray[i].inputCurrent = moduleJson.value("inputCurrent", 0.0f);
        pmArray[i].outputVoltage = moduleJson.value("outputVoltage", 0.0f);
        pmArray[i].outputCurrent = moduleJson.value("outputCurrent", 0.0f);

        pmArray[i].isFaultTriggered = moduleJson.value("isFaultTriggered", false);
        pmArray[i].isProfilingOngoing = moduleJson.value("isProfilingOngoing", false);
        pmArray[i].ProfileType = stringToProfiling(moduleJson.value("ProfileType", "INCREASE"));

        // Populate faultBits array
        auto faultBitsJson = moduleJson.value("faultBits", json::array());
        for (size_t j = 0; j < faultBitsJson.size() && j < pmArray[i].faultBits.size(); ++j) {
            pmArray[i].faultBits[j] = (faultBitsJson[j].get<std::string>() != "NO_FAULT");
        }
    }
}

// Function: build JSON and dump to console + file
void createConnectorModuleJson2(const std::string& filename) {
    json j;

    // ensure all connectors appear in order, even if empty
    for (int c = 1; c <= 12; c++) {
        std::string key = "Connector" + std::to_string(c);
        j[key] = json::array();
    }

    for (int i = 0; i < 49; i++) {
        std::string key = connectorName(pmArray[i].Connector);
        if (key != "DEFAULT") {
            //j[key].push_back(pmArray[i].moduleAddress);
            j[key].push_back(i);
        }
    }

    // Open output file for writing
    std::ofstream out(filename);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    // Print each connector in a single line to both console and file
    for (int c = 1; c <= 12; c++) {
        std::string key = "Connector" + std::to_string(c);

        // Create a JSON object for the current connector
        json connectorJson;
        connectorJson[key] = j[key];  // Get the connector array

        // Print the connector as a single line in console
        std::cout << connectorJson.dump() << std::endl;

        // Write the connector to file (single line per connector)
        out << connectorJson.dump() << std::endl;  // Single-line output to file
    }

    out.close();
}

void createConnectorModuleJson(const std::string& filename) {
    json j;

    // ensure all connectors appear in order, even if empty
    for (int c = 1; c <= 12; c++) {
        std::string key = "Connector" + std::to_string(c);
        j[key] = json::array();
    }

    for (int i = 0; i < 49; i++) {
        std::string key = connectorName(pmArray[i].Connector);
        if (key != "DEFAULT") {
            j[key].push_back(i);
        }
    }

    // Open output file for writing
    std::ofstream out(filename);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    // Write the entire JSON object in one go
    out << j.dump(4) << std::endl;   // pretty print with 4 spaces
    out.close();

    // Also print to console
    std::cout << j.dump(4) << std::endl;
}

void createMuxRelayJson(const std::string& filename) {
    json j;  // use object, not array

    // add relay entries
    for (size_t i = 0; i < relayMuxCount; i++) {
        j[std::to_string(relayMuxTable[i].muxId)] = relayMuxTable[i].status;
    }

    // add mux entries
    for (size_t i = 0; i < connectorMuxCount; i++) {
        j[std::to_string(connectorPairMuxTable[i].muxId)] = connectorPairMuxTable[i].status;
    }

    // write to file
    std::ofstream file(filename);
    if (file.is_open()) {
        file << j.dump(4); // pretty print
    }
}


//******************************************   JSON UTILS END   ******************************************************/



//Funtion Declarations
void isolateModule(uint16_t module);
void isolateConnector(ConnectorType connector);
void printModuleStatus();
void printMuxStatus();
void printRelayStatus();

uint16_t defaultModule(ConnectorType connector) {
    return ((static_cast<uint16_t>(connector) - 1) / 2) * 8 + ((static_cast<uint16_t>(connector) % 2 == 1) ? 1 : 7);
}

uint16_t subset(ConnectorType connector) {
    return (static_cast<uint16_t>(connector) - 1) / 2 + 1;
}

uint16_t superset(ConnectorType connector) {
    return (static_cast<uint16_t>(connector) - 1) / 4 + 1;
}

bool connectorStatus(ConnectorType connector) {
    return connectorArray[static_cast<uint8_t>(connector)].isActive;
}

bool moduleStatus(uint16_t module) {
    return pmArray[module].isActive;
}

ConnectorType getdefaultConnector(uint16_t module) {

    if (module < 1 || module > 47) std::cerr << "Invalid module number. Must be between 1 and 47.\n";

    switch (module) {
    case 1:  return ConnectorType::Connector1;
    case 7:  return ConnectorType::Connector2;
    case 9:  return ConnectorType::Connector3;
    case 15: return ConnectorType::Connector4;
    case 17: return ConnectorType::Connector5;
    case 23: return ConnectorType::Connector6;
    case 25: return ConnectorType::Connector7;
    case 31: return ConnectorType::Connector8;
    case 33: return ConnectorType::Connector9;
    case 39: return ConnectorType::Connector10;
    case 41: return ConnectorType::Connector11;
    case 47: return ConnectorType::Connector12;
    default: return ConnectorType::DEFAULT;  // no default connector for other modules
    }
}


void getRelay(int moduleId, uint16_t relayIds[2]) {

    int count = 0;

    for (int i = 0; i < relayMuxCount && count < 2; i++) {
        if (relayMuxTable[i].pmA == moduleId || relayMuxTable[i].pmB == moduleId) {
            relayIds[count++] = relayMuxTable[i].muxId;
        }
    }

    while (count < 2) {
        relayIds[count++] = 0;
    }
}

bool relayStatus(uint16_t relayId) {
    for (int i = 0; i < relayMuxCount; i++) {
        if (relayMuxTable[i].muxId == relayId) {
            return relayMuxTable[i].status;
        }
    }
    return false;
}


bool assign(ConnectorType connector, uint16_t module) {

    //Adjust if module directly gets assigned if becomes alive
    //NOW : if supplementary module is active, it waits until next assignment

    if (pmArray[module].isActive || pmArray[module + 1].isActive) return false; // already assigned

    // Check if the module is alive before assigning
    if (pmArray[module].isAlive) {
        pmArray[module].Connector = connector;
        pmArray[module].isActive = true;
    }

    // Check if the supplementary module is alive before assigning
    if (pmArray[module + 1].isAlive) {
        pmArray[module + 1].Connector = connector;
        pmArray[module + 1].isActive = true;
    }

    return (pmArray[module].isAlive || pmArray[module + 1].isAlive) ? true : false;
}


void relay_on(uint16_t moduleA, uint16_t moduleB) {
    if (moduleA > moduleB) {
        std::swap(moduleA, moduleB);
    }

    if (moduleB - moduleA == 2) {
        for (uint16_t i = 0; i < relayMuxCount; i++) {
            if (relayMuxTable[i].pmA == moduleA && relayMuxTable[i].pmB == moduleB) {
                relayMuxTable[i].status = true;
                std::cout << " : ON";
                return;
            }
        }
    }
    else if (moduleB - moduleA == 4) {
        relay_on(moduleA, moduleA + 2);
        relay_on(moduleA + 2, moduleB);
    }
    else if (moduleB - moduleA == 6) {
        relay_on(moduleA, moduleA + 2);
        relay_on(moduleA + 2, moduleA + 4);
        relay_on(moduleA + 4, moduleB);
    }
    else {
        std::cerr << " : No Connecting Found!";
    }
}

void mux_on(uint16_t muxid) {
    for (uint16_t i = 0; i < connectorMuxCount; i++) {
        if (connectorPairMuxTable[i].muxId == muxid) {
            connectorPairMuxTable[i].status = true;
            std::cout << "\nMux " << muxid << " is ON";
            return;
        }
    }
    std::cerr << "\nMux " << muxid << " not found!";
}

void mux_off(uint16_t muxid) {
    for (uint16_t i = 0; i < connectorMuxCount; i++) {
        if (connectorPairMuxTable[i].muxId == muxid) {
            connectorPairMuxTable[i].status = false;
            std::cout << "\nMux " << muxid << " is OFF";
            return;
        }
    }
    std::cerr << "\nMux " << muxid << " not found!";
}

void allMuxesOff(ConnectorType connector) {
    for (uint16_t i = 0; i < connectorMuxCount; i++) {
        if (connectorPairMuxTable[i].connectorA == connector || connectorPairMuxTable[i].connectorB == connector) {
            connectorPairMuxTable[i].status = false;
        }
    }
}

bool isMuxIsolation(ConnectorType connector) {
    for (uint16_t i = 0; i < connectorMuxCount; i++) {
        if (connectorPairMuxTable[i].connectorA == connector || connectorPairMuxTable[i].connectorB == connector) {
            if (connectorPairMuxTable[i].status) {
                return false;
            }
        }
    }
    return true;
}

uint16_t subsetModuleBegin(uint16_t subsetId) {
    return (subsetId - 1) * 8 + 1;
}

uint16_t supersetModuleBegin(uint16_t supersetId) {
    return (supersetId - 1) * 16 + 1;
}

uint16_t muxExistence(ConnectorType connectorA, ConnectorType connectorB) {
    for (uint16_t i = 0; i < connectorMuxCount; i++) {
        if ((connectorPairMuxTable[i].connectorA == connectorA && connectorPairMuxTable[i].connectorB == connectorB) ||
            (connectorPairMuxTable[i].connectorA == connectorB && connectorPairMuxTable[i].connectorB == connectorA)) {
            return connectorPairMuxTable[i].muxId;
        }
    }
    return 0;
}

bool muxStatus(uint16_t muxId) {
    for (uint16_t i = 0; i < connectorMuxCount; i++) {
        if (connectorPairMuxTable[i].muxId == muxId) {
            return connectorPairMuxTable[i].status;
        }
    }
    return false;
}

ConnectorType getActiveConnector(uint16_t module) {
    return pmArray[module].Connector;
    //return ConnectorType::DEFAULT;
}

//TODO change i to connectorMuxCount
void getActiveMuxes(ConnectorType connector, uint16_t* outputArray) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < connectorMuxCount; i++) { //Max Active Muxes = 2
        if (connectorPairMuxTable[i].connectorA == connector || connectorPairMuxTable[i].connectorB == connector) {
            if (connectorPairMuxTable[i].status) {
                outputArray[count++] = connectorPairMuxTable[i].muxId;
            }
        }
    }
}

void getAllMuxes(ConnectorType connector, uint16_t* outputArray) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < connectorMuxCount; i++) {// Max muxes = 4 . 2 subset mux, 1 super, 1 priority(optional)
        if (connectorPairMuxTable[i].connectorA == connector || connectorPairMuxTable[i].connectorB == connector) {
            outputArray[count++] = connectorPairMuxTable[i].muxId;
        }
    }
}

bool sufficientPower(ConnectorType connector) {

    uint8_t connectorIndex = static_cast<uint8_t>(connector);
    /*
    if (connector >= ConnectorType::Connector1 && connector <= ConnectorType::Connector12) {
        //use if after cast or go for branchless - a little better performance
        return connectorArray[connectorIndex].EVSEMaxCurrent - connectorArray[connectorIndex].EVMaxCurrent > 0;
    }
    */

    float TotalCurrent = 0;

    for (uint16_t i = 1; i < 49; i++) {
        if (pmArray[i].Connector == connector) {
            TotalCurrent += pmArray[i].MaxCurrent;
        }
    }

    return TotalCurrent - connectorArray[connectorIndex].EVMaxCurrent >= 0;

    std::cerr << "Invalid connector type";
    return false;
}

bool extraPower(ConnectorType connector) {

    uint8_t connectorIndex = static_cast<uint8_t>(connector);

    float TotalCurrent = 0;

    for (uint16_t i = 1; i < 49; i++) {
        if (pmArray[i].Connector == connector) {
            TotalCurrent += pmArray[i].MaxCurrent;
        }
    }

    return TotalCurrent - connectorArray[connectorIndex].EVMaxCurrent >= 60;

    std::cerr << "Invalid connector type";
    return false;
}

void assign_power_modules(ConnectorType connector) {

    isolateConnector(connector);

    // Default assignment
    uint16_t connection_module = defaultModule(connector);
    if (assign(connector, connection_module)) {
        connectorArray[static_cast<uint8_t>(connector)].isActive = true;
        if (sufficientPower(connector)) return;
    }


    // Same subset relays
    if (static_cast<uint8_t>(connector) % 2 == 0) {
        if (assign(connector, connection_module - 2)) relay_on(connection_module, connection_module - 2);
        if (sufficientPower(connector)) return;
        if (assign(connector, connection_module - 4)) relay_on(connection_module, connection_module - 4);
        if (sufficientPower(connector)) return;
        if (assign(connector, connection_module - 6)) relay_on(connection_module, connection_module - 6);
        if (sufficientPower(connector)) return;
    }
    else {
        if (assign(connector, connection_module + 2)) relay_on(connection_module, connection_module + 2);
        if (sufficientPower(connector)) return;
        if (assign(connector, connection_module + 4)) relay_on(connection_module, connection_module + 4);
        if (sufficientPower(connector)) return;
        if (assign(connector, connection_module + 6)) relay_on(connection_module, connection_module + 6);
        if (sufficientPower(connector)) return;
    }

    // Normal mux subset (muxId < 400)
    for (uint16_t i = 0; i < connectorMuxCount; i++) {
        if (connectorPairMuxTable[i].muxId > 400) continue;

        ConnectorType peer = ConnectorType::DEFAULT;
        if (connectorPairMuxTable[i].connectorA == connector && !connectorStatus(connectorPairMuxTable[i].connectorB)) {
            peer = connectorPairMuxTable[i].connectorB;
        }
        else if (connectorPairMuxTable[i].connectorB == connector && !connectorStatus(connectorPairMuxTable[i].connectorA)) {
            peer = connectorPairMuxTable[i].connectorA;
        }
        else {
            continue;
        }

        connection_module = defaultModule(peer);
        if (!moduleStatus(connection_module) && pmArray[connection_module].isAlive) {
            assign(connector, connection_module); mux_on(connectorPairMuxTable[i].muxId); if (sufficientPower(connector)) return;
            if (static_cast<uint8_t>(peer) % 2 == 0) {
                if (assign(connector, connection_module - 2)) relay_on(connection_module, connection_module - 2);
                if (sufficientPower(connector)) return;
                if (assign(connector, connection_module - 4)) relay_on(connection_module, connection_module - 4);
                if (sufficientPower(connector)) return;
                if (assign(connector, connection_module - 6)) relay_on(connection_module, connection_module - 6);
                if (sufficientPower(connector)) return;
            }
            else {
                if (assign(connector, connection_module + 2)) relay_on(connection_module, connection_module + 2);
                if (sufficientPower(connector)) return;
                if (assign(connector, connection_module + 4)) relay_on(connection_module, connection_module + 4);
                if (sufficientPower(connector)) return;
                if (assign(connector, connection_module + 6)) relay_on(connection_module, connection_module + 6);
                if (sufficientPower(connector)) return;
            }
            //mux_on(connectorPairMuxTable[i].muxId);
            break;
        }
    }

    // Super mux subset (muxId >= 400)
    for (uint16_t i = 0; i < connectorMuxCount; i++) {
        if (connectorPairMuxTable[i].muxId < 400) continue;

        ConnectorType superPeer = ConnectorType::DEFAULT;
        if (connectorPairMuxTable[i].connectorA == connector && !connectorStatus(connectorPairMuxTable[i].connectorB)) {
            superPeer = connectorPairMuxTable[i].connectorB;
        }
        else if (connectorPairMuxTable[i].connectorB == connector && !connectorStatus(connectorPairMuxTable[i].connectorA)) {
            superPeer = connectorPairMuxTable[i].connectorA;
        }
        else {
            continue;
        }

        connection_module = defaultModule(superPeer);
        if (!moduleStatus(connection_module) && pmArray[connection_module].isAlive) {
            assign(connector, connection_module); mux_on(connectorPairMuxTable[i].muxId); if (sufficientPower(connector)) return;
            if (static_cast<uint8_t>(superPeer) % 2 == 0) {
                if (assign(connector, connection_module - 2)) relay_on(connection_module, connection_module - 2);
                if (sufficientPower(connector)) return;
                if (assign(connector, connection_module - 4)) relay_on(connection_module, connection_module - 4);
                if (sufficientPower(connector)) return;
                if (assign(connector, connection_module - 6)) relay_on(connection_module, connection_module - 6);
                if (sufficientPower(connector)) return;
            }
            else {
                if (assign(connector, connection_module + 2)) relay_on(connection_module, connection_module + 2);
                if (sufficientPower(connector)) return;
                if (assign(connector, connection_module + 4)) relay_on(connection_module, connection_module + 4);
                if (sufficientPower(connector)) return;
                if (assign(connector, connection_module + 6)) relay_on(connection_module, connection_module + 6);
                if (sufficientPower(connector)) return;
            }
            //mux_on(connectorPairMuxTable[i].muxId);
        }

        // Try finding a second-level mux from the peer
        for (uint16_t j = 0; j < connectorMuxCount; j++) {
            if (connectorPairMuxTable[j].muxId > 400) continue;

            ConnectorType subPeer = ConnectorType::DEFAULT;
            if (connectorPairMuxTable[j].connectorA == superPeer && !connectorStatus(connectorPairMuxTable[j].connectorB)) {
                subPeer = connectorPairMuxTable[j].connectorB;
            }
            else if (connectorPairMuxTable[j].connectorB == superPeer && !connectorStatus(connectorPairMuxTable[j].connectorA)) {
                subPeer = connectorPairMuxTable[j].connectorA;
            }
            else {
                continue;
            }

            connection_module = defaultModule(subPeer);

            // Check if the module is not already assigned and is alive
            // change moduleStatus fn to pmArray[module].isActive - check redability
            if (!moduleStatus(connection_module) && pmArray[connection_module].isAlive) {
                assign(connector, connection_module); mux_on(connectorPairMuxTable[j].muxId); if (sufficientPower(connector)) return;
                if (static_cast<uint8_t>(subPeer) % 2 == 0) {
                    if (assign(connector, connection_module - 2)) relay_on(connection_module, connection_module - 2);
                    if (sufficientPower(connector)) return;
                    if (assign(connector, connection_module - 4)) relay_on(connection_module, connection_module - 4);
                    if (sufficientPower(connector)) return;
                    if (assign(connector, connection_module - 6)) relay_on(connection_module, connection_module - 6);
                    if (sufficientPower(connector)) return;
                }
                else {
                    if (assign(connector, connection_module + 2)) relay_on(connection_module, connection_module + 2);
                    if (sufficientPower(connector)) return;
                    if (assign(connector, connection_module + 4)) relay_on(connection_module, connection_module + 4);
                    if (sufficientPower(connector)) return;
                    if (assign(connector, connection_module + 6)) relay_on(connection_module, connection_module + 6);
                    if (sufficientPower(connector)) return;
                }
                //mux_on(connectorPairMuxTable[j].muxId);
                break;
            }
        }
    }
}

void assign_extra_modules(ConnectorType connector) {

    std::cout << "\nAssigning extra modules for connector: " << static_cast<int>(connector) << "\n";
    int connectorIndex = static_cast<int>(connector);
    if (connectorArray[connectorIndex].isActive == false) return;
    if (sufficientPower(connector) == true) return;

    uint16_t allMuxArray[4] = { 0, 0, 0, 0 };

    getAllMuxes(connector, allMuxArray);

    for (uint8_t i = 0; i < 4; i++) {
        uint16_t muxId = allMuxArray[i];
        if (muxId == 0) continue;
        if (muxStatus(muxId) == true) continue;

        //Normal Mux
        if (muxId > 300 && muxId < 400) {
            if (muxStatus(muxId + (muxId % 2 ? 1 : -1))) continue; // checking peer mux status - continue if ON
            for (uint8_t i = 0; i < connectorMuxCount; i++) {
                if (connectorPairMuxTable[i].muxId == muxId) {
                    ConnectorType peer = (connectorPairMuxTable[i].connectorA == connector) ? connectorPairMuxTable[i].connectorB : connectorPairMuxTable[i].connectorA;
                    uint16_t connection_module = defaultModule(peer);
                    if (!connectorArray[static_cast<int>(peer)].isActive && pmArray[connection_module].isAlive && !moduleStatus(connection_module)) {
                        assign(connector, connection_module); mux_on(muxId);
                        return; // return after assigning one module
                    }
                }
            }
        }

        //Super Mux
        if (muxId > 400) {
            for (uint8_t i = 0; i < connectorMuxCount; i++) {
                if (connectorPairMuxTable[i].muxId == muxId) {
                    ConnectorType peer = (connectorPairMuxTable[i].connectorA == connector) ? connectorPairMuxTable[i].connectorB : connectorPairMuxTable[i].connectorA;
                    uint16_t connection_module = defaultModule(peer);
                    if (!connectorArray[static_cast<int>(peer)].isActive && pmArray[connection_module].isAlive && !moduleStatus(connection_module)) {
                        assign(connector, connection_module); mux_on(muxId);
                        return;
                    }
                }

            }

        }
    }

}



void isolateModule(uint16_t module) {

    std::cout << "\nIsolating module: " << module << "\n";

    if (module > 0 && module < 49) {
        pmArray[module].Connector = ConnectorType::DEFAULT;
        pmArray[module].isActive = false;

        pmArray[module + 1].Connector = ConnectorType::DEFAULT;
        pmArray[module + 1].isActive = false;

        std::cout << "Module " << module << " and " << module + 1 << " isolated.\n";
    }
    else {
        std::cerr << "Invalid module index: " << module << std::endl;
    }


    //pmArray[module].Connector = ConnectorType::DEFAULT;
    //pmArray[module].isActive = false;

    //switchOff relays
    for (uint8_t i = 0; i < relayMuxCount; i++) {
        if (relayMuxTable[i].pmA == module || relayMuxTable[i].pmB == module) {
            relayMuxTable[i].status = false;
            //send command to switch off relay
        }
    }
}

void isolateConnector(ConnectorType connector) {
    //TODO : when isolating entire subset. check for order
    //TODO : implement isolation logic for connector subsets or supersets as whole.

    std::cout << "\nIsolating connector: " << static_cast<int>(connector) << "\n";

    if (connectorArray[static_cast<int>(connector)].isActive == true) {
        std::cerr << "Connector is active. Cannot isolate!\n USE stopConnector() function.\n";
    }

    printModuleStatus();

    //check if default module is already assigned
    uint16_t defaultModuleId = defaultModule(connector);
    if (moduleStatus(defaultModuleId) == false && isMuxIsolation(connector)) return;

    //check if this if is needed
    if (moduleStatus(defaultModuleId)) {
        //Already Assigned
        ConnectorType active_connector = getActiveConnector(defaultModuleId);
        uint16_t activeMuxes[2] = { 0, 0 };

        getActiveMuxes(connector, activeMuxes);
        uint16_t muxId = muxExistence(connector, active_connector);
        //case 1 : if all muxes are off. direct isolate default module
        std::cout << "TEST : " << activeMuxes[0] << ", " << activeMuxes[1] << "\n";
        if (activeMuxes[0] == 0 && activeMuxes[1] == 0) {
            std::cout << "CASE 1 : " << activeMuxes[0] << ", " << activeMuxes[1];
            isolateModule(defaultModuleId);
        }

        //case 2 : if there is direct mux connection btw connector and active connector,
        else if (muxId != 0 && muxStatus(muxId)) {
            std::cout << "CASE 2";
            if (activeMuxes[0] != 0 && activeMuxes[1] != 0) {
                // No zeros exist, both Muxes are active
                //case 2.1 : if both muxes are active, isolate all modules of superset
                uint16_t i = supersetModuleBegin(superset(connector));
                for (; i < supersetModuleBegin(superset(connector)) + 16; i++) {
                    if (getActiveConnector(i) == active_connector) {
                        isolateModule(i);
                    }
                }
            }
            else {
                //case 2.2 : if only one mux is active, isolate all modules of subset
                uint16_t i = subsetModuleBegin(subset(connector));
                for (; i < subsetModuleBegin(subset(connector)) + 8; i++) {
                    if (getActiveConnector(i) == active_connector) {
                        isolateModule(i);
                    }
                }
            }
        }

        //case 3 : if direct mux connection not exists: isolate all modules of subset
        else if (muxId == 0) {
            std::cout << "CASE 3";
            uint16_t i = subsetModuleBegin(subset(connector));
            for (; i < subsetModuleBegin(subset(connector)) + 8; i++) {
                if (getActiveConnector(i) == active_connector) {
                    isolateModule(i);
                }
            }
        }
        std::cout << "\nAfter Isolation:\n";
        allMuxesOff(connector);
        printModuleStatus();
        return;
    }
    else {
        //check active muxes - if exists error
        std::cout << "Already Isolated!";
        return;
    }

}

void stopConnector(ConnectorType connector) {

    if (connectorArray[static_cast<int>(connector)].isActive == false) return;

    for (int i = 1; i < 49; i++) {
        if (pmArray[i].Connector == connector) {
            //Power Down first
        }
    }

    for (int i = 1; i < 49; i++) {
        if (pmArray[i].Connector == connector) {
            isolateModule(i);
        }
    }

    uint16_t activeMuxes[2] = { 0,0 };
    getActiveMuxes(connector, activeMuxes);
    for (uint8_t i = 0; i < 2; i++) {
        if (activeMuxes[i] == 0) continue;
        for (uint8_t j = 0; j < connectorMuxCount; j++) {
            if (connectorPairMuxTable[j].muxId == activeMuxes[i]) {
                ConnectorType peer = (connectorPairMuxTable[j].connectorA == connector) ? connectorPairMuxTable[j].connectorB : connectorPairMuxTable[j].connectorA;
                isolateConnector(peer);
                connectorPairMuxTable[j].status = false; // mux_off(activeMuxes[i]);
            }
        }

    }
    connectorArray[static_cast<int>(connector)].isActive = false;
}

void printModuleStatus() {
    std::cout << "\n ****** Module Status (1-48) ****** \n";
    // Print column headers
    std::cout << "\n      ";  // padding for row labels
    for (int col = 1; col <= 8; ++col) {
        std::cout << "M" << col << " ";
    }
    std::cout << "\n";

    // Print rows
    for (int row = 1; row <= 6; ++row) {  // 48 items / 8 cols = 6 rows
        std::cout << "SS" << row << " | ";
        for (int col = 1; col <= 8; ++col) {
            int index = (row - 1) * 8 + col;
            std::cout << (pmArray[index].isActive ? " 1 " : " 0 ");
        }
        std::cout << " | " << (row - 1) * 8 + 1 << " - " << (row - 1) * 8 + 8 << "\n";
    }
    printMuxStatus();
    printRelayStatus();
    std::cout << "\n ****** ************** ****** \n";
}

void printMuxStatus() {
    std::cout << "\nMux Status:\n";
    std::cout << "301  302  303  304  305  306  307  308  309  310  311  312  401  402  403\n";
    for (uint8_t i = 0; i < connectorMuxCount; i++) {
        std::cout << (connectorPairMuxTable[i].status ? " 1   " : " 0   ");
    }
}

void printRelayStatus() {
    //print as 3 x 6 grid
    std::cout << "\nRelay Status:\n";
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 6; ++col) {
            int index = row * 6 + col;
            std::cout << (relayMuxTable[index].status ? " 1 " : " 0 ");
        }
        std::cout << "\n";
    }
}

uint16_t hasSpareModules(ConnectorType connector) {

    int connectorIndex = static_cast<int>(connector);

    int baseModuleCurrent = 33.5;
    int extraCurrent = connectorArray[connectorIndex].EVSEMaxCurrent - connectorArray[connectorIndex].EVMaxCurrent;

    return (extraCurrent <= 0) ? 0 : extraCurrent / (2 * baseModuleCurrent); // 0 or no of extra modules

}

bool hasFreeModules(uint8_t subsetId) {
    for (int i = subsetId * 8 - 7; i < subsetId * 8; i++) {
        if (!pmArray[i].isActive) {
            return true;
        }
    }
    return false;
}

bool preference(ConnectorType connectorA, ConnectorType connectorB) { //TODO: Works fine now . make it robust

    //checking Default
    if (connectorA == ConnectorType::DEFAULT && connectorB == ConnectorType::DEFAULT) throw std::runtime_error("Both connectors are default");
    else if (connectorA == ConnectorType::DEFAULT) return false;
    else if (connectorB == ConnectorType::DEFAULT) return true;

    //checking power
    if (sufficientPower(connectorA) && sufficientPower(connectorB)) throw std::runtime_error("Both connectors have sufficient power");
    else if (sufficientPower(connectorA) && !sufficientPower(connectorB)) return false;
    else if (!sufficientPower(connectorA) && sufficientPower(connectorB)) return true;

    // Implement your preference logic here
    //if connectorA has higher priority than connectorB -> return true; else return false
    return true; // Placeholder
}


void opt_removeModules(ConnectorType connector, int num = -1) {

    // will remove END modules only

    int connectorIndex = static_cast<int>(connector);
    std::cout << "\nRemoving modules from connector: " << connectorIndex << " , Status :" << connectorArray[connectorIndex].isActive << "\n";

    if (connectorArray[connectorIndex].isActive == false) return;

    if (num == -1)
    {
        if (!extraPower(connector)) return;
        int num = extraPower(connector) / 60;
    }

    uint16_t defaultModuleId = defaultModule(connector);

    for (int i = 1; i < 49; i++) {

        if (i % 2 == 0) continue; //secondary modules
        if (pmArray[i].isAlive == false) continue; //not alive
        if (pmArray[i].isActive == false) continue; //not active
        if (pmArray[i].Connector != connector) continue; // not connected to this connector 
        if (i == defaultModuleId) continue; //default module - not removable

        uint16_t relayIds[2];
        getRelay(i, relayIds);

        //Remove End PM (Has two relays & one of the is off)
        if (relayIds[0] != 0 && relayIds[1] != 0) {
            if (relayStatus(relayIds[0]) == false || relayStatus(relayIds[1]) == false) {
                isolateModule(i);

                num--;
                if (num == 0) break;
            }
        }
        //PMs with 1 relay (has direct connection to connector)
        else if (relayIds[0] == 0 || relayIds[1] == 0) { //actually relayIds[1] will be zero

            std::cout << "\nSingle Relay PM: " << i << "\n";

            ConnectorType defaultConnector = getdefaultConnector(i);

            //if relay = ON, MuxIsolation = true -> powering only through relay -> Isolate module
            if (relayStatus(relayIds[0]) == true && isMuxIsolation(defaultConnector) == true) {
                isolateModule(i);
                num--;
                if (num == 0) break;
            }

            //if relay = OFF, MuxIsolation = false -> powering through mux -> Isolate connector(shutsdown PM and all connector muxes)
            //TODO : test isolate connector works fine here
            else if (relayStatus(relayIds[0]) == false && isMuxIsolation(defaultConnector) == false) {

                uint16_t activeMuxes[2] = { 0, 0 };
                getActiveMuxes(defaultConnector, activeMuxes);
                // Only 1 mux should be active to be END module - alredy checked mux isolation
                if (activeMuxes[0] == 0 || activeMuxes[1] == 0) {
                    isolateModule(i);
                    isolateConnector(defaultConnector); // isolates connector as well as powermodules
                    num--;
                    if (num == 0) break;
                }
            }
        }
        else {
            std::cerr << "Error :: invalid relay ids for module: " << i << std::endl;
        }
    }
}

void opt_assignModules(int iteration) {
    for (int i = 1; i < 49; i++) {
        if (i % 2 == 0) continue; //secondary modules - TODO: modify for primary not alive
        if (pmArray[i].isAlive == false) continue; //not alive
        if (pmArray[i].isActive) continue; //already active

        uint16_t relayIds[2];
        getRelay(i, relayIds);
        if (getdefaultConnector(i) == ConnectorType::DEFAULT) {
            // Middle modules
            if (relayIds[0] != 0 && relayIds[1] != 0) {

                uint16_t moduleA = i - 2; ConnectorType connectorA = pmArray[moduleA].Connector;
                uint16_t moduleB = i + 2; ConnectorType connectorB = pmArray[moduleB].Connector;

                if (connectorA == ConnectorType::DEFAULT && connectorB == ConnectorType::DEFAULT) continue; // No active adjacent powerModules
                if (sufficientPower(connectorA) && sufficientPower(connectorB)) continue; // Both connectors have sufficient power


                if (preference(connectorA, connectorB)) {
                    if (sufficientPower(connectorA)) continue;
                    if (assign(connectorA, i)) relay_on(i - 2, i);
                }
                else {
                    if (sufficientPower(connectorB)) continue;
                    if (assign(connectorB, i)) relay_on(i, i + 2);
                }
            }
        }
        else {
            ConnectorType defaultConnector = getdefaultConnector(i);
            if (connectorArray[static_cast<int>(defaultConnector)].isActive == true) continue;

            uint16_t moduleA = ((i - 1) % 8 == 0) ? i + 2 : i - 2;
            ConnectorType ConnectorA = pmArray[moduleA].Connector;
            if (ConnectorA != ConnectorType::DEFAULT && !sufficientPower(ConnectorA)) {
                if (assign(ConnectorA, i)) relay_on(moduleA, i);
            }
        }


    }
}


//Threads - for simulator

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

// ---- Global Control Flags ----
std::atomic<bool> running{ true };
std::mutex mtx;
std::condition_variable cv;
bool workerRunning = false;   // worker is in opt_remove/opt_assign
bool workerSleeping = false;  // worker finished and is in sleep


void runTriggerActions(json& trig) {
    bool modified = false;  // track if we changed anything

    for (auto& kv : trig.items()) {
        auto& key = kv.key();
        auto& val = kv.value();

        std::string action = val.value("action", "none");
        int voltage = val.value("EVMaxVoltage", 0);
        int current = val.value("EVMaxCurrent", 0);

        std::cout << "[Trigger] " << key
            << " action=" << action
            << " V=" << voltage
            << " I=" << current << "\n";

        if (action == "none") continue;

        if (action == "start") {
            //ConnectorType conn = static_cast<ConnectorType>(std::stoi(key.substr(9))); // "connectorX"
            ConnectorType conn = stringToConnector(key);
            connectorArray[static_cast<int>(conn)].EVMaxVoltage = voltage;
            connectorArray[static_cast<int>(conn)].EVMaxCurrent = current;
            std::cout << "Key is " << key << ", conn=" << static_cast<int>(conn) << "\n";
            std::cout << "Assigning PM to connector " << static_cast<int>(conn) << "\n";
            assign_power_modules(conn);
        }
        else if (action == "stop") {
            ConnectorType conn = stringToConnector(key);
            stopConnector(conn);
            connectorArray[static_cast<int>(conn)].EVMaxVoltage = 0;
            connectorArray[static_cast<int>(conn)].EVMaxCurrent = 0;
        }
        else if (action == "update") {
            ConnectorType conn = stringToConnector(key);
            connectorArray[static_cast<int>(conn)].EVMaxVoltage = voltage;
            connectorArray[static_cast<int>(conn)].EVMaxCurrent = current;

        }



        // TODO: call assign_power_modules2(), opt_removeModules(), etc.
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Reset action after handling
        val["action"] = "none";
        modified = true;
    }

    // If we modified anything, write back to file
    if (modified) {
        std::ofstream out("json_data/trigger.json");
        if (out) {
            out << std::setw(4) << trig;  // pretty print with 4 spaces
        }
    }

    //saving to json file
    saveConnectorArrayToJson("json_data/connectors.json");
    createModuleStatusJson("json_data/modules.json");
    createMuxRelayJson("json_data/mux.json");
    createConnectorModuleJson("json_data/connector_modules.json");
}


// ---- Worker Thread ----
void workerLoop() {
    while (running) {

        std::cout << "[Worker] Starting optimization cycle...\n";

        {
            std::unique_lock<std::mutex> lock(mtx);
            workerRunning = true;
            workerSleeping = false;
        }

        // Do work
        printModuleStatus();
        for (int i = 1; i <= 12; i++) {
            std::cout << "[Worker] removing Extra Modules from Connector " << i << "...\n";
            opt_removeModules(static_cast<ConnectorType>(i));
        }
        printModuleStatus();


        for (int i = 1; i <= 3; i++) {
            std::cout << "[Worker] assigning Modules : Iteration  " << i << "...\n";
            opt_assignModules(i);
        }

        for (int i = 1; i <= 12; i++) {
            std::cout << "[Worker] removing Extra Modules from Connector " << i << "...\n";
            assign_extra_modules(static_cast<ConnectorType>(i));
        }

        //saving to json file
        saveConnectorArrayToJson("json_data/connectors.json");
        createModuleStatusJson("json_data/modules.json");
        createMuxRelayJson("json_data/mux.json");
        createConnectorModuleJson("json_data/connector_modules.json");

        {
            std::unique_lock<std::mutex> lock(mtx);
            workerRunning = false;
            workerSleeping = true;
            cv.notify_all(); // notify trigger that worker is sleeping
        }

        // Sleep for 60s (can be interrupted by running flag)
        for (int i = 0; i < 20 && running; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

// ---- Trigger Thread ----
void triggerListener() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // poll every 5s

        std::cout << "[Trigger] Checking for trigger actions...\n";

        std::ifstream in("json_data/trigger.json");
        if (!in) continue;

        json trig;
        try {
            in >> trig;
        }
        catch (...) {
            continue; // skip malformed json
        }

        bool hasWork = false;
        for (auto& [key, val] : trig.items()) {
            if (val.value("action", "none") != "none") {
                hasWork = true;
                break;
            }
        }
        if (!hasWork) continue;

        std::unique_lock<std::mutex> lock(mtx);

        // If worker is running, wait until it sleeps
        cv.wait(lock, [] { return workerSleeping; });

        std::cout << "[Trigger] Worker is asleep, running trigger...\n";
        runTriggerActions(trig);

        std::cout << "[Trigger] Finished actions.\n";
    }
}

// ---- Main ----
int main() {
    std::thread tWorker(workerLoop);
    std::thread tTrigger(triggerListener);

    // Let it run for demo
    std::this_thread::sleep_for(std::chrono::seconds(600));// 10 minutes
    running = false;

    tWorker.join();
    tTrigger.join();

    std::cout << "Program exiting.\n";
    return 0;
}