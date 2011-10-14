#ifndef NETWORK_FRAMEWORK_H_INCLUDED
#define NETWORK_FRAMEWORK_H_INCLUDED


extern char * peer_feed;

int StartupNetworkServer();
int StartupNetworkClient(char * ip,unsigned int port);

#endif // NETWORK_FRAMEWORK_H_INCLUDED
