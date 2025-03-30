// B22CS058-B22CS088

#ifndef CLIENT_H_
#define CLIENT_H_

#include <omnetpp.h>
#include <bits/stdc++.h>
using namespace omnetpp;
using namespace std;

class gossipMessage:public cMessage
{
     public:
         string s;
};

class ClientMessage:public cMessage
{
     public:
         vector<int> arr;
         int arr_len;
         int subtask_id;
         int subtask_num;
         int task_num;
         int ID;
         time_t time;
};

class Client : public cSimpleModule
{
    public:
        vector<int> res;
        int server_mutex = 0;
        
        void send_message(std::vector<int> arr, int arr_len, int server_id, int subtask_id, int subtask_num)
        {
            ClientMessage *newm= new ClientMessage();
            newm->arr=arr;
            newm->arr_len=arr_len;
            newm->subtask_id=subtask_id;
            newm->subtask_num=subtask_num;
            send(newm, "out", server_id);
        }
        
        void send_message(vector<int> arr, time_t currentTime, int ID, int size, int dest) {
            ClientMessage *newMsg = new ClientMessage();
            newMsg->arr = arr;
            newMsg->ID = ID;
            newMsg->time = currentTime;
            newMsg->arr_len = size;
            send(newMsg, "out", dest);
        }

    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;
};

#endif /* CLIENT_H_ */
