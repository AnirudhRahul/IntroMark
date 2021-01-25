#include <chromaprint.h>
#include <AudioFile.h>
#include <karkkainen_sanders.hpp>
#include <iostream>

using std::cout, std::endl;

int main()
{
    cout << "Hello, Chromaprint!" << endl;
    cout << "Version: " << chromaprint_get_version << endl;

    return 0;
}
