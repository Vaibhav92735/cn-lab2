
simple Server
{
    parameters:
//        @display("i=block/routing");

        int totalServers;
        int totalClients;
    gates:
        input in[];
        output out[];

}

simple Client
{
    parameters:
        //        @display("i=block/routing");
        int totalServers;
        int totalClients;
    gates:
        input in[];
        output out[];

}

network Net
{
    parameters:
        int numServers;
        int numClients;
        @display("bgb=700,400");
    submodules:
        s[numServers]: Server {
            totalServers = parent.numServers;
            totalClients = parent.numClients;
        }
        c[numClients]: Client {
            totalServers = parent.numServers;
            totalClients = parent.numClients;
        }
    connections:
        for i=0..(numServers-1), for j=0..(numClients-1) {
            s[i].out++ --> {  delay = 10ms; } --> c[j].in++;
            s[i].in++ <--{  delay = 1000ms; } <-- c[j].out++;
        }

        for i=0..(numClients-1), for j=0..(numClients-1) {
            c[i].out++ --> {  delay = 10ms; } --> c[j].in++;
        }
}
