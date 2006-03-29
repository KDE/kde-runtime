// Let's crash.
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <stdio.h>
#include <assert.h>

static KCmdLineOptions options[] =
{
  { "+crash|malloc|div0|assert", "Type of crash.", 0 },
  KCmdLineLastOption
};

enum CrashType { Crash, Malloc, Div0, Assert };

void do_crash()
{
  KCmdLineArgs *args = 0;
  QCString type = args->arg(0);
  printf("result = %s\n", type.data());
}

void do_malloc()
{
  delete (char*)0xdead;
}

void do_div0()
{
  volatile int a = 99;
  volatile int b = 10;
  volatile int c = a / ( b - 10 );
  printf("result = %d\n", c);
}

void do_assert()
{
  assert(false);
}

void level4(int t)
{
  if (t == Malloc)
    do_malloc();
  else if (t == Div0)
    do_div0();
  else if (t == Assert)
    do_assert();
  else
    do_crash();
}

void level3(int t)
{
  level4(t);
}

void level2(int t)
{
  level3(t);
}

void level1(int t)
{
  level2(t);
}

int main(int argc, char *argv[])
{
  KAboutData aboutData("crashtext", "Crash Test for DrKonqi",
                       "1.1",
                       "Crash Test for DrKonqi",
                       KAboutData::License_GPL,
                       "(c) 2000-2002 David Faure, Waldo Bastian");

  KCmdLineArgs::init(argc, argv, &aboutData);
  KCmdLineArgs::addCmdLineOptions(options);

  KApplication app(false, false);
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  QCString type = args->count() ? args->arg(0) : "";
  int crashtype = Crash;
  if (type == "malloc")
    crashtype = Malloc;
  else if (type == "div0")
    crashtype = Div0;
  else if (type == "assert")
    crashtype = Assert;
  level1(crashtype);
  return app.exec();
}
