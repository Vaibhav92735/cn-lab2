// B22CS058-B22CS088

#include <omnetpp.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
using namespace omnetpp;
using namespace std;

class Net : public cSimpleModule {
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(Net);

void Net::initialize() {
    // Read the topology file name from the parameter.
    const char *topoFilename = par("topoFile").stringValue().c_str();
    EV << "Attempting to open topology file: " << topoFilename << "\n";
    cout << "Attempting to open topology file: " << topoFilename << "\n";

    // Default values.
    int numServers = 0, numClients = 0;
    simtime_t serverToClientDelay = 10e-3, clientToServerDelay = 1, clientToClientDelay = 10e-3;

    // Open and parse the topology file.
    ifstream infile(topoFilename);
    if (!infile) {
        EV << "ERROR: Cannot open topology file " << topoFilename << "\n";
        cout << "ERROR: Cannot open topology file " << topoFilename << "\n";
        endSimulation();
        return;
    }
    string line;
    while (std::getline(infile, line)) {
        // Skip empty lines or comments
        if (line.empty() || line[0] == '#')
            continue;
        istringstream iss(line);
        string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            if (key == "numServers")
                numServers = std::stoi(value);
            else if (key == "numClients")
                numClients = std::stoi(value);
            else if (key == "serverToClientDelay")
                serverToClientDelay = SimTime(value.c_str());
            else if (key == "clientToServerDelay")
                clientToServerDelay = SimTime(value.c_str());
            else if (key == "clientToClientDelay")
                clientToClientDelay = SimTime(value.c_str());
        }
    }
    infile.close();

    // Log the parsed topology parameters
    EV << "Topology file " << topoFilename << " read successfully.\n";
    EV << "numServers = " << numServers << ", numClients = " << numClients << "\n";
    EV << "serverToClientDelay = " << serverToClientDelay << ", clientToServerDelay = " << clientToServerDelay
       << ", clientToClientDelay = " << clientToClientDelay << "\n";
    cout << "Topology file " << topoFilename << " read successfully." << endl;
    cout << "numServers = " << numServers << ", numClients = " << numClients << endl;
    cout << "serverToClientDelay = " << serverToClientDelay << ", clientToServerDelay = " << clientToServerDelay
         << ", clientToClientDelay = " << clientToClientDelay << endl;

    // Check that we have non-zero numbers for servers and clients
    if (numServers <= 0 || numClients <= 0) {
        EV << "ERROR: Invalid number of servers or clients from topo file.\n";
        cout << "ERROR: Invalid number of servers or clients from topo file." << endl;
        endSimulation();
        return;
    }

    // Dynamically create server modules.
    cModuleType *serverType = cModuleType::get("Server");
    vector<cModule*> servers;
    for (int i = 0; i < numServers; i++) {
        string name = "s" + std::to_string(i);
        cModule *server = serverType->create(name.c_str(), this);
        server->par("totalServers").setIntValue(numServers);
        server->par("totalClients").setIntValue(numClients);
        server->finalizeParameters();
        server->buildInside();
        server->scheduleStart(simTime());
        servers.push_back(server);
    }

    // Dynamically create client modules.
    cModuleType *clientType = cModuleType::get("Client");
    vector<cModule*> clients;
    for (int i = 0; i < numClients; i++) {
        string name = "c" + std::to_string(i);
        cModule *client = clientType->create(name.c_str(), this);
        client->par("totalServers").setIntValue(numServers);
        client->par("totalClients").setIntValue(numClients);
        client->finalizeParameters();
        client->buildInside();
        client->scheduleStart(simTime());
        clients.push_back(client);
    }

    // Create connections between servers and clients.
    // 1. Connect each server's "out" gate to each client's "in" gate.
    for (int i = 0; i < numServers; i++) {
        for (int j = 0; j < numClients; j++) {
            // Increase gate vector sizes as needed.
            int sOutIndex = servers[i]->gateSize("out");
            servers[i]->setGateSize("out", sOutIndex + 1);
            int cInIndex = clients[j]->gateSize("in");
            clients[j]->setGateSize("in", cInIndex + 1);

            cGate *sOutGate = servers[i]->gate("out", sOutIndex);
            cGate *cInGate = clients[j]->gate("in", cInIndex);
            sOutGate->connectTo(cInGate)->setDelay(serverToClientDelay);
        }
    }

    // 2. Connect each client's "out" gate to each server's "in" gate.
    for (int j = 0; j < numClients; j++) {
        for (int i = 0; i < numServers; i++) {
            int cOutIndex = clients[j]->gateSize("out");
            clients[j]->setGateSize("out", cOutIndex + 1);
            int sInIndex = servers[i]->gateSize("in");
            servers[i]->setGateSize("in", sInIndex + 1);

            cGate *cOutGate = clients[j]->gate("out", cOutIndex);
            cGate *sInGate = servers[i]->gate("in", sInIndex);
            cOutGate->connectTo(sInGate)->setDelay(clientToServerDelay);
        }
    }

    // 3. Create gossip connections among clients (each client to every other client).
    for (int i = 0; i < numClients; i++) {
        for (int j = 0; j < numClients; j++) {
            if (i == j)
                continue;
            int cOutIndex = clients[i]->gateSize("out");
            clients[i]->setGateSize("out", cOutIndex + 1);
            int cInIndex = clients[j]->gateSize("in");
            clients[j]->setGateSize("in", cInIndex + 1);

            cGate *fromGate = clients[i]->gate("out", cOutIndex);
            cGate *toGate = clients[j]->gate("in", cInIndex);
            fromGate->connectTo(toGate)->setDelay(clientToClientDelay);
        }
    }
}

void Net::handleMessage(cMessage *msg) {
    // This module does not expect to receive messages.
    delete msg;
}
