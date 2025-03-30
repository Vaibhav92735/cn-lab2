// B22CS058-B22CS088

#include "Client.h"
#include <bits/stdc++.h>
#include <omnetpp.h>

using namespace std;
using namespace omnetpp;

// Custom message type used between servers and clients.
class SrvClientMsg : public cMessage {
public:
    int result;
    int subtaskId;
    int totalSubtasks;
    int taskIndex;
};

Define_Module(Client);

// Global flags and counters
bool isFirstRun = true;
int globalCounter = 0;
int partIndex = 0;
int kIndex = 0;
vector<int> clientIDs;
vector<int> gateIndices;
map<int, int> clientMapping;
int currentTask = 1;
map<string, bool> seenGossip; // Tracks gossip messages already processed

// Returns the current timestamp as a formatted string with milliseconds.
string currentTimestamp() {
    auto now = chrono::system_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
    time_t now_time = chrono::system_clock::to_time_t(now);
    stringstream timeStream;
    timeStream << put_time(localtime(&now_time), "%Y-%m-%d %H:%M:%S")
               << '.' << setfill('0') << setw(3) << ms.count();
    return timeStream.str();
}

// Logs a message to both the simulation log and a specified file.
// Now each log includes the timestamp from currentTimestamp().
void writeLog(const string& msg, const string& filename) {
    string fullMsg = "[" + currentTimestamp() + "] " + msg;
    EV << fullMsg << "\n";
    cout << fullMsg << "\n";

    ofstream fileStream(filename, ios::app);
    if (fileStream.is_open()) {
        fileStream << fullMsg << endl;
        fileStream.close();
    }
}

// Returns a random selection of unique server indices using a set.
vector<int> getRandomServers(int totalServers, int count) {
    set<int> chosen;
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> dist(0, totalServers - 1);

    while (chosen.size() < static_cast<size_t>(count)) {
        chosen.insert(dist(rng));
    }

    return vector<int>(chosen.begin(), chosen.end());
}

// Distributes array elements among subtasks in a round-robin fashion.
vector<string> splitTaskRoundRobin(const vector<int>& arrayData, int numParts) {
    vector<vector<int>> parts(numParts);
    for (size_t i = 0; i < arrayData.size(); i++) {
        parts[i % numParts].push_back(arrayData[i]);
    }
    
    vector<string> partStrings;
    for (int i = 0; i < numParts; i++) {
        stringstream ss;
        for (int num : parts[i])
            ss << num << " ";
        partStrings.push_back(ss.str());
    }

    return partStrings;
}

// Uses partial sort to get indices of the top k elements.
vector<int> pickTopKIndexes(const vector<int>& values, int k) {
    vector<int> indexes(values.size());
    iota(indexes.begin(), indexes.end(), 0);
    nth_element(indexes.begin(), indexes.begin() + k, indexes.end(),
                [&values](int a, int b) { return values[a] > values[b]; });
    indexes.resize(k);
    return indexes;
}

