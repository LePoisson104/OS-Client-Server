#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <math.h>
#include <netinet/in.h>
#include <netdb.h>
#include <list>
#include <iterator>
#include <map>

struct clientStruct
{
    float probVal, sum;
    int portNo;
    std::string resultStr, hostName;
};

std::string findFrequency(std::string findFreq)
{ // find the frequency of the user input
    std::string inputStr;
    std::map<char, int> countChar;
    std::map<char, int>::iterator count;

    for (int i = 0; i < findFreq.length(); i++)
    {
        count = countChar.find(findFreq[i]);
        if (count == countChar.end())
        {
            countChar[findFreq[i]] = 1;
        }
        else
        {
            count->second += 1;
        }
    }
    for (count = countChar.begin(); count != countChar.end(); count++)
    {
        float buffer = (float)count->second / findFreq.length();
        inputStr += std::to_string(buffer) + " ";
    }

    return inputStr;
}

std::string getSymbols(std::string findSym)
{ // get the different symbols in user input

    std::string symbolStr;
    std::list<char> charCheck;
    std::list<char>::iterator findChar;
    for (int i = 0; i < findSym.length(); i++)
    {
        findChar = std::find(charCheck.begin(), charCheck.end(), findSym[i]);
        if (findChar == charCheck.end())
        {
            charCheck.push_back(findSym[i]);
            symbolStr = symbolStr + findSym[i] + " ";
        }
    }
    return symbolStr;
}

void *callToServer(void *clientPtr)
{
    struct clientStruct *cStruct = (struct clientStruct *)clientPtr;
    struct sockaddr_in serverAdrr;

    std::string hostNameLen = cStruct->hostName;
    int hostNameInt = hostNameLen.length();
    char *hostNameChar = new char[hostNameInt];
    strcpy(hostNameChar, hostNameLen.c_str());
    struct hostent *server = gethostbyname(hostNameChar);
    if (server == NULL)
    {
        std::cerr << "no such host" << std::endl;
        exit(0);
    }
    // server info
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0)
    {
        std::cerr << "Can't create a socket!" << std::endl;
        exit(1);
    }
    bzero((char *)&serverAdrr, sizeof(serverAdrr));
    serverAdrr.sin_family = AF_INET;
    serverAdrr.sin_addr.s_addr = INADDR_ANY;
    bcopy((char *)server->h_addr,
          (char *)&serverAdrr.sin_addr.s_addr,
          server->h_length);
    serverAdrr.sin_port = htons(cStruct->portNo);
    if (connect(sfd, (struct sockaddr *)&serverAdrr, sizeof(serverAdrr)) < 0)
    {
        std::cerr << "failed to connect!" << std::endl;
        exit(1);
    }
    int n, resultSize;
    std::string probToStr = std::to_string(cStruct->probVal); // convert float to string
    std::string sumToStr = std::to_string(cStruct->sum);
    int sumMessageSize = sumToStr.length();
    int probMessageSize = probToStr.length();
    char *charProb = new char[probMessageSize + 1]; // store the prob value as char to be send to server
    char *sumVal = new char[sumMessageSize + 1];
    strcpy(charProb, probToStr.c_str()); // convert string into char
    strcpy(sumVal, sumToStr.c_str());
    n = write(sfd, &probMessageSize, sizeof(int)); // send the size of the message
    n = write(sfd, charProb, probMessageSize);     // send the message
    n = write(sfd, &sumMessageSize, sizeof(int));  // send the size of the message
    n = write(sfd, sumVal, sumMessageSize);        // send the message
    n = read(sfd, &resultSize, sizeof(int));       // read the size of result from server
    char *result = new char[resultSize + 1];       // store the result form server
    n = read(sfd, result, resultSize);             // read the result from server
    std::string rStr(result);                      // convert the char of result into string
    cStruct->resultStr = rStr;                     // store result to be printed
    // std::cout<<result<<std::endl;
    close(sfd);
    pthread_exit(0);
}

int main(int argc, char *argv[])
{

    int numThread;
    float sum = 0;
    std::string userInput, symbols, frequencies;
    std::list<char> symbolsList;
    std::list<float> probabilities;
    std::list<float>::iterator outputFloat;
    std::list<char>::iterator outputChar;
    if (argc < 3)
    {
        std::cerr << "no port provided!" << std::endl;
        exit(0);
    }

    std::string server(argv[1]);
    int portNo = atoi(argv[2]);

    std::getline(std::cin, userInput); // input

    symbols = getSymbols(userInput); // store the symbols into a string

    for (int i = 0; i < symbols.length(); i++)
    { // copy value from string into list
        if (symbols[i] != ' ')
        {
            symbolsList.push_back(symbols[i]);
        }
    }

    numThread = symbolsList.size();
    struct clientStruct cStruct[numThread];
    frequencies = findFrequency(userInput); // set the calculated frequencies into string
    pthread_t tids[numThread];
    std::string probValue; // buffer to store each prob from string into list
    for (int i = 0; i < frequencies.length(); i++)
    { // copy decimal values into list
        if (i == frequencies.length() - 1)
        { // check for last value in the string
            probValue += frequencies[i];
            probabilities.push_back(stof(probValue)); // convert string into float
        }
        else if (frequencies[i] != ' ')
        { // copy char into string if it's not an empty space
            probValue += frequencies[i];
        }
        else
        {
            if (probValue != "")
            { // push the (converted string into float) into list
                probabilities.push_back(stof(probValue));
            }
            probValue = ""; // set string back to empty string for the next value
        }
    }

    outputFloat = probabilities.begin();
    outputChar = symbolsList.begin();

    for (int i = 0; i < numThread; i++)
    { // create numThread of threads
        cStruct[i].hostName = server;
        cStruct[i].portNo = portNo;
        cStruct[i].probVal = *outputFloat; // store the prob val from list into struct probVal
        cStruct[i].sum = sum;
        sum += *outputFloat;                                          // add previous probabilities for a specific probability
        outputFloat++;                                                // increment to the next prob value in list
        pthread_create(&tids[i], nullptr, callToServer, &cStruct[i]); // calculate
    }
    std::cout << "SHANNON-FANO-ELIAS Codes:\n"
              << std::endl;

    for (int i = 0; i < numThread; i++)
    {                                                                                             // join all threads and print the answer
        pthread_join(tids[i], nullptr);                                                           // join all threads
        std::cout << "Symbol " << *outputChar << ", Code: " << cStruct[i].resultStr << std::endl; // print shannon
        outputChar++;                                                                             // increment to the next symbol in list to be printed
    }

    // close socket
    return 0;
}