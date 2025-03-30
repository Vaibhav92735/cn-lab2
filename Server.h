// B22CS058-B22CS088

#ifndef SERVER_H_
#define SERVER_H_

#include <omnetpp.h>
#include <string>
#include <vector>
using namespace omnetpp;
using namespace std;

class Server : public cSimpleModule
{
protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
};

#endif /* SERVER_H_ */