void Client::initialize() {
    int numServers = par("totalServers").intValue();
    int numClients = par("totalClients").intValue();
    int thisClient = getIndex();

    // Setup log files for this client.
    string logFile = "Client" + to_string(thisClient) + ".txt";
    {
        ofstream outFile(logFile, ios::trunc);
        if (outFile.is_open()) {
            outFile << "=== Log for Client " << thisClient << " ===" << endl;
            outFile << "Start time: " << simTime() << endl;
            outFile << "==============================" << endl;
            outFile.close();
            EV << "Created log file: " << logFile << "\n";
        } else {
            EV << "Error opening log file: " << logFile << "\n";
        }
    }
    
    string gossipFile = "Client" + to_string(thisClient) + "_gossip.txt";
    {
        ofstream gossipStream(gossipFile, ios::trunc);
        if (gossipStream.is_open()) {
            gossipStream << "=== Gossip Log for Client " << thisClient << " ===" << endl;
            gossipStream << "Start time: " << simTime() << endl;
            gossipStream << "==============================" << endl;
            gossipStream.close();
            EV << "Gossip file: " << gossipFile << "created\n";
        } else {
            EV << "Error opening gossip file: " << gossipFile << "\n";
        }
    }
    
    // Prepare the result vector (one entry per server).
    res.resize(numServers, 0);
    
    // Determine how many servers each subtask will contact.
    int serversPerTask = (numServers / 2) + 1;
    
    // Define the array for task 1 (find maximum element).
    vector<int> dataArray = {45, 22, 87, 34, 56, 12, 99, 23, 67, 78};
    
    // Ensure at least 2 elements per server; add random values if needed.
    while (dataArray.size() / numServers < 2) {
        dataArray.push_back(rand() % 100);
    }
    
    // Log task 1 start.
    string initMsg = "Client " + to_string(thisClient) + " starting task 1: Find maximum in array";
    writeLog(initMsg, logFile);

    // Split the task in a round-robin fashion.
    vector<string> subtasks = splitTaskRoundRobin(dataArray, numServers);
    for (int i = 0; i < numServers; i++) {
        string subtaskMsg = "Client " + to_string(thisClient) + " created subtask " + to_string(i) + ": " + subtasks[i];
        writeLog(subtaskMsg, logFile);
        
        // Create and tag the message with subtask id.
        cMessage *subtaskMsgPtr = new cMessage(subtasks[i].c_str());
        subtaskMsgPtr->setKind(i);
        
        // Choose random servers for the subtask.
        vector<int> targetServers = getRandomServers(numServers, serversPerTask);
        for (int serverId : targetServers) {
            string sendMsg = "Client " + to_string(thisClient) + " dispatching subtask " + to_string(i) +
                             " to server " + to_string(serverId);
            writeLog(sendMsg, logFile);
            send(subtaskMsgPtr->dup(), "out", serverId);
        }
        delete subtaskMsgPtr;
    }
}

