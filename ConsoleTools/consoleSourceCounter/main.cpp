#include "consoleSourceCounter.h"
#include <QCoreApplication>
#include <QDir>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if(argc != 2) {
        std::cerr << "Argument required! Please start the program again\n"
                  "but this time use an argument:\n"
                  "consoleSourceCounter path\n"
                  "Example usage: consoleSourceCounter /home/MyFolder\n"
                  "where path is the path to your folder or file. Thank you!"<< std::endl;
        return 1;
    }

    SourceCounter counter(argv[1]);
}
