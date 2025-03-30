# Remote Task Execution Simulation using OMNeT++

## Overview

This simulation uses the OMNeT++ Discrete Event Simulator to mimic remote program execution. Each job (task) is divided into *n* subtasks that are dispatched to *(n/2 + 1)* servers. The result determined by the majority is then accepted as the correct outcome.

In this implementation, the server is responsible for a simple computational task: finding the maximum element in an array of integers. The client divides a given array into roughly *n* parts (ensuring each subarray contains at least two elements) and sends each subarray as a subtask to a selected subset of servers.

## Implementation Details

### Network Architecture

- **Modules:**  
  The simulation consists of two types of modules: **Client** and **Server**.  
  - **Server:** Each server receives a subtask, computes the maximum element, and returns the result. Servers may sometimes act maliciously by deliberately sending an incorrect result.
  - **Client:** The client divides the main task into subtasks, dispatches these to a subset of servers (selected randomly in the first round and based on scores in subsequent rounds), aggregates the server responses using majority voting, and then uses a gossip protocol to exchange server performance scores with other clients.

- **Dynamic Topology:**  
  The network topology can be defined in an external file (e.g., `topo.txt`), which specifies the number of clients and servers and their interconnections. This file is parsed during initialization, allowing the simulation to dynamically adapt to different network instances.

### Code Overview

1. **Server.cc:**  
   - **Task Execution:** The server extracts an integer array from the incoming message, computes the maximum element (using a standard algorithm), and decides whether to behave honestly or maliciously based on a configurable rule.  
   - **Logging:** The server logs each computed result along with timestamps and node IDs to both the console and an output file (`serverOutput.txt`).

2. **Client.cc:**  
   - **Task Splitting:** The client splits the input array into *n* subtasks using a round-robin method, ensuring each subtask meets the minimum size requirement.  
   - **Subtask Dispatching:** In the first round, subtasks are sent to randomly selected servers (using a selection of *(n/2 + 1)* servers). In later rounds, servers are chosen based on their performance scores, which are calculated from the previous task execution.
   - **Aggregation and Majority Voting:** After receiving responses from the servers, the client aggregates the results, determines the majority value, and updates server scores.
   - **Gossip Protocol:** The client broadcasts its server scores to other clients using a gossip protocol. Each gossip message is logged, and duplicate messages are prevented through a tracking mechanism.

## Execution Steps

1. **Open the Project:**  
   Open the project in the OMNeT++ IDE.

2. **Configure Simulation Parameters:**  
   Set the values for `numServers` and `numClients` in the `omnetpp.ini` file or through the external topology file (`topo.txt`).

3. **Build and Run:**  
   Compile and execute the simulation within the OMNeT++ environment. The simulation output will be displayed on the console and logged into separate files (e.g., `ClientX.txt` for each client and `serverOutput.txt` for servers).

4. **Monitor Logging:**  
   Verify that diagnostic messages, including topology reading and gossip exchanges, are logged to ensure that the simulation is using the provided topology file.
