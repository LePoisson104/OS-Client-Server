#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <list>
#include <math.h>
#include <iterator>
#include <cstring>
#include <netinet/in.h>
#include <sys/wait.h>

void fireman(int)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

int main(int argc, char *argv[])
{
    // reference from lecture server.cpp sample code
    int sfd, newsfd, portNo, clientSize, probMessageSize, sumMessageSize, n;
    struct sockaddr_in serverAddr, clientAddr;

    // create a socket
    if (argc < 2)
    {
        std::cerr << "ERROR, no port provided\n";
        exit(1);
    }
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0)
    {
        std::cerr << "Can't create a socket!";
        exit(1);
    }
    // bind the socket to a port
    bzero((char *)&serverAddr, sizeof(serverAddr));
    portNo = atoi(argv[1]);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(portNo);

    signal(SIGCHLD, fireman);

    if (bind(sfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "can't bind to port";
        exit(1);
    }
    // listen
    if (listen(sfd, SOMAXCONN) < 0)
    {
        std::cerr << "can't listen!";
        exit(1);
    }
    // accept the connection from client
    clientSize = sizeof(clientAddr);
    while (true)
    {
        newsfd = accept(sfd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientSize);
        if (newsfd < 0)
        {
            std::cerr << "failed to accept!" << std::endl;
            exit(1);
        }
        if (fork() == 0)
        {
            int resultSize;
            char *resultChar = new char[resultSize + 1];

            n = read(newsfd, &probMessageSize, sizeof(int));
            if (n < 0)
            {
                std::cerr << "failed to read message" << std::endl;
                exit(1);
            }

            char *needToCal = new char[probMessageSize + 1]; // message from client
            std::string probInput(needToCal);

            n = read(newsfd, needToCal, probMessageSize);
            if (n < 0)
            {
                std::cerr << "failed to read message" << std::endl;
                exit(1);
            }
            std::string needToCalStr(needToCal);
            float probVal = std::stof(needToCalStr);
            n = read(newsfd, &sumMessageSize, sizeof(int));
            if (n < 0)
            {
                std::cerr << "failed to read message" << std::endl;
                exit(1);
            }
            char *sum = new char[sumMessageSize + 1];
            n = read(newsfd, sum, sumMessageSize);
            if (n < 0)
            {
                std::cerr << "failed to read message" << std::endl;
                exit(1);
            }
            std::string sumStr(sum);
            float sumVal = std::stof(sumStr);
            float cumulative = sumVal + probVal * .5;
            std::string result = std::to_string(cumulative);

            std::string toBinary = "";

            int binaryLength = ceil(log2(1 / probVal)) + 1; // calculate the length of the binary code

            for (int i = 0; i < binaryLength; i++)
            {                    // loop through how many time it will calcuate the binary code according to the length that was calculated
                cumulative *= 2; // multiply the probVal with 2
                if (cumulative < 1)
                { // if less than 1 then binary value is 0
                    toBinary += "0";
                }
                else
                {
                    cumulative -= 1; // else if probArray *= 2 is greater than 1, then binary value is 1
                    toBinary += "1";
                }
            }
            resultSize = toBinary.length();
            strcpy(resultChar, toBinary.c_str());
            n = write(newsfd, &resultSize, sizeof(int));
            n = write(newsfd, resultChar, resultSize);
            _exit(0);
        }
    }
    wait(nullptr);

    // close socket
    close(sfd);
    return 0;
}