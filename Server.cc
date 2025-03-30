// B22CS058-B22CS088

#include "Server.h"
#include <bits/stdc++.h>
#include <omnetpp.h>

using namespace std;
using namespace omnetpp;

// Renamed message class to reflect its purpose
class SrvClientResponse : public cMessage {
public:
    int answer;
    int subtaskID;
    int subtaskCount;
    int taskType;
};

Define_Module(Server);

// Helper: Returns the current time stamp with milliseconds using strftime.
string getTimestamp() {
    auto now = chrono::system_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
    time_t now_time = chrono::system_clock::to_time_t(now);
    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now_time));
    stringstream ss;
    ss << timeStr << '.' << setfill('0') << setw(3) << ms.count();
    return ss.str();
}

// Helper: Logs a message to "serverOutput.txt" along with a timestamp.
void logServerEvent(const string& msg) {
    string fullMsg = "[" + getTimestamp() + "] " + msg;
    EV << fullMsg << "\n";
    cout << fullMsg << "\n";
    ofstream logFile("serverOutput.txt", ios::app);
    if (logFile.is_open()) {
        logFile << fullMsg << "\n";
        logFile.close();
    } else {
        cerr << "Error: Cannot open serverOutput.txt for writing!" << "\n";
    }
}

void Server::initialize() {
    int serverID = getIndex();
    string outputFile = "serverOutput.txt";
    
    // For server 0, clear the log file at the start of the simulation.
    if (serverID == 0) {
        ofstream ofs(outputFile, ios::trunc);
        if (ofs.is_open()) {
            ofs << "=== Server Output Log ===" << "\n";
            ofs << "Simulation started at: " << simTime() << "\n";
            ofs << "===========================" << "\n";
            ofs.close();
            EV << "Initialized log file: " << outputFile << "\n";
        } else {
            EV << "Error: Unable to initialize log file: " << outputFile << "\n";
        }
    }
    
    // Log the initialization of this server
    string initMsg = "Server " + to_string(serverID) + " has been initialized.";
    EV << initMsg << "\n";
    logServerEvent(initMsg);
}

void Server::handleMessage(cMessage *msg) {
    int numServers = par("totalServers").intValue();
    int serverID = getIndex();
    
    // Use a random engine for any needed randomness.
    random_device rd;
    mt19937 rng(rd());
    
    // Extract integer values from the message name.
    string msgContent = msg->getName();
    stringstream ss(msgContent);
    vector<int> values;
    int value;
    while (ss >> value) {
        values.push_back(value);
    }
    
    // Compute the maximum element using standard algorithm.
    int maxValue = (values.empty()) ? INT_MIN : *max_element(values.begin(), values.end());
    
    // Determine subtask identifier from the message kind.
    int subID = msg->getKind();
    
    // Create a response message.
    SrvClientResponse *response = new SrvClientResponse();
    response->subtaskID = subID;
    response->subtaskCount = subID; // In this design, we use the same value.
    response->taskType = (subID >= 100) ? 2 : 1;
    
    // Alter malicious behavior: if (serverID + subID) is divisible by 3 then misbehave.
    bool maliciousBehavior = (((serverID + subID) % 3) == 0);
    
    if (maliciousBehavior) {
        // For malicious nodes, report a deliberately lowered maximum.
        response->answer = maxValue - 10;
        string malLog = "Malicious Server " + to_string(serverID) + " sending incorrect value " +
                        to_string(response->answer) + " for subtask [" + msgContent +
                        "] (subtaskID: " + to_string(subID) + ")";
        logServerEvent(malLog);
    }
    else {
        // Honest behavior returns the correct maximum.
        response->answer = maxValue;
        string honestLog = "Honest Server " + to_string(serverID) + " sending correct value " +
                           to_string(response->answer) + " for subtask [" + msgContent +
                           "] (subtaskID: " + to_string(subID) + ")";
        logServerEvent(honestLog);
    }

    // Send the response out through all available gates.
    int numOutGates = gateSize("out");
    for (int i = 0; i < numOutGates; i++) {
        send(response->dup(), "out", i);
    }

    delete response;
    delete msg;
}
