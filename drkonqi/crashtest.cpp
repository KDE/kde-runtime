// Let's crash.
#include <kapp.h>
#include <kdebug.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  KApplication app(argc,argv,"crashtest",false,false); 
  delete (void*)0xdead;
  return app.exec();
}