void Client::handleMessage(cMessage *msg) {
    int numServers = par("totalServers").intValue();
    int numClients = par("totalClients").intValue();
    int thisClient = getIndex();
    
    // Determine message type.
    SrvClientMsg *srvMsg = dynamic_cast<SrvClientMsg *>(msg);
    ClientMessage *clientGossip = dynamic_cast<ClientMessage *>(msg);
    
    if (clientGossip) {
        // Process gossip message from another client.
        vector<int> newScores = clientGossip->arr;
        int originClient = clientGossip->ID;
        time_t rawTime = clientGossip->time;

        char timeBuffer[80];
        tm *timeInfo = localtime(&rawTime);
        strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeInfo);
        string formattedTime = timeBuffer;
        
        string gossipKey = to_string(originClient) + "_" + formattedTime;
        if (seenGossip.find(gossipKey) == seenGossip.end()) {
            seenGossip[gossipKey] = true;
            
            string gossipLog = "GOSSIP [" + currentTimestamp() + "] from Client " + to_string(originClient) +
                               ": Server scores: ";
            for (int score : newScores) {
                gossipLog += to_string(score) + " ";
            }
            writeLog(gossipLog, "Client" + to_string(thisClient) + "_gossip.txt");
            
            // Merge gossip scores.
            if (res.empty())
                res = newScores;
            else {
                for (int i = 0; i < numServers && i < newScores.size(); i++) {
                    res[i] += newScores[i];
                }
            }
            
            // Forward gossip to other clients (simulate delay before forwarding).
            for (int i = 0; i < numClients; i++) {
                if (i != originClient && i != thisClient) {
                    // Simulate a small random delay.
                    this_thread::sleep_for(chrono::milliseconds(rand() % 50 + 10));

                    ClientMessage *forwardMsg = new ClientMessage();
                    forwardMsg->arr = newScores;
                    forwardMsg->ID = originClient;
                    forwardMsg->time = rawTime;
                    forwardMsg->arr_len = newScores.size();
                    send(forwardMsg, "out", numServers + i);
                    
                    string fwdLog = "Client " + to_string(thisClient) +
                                    " forwarding gossip to client " + to_string(i);
                    writeLog(fwdLog, "Client" + to_string(thisClient) + ".txt");
                }
            }
            
            // Transition from task 1 to task 2.
            if (currentTask == 1) {
                currentTask = 2;
                string task2Log = "Client " + to_string(thisClient) +
                                  " starting task 2: Find maximum using trusted servers";
                writeLog(task2Log, "Client" + to_string(thisClient) + ".txt");
                
                // Merge scores into a new normalized vector.
                vector<int> normalizedScore(numServers, 0);
                for (int i = 0; i < numServers; i++) {
                    normalizedScore[i] = newScores[i] + (i < res.size() ? res[i] : 0);
                }
                
                // Task 2 array.
                vector<int> arrayTask2 = {67, 42, 101, 55, 88, 33, 77, 22, 91, 44, 66, 11, 95, 30, 82};
                
                // Select top servers based on normalized scores.
                vector<int> bestServers = pickTopKIndexes(normalizedScore, (numServers / 2) + 1);
                string serverSelectLog = "Client " + to_string(thisClient) + " chose servers for task 2: ";
                for (int srv : bestServers) {
                    serverSelectLog += to_string(srv) + " ";
                }
                writeLog(serverSelectLog, "Client" + to_string(thisClient) + ".txt");
                
                // Use round-robin distribution for task 2 as well.
                vector<string> task2Subtasks = splitTaskRoundRobin(arrayTask2, numServers);
                for (int i = 0; i < numServers; i++) {
                    string subtaskMsg = "Client " + to_string(thisClient) + " created task2 subtask " +
                                        to_string(i) + ": " + task2Subtasks[i];
                    writeLog(subtaskMsg, "Client" + to_string(thisClient) + ".txt");
                    
                    cMessage *task2Msg = new cMessage(task2Subtasks[i].c_str());
                    task2Msg->setKind(i + 100);
                    
                    for (int serverId : bestServers) {
                        string sendMsg = "Client " + to_string(thisClient) + " sending task2 subtask " +
                                         to_string(i) + " to server " + to_string(serverId);
                        writeLog(sendMsg, "Client" + to_string(thisClient) + ".txt");
                        send(task2Msg->dup(), "out", serverId);
                    }
                    delete task2Msg;
                }
            }
        }
    }
    else if (srvMsg) {
        // Handle server response.
        int srcServer = msg->getArrivalGate()->getIndex();
        int valueReceived = srvMsg->result;
        int subId = srvMsg->subtaskId;
        int totalSubId = srvMsg->totalSubtasks;
        
        string recvLog = "Client " + to_string(thisClient) + " got result " + to_string(valueReceived) +
                         " for subtask " + to_string(totalSubId) +
                         " from server " + to_string(srcServer);
        writeLog(recvLog, "Client" + to_string(thisClient) + ".txt");
        
        if (res.empty()) {
            res.resize(numServers, 0);
        }
        res[srcServer]++;
        
        server_mutex++;
        if (server_mutex == numServers * ((numServers / 2) + 1)) {
            server_mutex = 0;
            string consolidatedLog = "Client " + to_string(thisClient) + " consolidated server scores: ";
            for (int score : res) {
                consolidatedLog += to_string(score) + " ";
                EV << score << " ";
            }
            writeLog(consolidatedLog, "Client" + to_string(thisClient) + ".txt");
            
            // Broadcast gossip scores to other clients.
            for (int i = 0; i < numClients; i++) {
                if (i != thisClient) {
                    ClientMessage *gossipBroadcast = new ClientMessage();
                    gossipBroadcast->arr = res;
                    gossipBroadcast->ID = thisClient;
                    gossipBroadcast->time = time(nullptr);
                    gossipBroadcast->arr_len = res.size();
                    send(gossipBroadcast, "out", numServers + i);
                    
                    string broadcastLog = "Client " + to_string(thisClient) +
                                            " broadcasting scores to client " + to_string(i);
                    writeLog(broadcastLog, "Client" + to_string(thisClient) + ".txt");
                }
            }
            EV << "\n" << thisClient << "\n";
        }
    }
    else {
        // Default handling for a generic message.
        int srcServer = msg->getArrivalGate()->getIndex();
        int taskID = msg->getKind();

        string defaultLog = "Client " + to_string(thisClient) + " received message from server " +
                            to_string(srcServer) + ": " + msg->getName();
        writeLog(defaultLog, "Client" + to_string(thisClient) + ".txt");

        if (res.empty()) {
            res.resize(numServers, 0);
        }
        res[srcServer]++;

        string resStr;
        for (int score : res) {
            resStr += to_string(score) + " ";
        }

        cout << "Client " << thisClient << " received " << msg->getName() <<
                " from server " << srcServer << " Scores: " << resStr << "\n";

        string fileName = "Client" + to_string(thisClient) + ".txt";
        ofstream outFile(fileName, ios::app);
        if (outFile.is_open()) {
            outFile << "Client " + to_string(thisClient) + " received " + msg->getName() +
                       " from server " + to_string(srcServer) + " Scores: " + resStr << endl;
            outFile.close();
        }
    }
    
    delete msg;
}
