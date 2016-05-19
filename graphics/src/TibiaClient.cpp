///////////////////////////////////
// Internal ShankBot headers
#include "TibiaClient.hpp"
#include "PngImage.hpp"
#include "ImageTree.hpp"
#include "SpriteObjectBindings.hpp"
#include "utility.hpp"
using namespace GraphicsLayer;
using namespace SharedMemoryProtocol;
///////////////////////////////////

#include "X11/Xlib.h"
#include "X11/keysym.h"

///////////////////////////////////
// STD C++
#include <sstream>
#include <vector>
#include <iostream>
#include <cstring>
#include <cassert>
#include <algorithm>
///////////////////////////////////

///////////////////////////////////
// STD C
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
///////////////////////////////////

void sendKey(Display* display, Window window, char keySym)
{
    KeySym sym;
    if(keySym >= 8 && keySym <= '\r')
        sym = (int)keySym - 8 + XK_BackSpace;
    else if(keySym >= 32)
        sym = keySym;
    else
    {
        std::stringstream sstream;
        sstream << "ASCII character '" << (int)keySym << "' not implemented for sendKey.";
        throw std::runtime_error(sstream.str());
    }

    XKeyEvent event;
    event.display = display;
    event.keycode = XKeysymToKeycode(display, sym);
    event.root = XDefaultRootWindow(display);
    event.window = window;
    event.subwindow = None;
    event.time = CurrentTime;
    event.x = event.y = event.x_root = event.y_root = 1;
    event.same_screen = True;
    event.state = 0;
    event.send_event = false;

    event.type = KeyPress;
    XSendEvent(display, window, True, KeyPressMask, (XEvent*)&event);

    event.type = KeyRelease;
    XSendEvent(display, window, True, KeyReleaseMask, (XEvent*)&event);

//    XFlush(display);
//    usleep(1000 * 100);
}

void sendString(Display* display, Window window, const char* str)
{
    for(size_t i = 0; str[i] != '\0'; i++)
    {
        sendKey(display, window, str[i]);
//        XFlush(display);
//        usleep(1000 * 100);
    }
}

TibiaClient::TibiaClient(std::string clientDirectory, const TibiaContext& context)
: mContext(context)
, mFrameParser(context)
{
    int shmFd = prepareSharedMemory();
    mapSharedMemory(shmFd);

    char** tibiaEnv = prepareEnvironment(shmFd);


    launchClient(tibiaEnv, clientDirectory);
    deleteEnvironment(tibiaEnv);

    size_t ms = 10;
    while(mShm->xWindowId == 0)
        usleep(ms * 1000);

//    usleep(ms * 1000);

    mXDisplay = XOpenDisplay(nullptr);

    std::cout << mShm->xWindowId << std::endl;

    XWindowAttributes attributes;
    XGetWindowAttributes(mXDisplay, mShm->xWindowId, &attributes);
    std::cout << attributes.width << "x" << attributes.height << std::endl;
    std::cout << attributes.x << "x" << attributes.y << std::endl;

    Window root;
    Window parent;
    Window* children;
    unsigned int numChildren;
    XQueryTree(mXDisplay, mShm->xWindowId, &root, &parent, &children, &numChildren);



    XKeyEvent event;
    event.display = mXDisplay;
    event.keycode = XKeysymToKeycode(mXDisplay, XK_0);
    event.root = root;
    event.window = mShm->xWindowId;
    event.subwindow = None;
    event.time = CurrentTime;
    event.x = event.y = event.x_root = event.y_root = 1;
    event.same_screen = True;
    event.state = 0;
    event.type = KeyPress;
    event.send_event = false;

//    XSendEvent(mXDisplay, mShm->xWindowId, True, KeyPressMask, (XEvent*)&event);
//    event.type = KeyRelease;
//    XSendEvent(mXDisplay, mShm->xWindowId, True, KeyReleaseMask, (XEvent*)&event);


    Window w;
    int winX;
    int winY;
    int rootX;
    int rootY;
    unsigned int width;
    unsigned int height;
    unsigned int borderWidth;
    unsigned int depth;
    Window c;
    unsigned int mask;

//    XGetGeometry(mXDisplay, mShm->xWindowId, &w, &winX, &winY, &width, &height, &borderWidth, &depth);
//    XTranslateCoordinates(mXDisplay, mShm->xWindowId, root, winX, winY, &winX, &winY, &c);

    XGetGeometry(mXDisplay, mShm->xWindowId, &w, &winX, &winY, &width, &height, &borderWidth, &depth);

    std::cout << winX << "x" << winY << std::endl;

//    XQueryPointer(mXDisplay, mShm->xWindowId, &w, &w, &rootX, &rootY, &winX, &winY, &mask);

    XButtonEvent mouseEvent;
    mouseEvent.display = event.display;
    mouseEvent.button = 1;
    mouseEvent.root = root;
    mouseEvent.window = mShm->xWindowId;
    mouseEvent.subwindow = None;
    mouseEvent.time = CurrentTime;
    mouseEvent.x = 84;
    mouseEvent.x_root = winX + mouseEvent.x;
    mouseEvent.y = 297;
    mouseEvent.y_root = winY + mouseEvent.y;

//    mouseEvent.x = winX;
//    mouseEvent.y = winY;
//    mouseEvent.x_root = rootX;
//    mouseEvent.y_root = rootY;

    mouseEvent.same_screen = True;
    mouseEvent.state = Button1Mask;
    mouseEvent.type = ButtonPress;
    mouseEvent.send_event = false;

    std::cout << mouseEvent.x_root << "x" << mouseEvent.y_root << std::endl;

    XSendEvent(mXDisplay, mShm->xWindowId, True, ButtonPressMask, (XEvent*)&mouseEvent);
    mouseEvent.type = ButtonRelease;
    XSendEvent(mXDisplay, mShm->xWindowId, True, ButtonReleaseMask, (XEvent*)&mouseEvent);

    XFlush(mXDisplay);
    usleep(1000 * 3000);

    update();

    sendString(mXDisplay, mShm->xWindowId, "kakapapa\tpapakaka1\r");

    XFlush(mXDisplay);
    usleep(1000 * 1000);

    sendKey(mXDisplay, mShm->xWindowId, '\r');

    update();

    XFlush(mXDisplay);
    usleep(1000 * 1000);
//
//    sendKey(mXDisplay, mShm->xWindowId, '\r');
//    XFlush(mXDisplay);
}

void TibiaClient::checkBufferOverflow() const
{
    if(mShm->numPixelData > MAX_NUM_PIXEL_DATA)
    {
        std::stringstream sstream;
        sstream << "PIXEL DATA BUFFER OVERFLOW. (" << mShm->numPixelData << " of max " << MAX_NUM_PIXEL_DATA << ")";
        throw std::runtime_error(sstream.str());
    }

    if(mShm->numDrawCall > MAX_NUM_DRAW_CALL)
    {
        std::stringstream sstream;
        sstream << "DRAW CALL BUFFER OVERFLOW. (" << mShm->numDrawCall << " of max " << MAX_NUM_DRAW_CALL << ")";
        throw std::runtime_error(sstream.str());
    }
}



void TibiaClient::update()
{
    if(!mShm->hasPendingChanges)
        return;

    checkBufferOverflow();

    mFrameParser.updateTileBuffers(mShm->pixelData, mShm->numPixelData);
    mFrameParser.parse(mShm->drawCall, mShm->numDrawCall);

    mShm->numPixelData = 0;
    mShm->numDrawCall = 0;
    mShm->hasPendingChanges = false;



    Window w;
    int winX;
    int winY;
    unsigned int width;
    unsigned int height;
    unsigned int borderWidth;
    unsigned int depth;
    int rootX;
    int rootY;

    unsigned int mask;
    XQueryPointer(mXDisplay, mShm->xWindowId, &w, &w, &rootX, &rootY, &winX, &winY, &mask);
//    std::cout << winX << "x" << winY << std::endl;
//    std::cout << rootX << "x" << rootY << std::endl;
}

int TibiaClient::prepareSharedMemory() const
{
    int shmFd = shm_open("shared_memory", O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
    int flags = fcntl(shmFd, F_GETFD);
    flags &= ~FD_CLOEXEC;
    fcntl(shmFd, F_SETFD, flags);

    if(shmFd == -1)
        throw std::runtime_error("Could not open shared memory.");

    if(ftruncate(shmFd, NUM_BYTES) == -1)
        throw std::runtime_error("Could not set shared memory size.");

    return shmFd;
}

void TibiaClient::mapSharedMemory(int shmFd)
{
    mShm = (SharedMemorySegment*)mmap(nullptr, NUM_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if(mShm == MAP_FAILED)
        throw std::runtime_error("Could not map shared memory.");
}

void TibiaClient::launchClient(char** environment, std::string clientDirectory) const
{
    char* argv[] = {(char*)"./Tibia", 0};

    pid_t pid = fork();
    if(pid == 0)
    {
        chdir(clientDirectory.c_str());
        execve(argv[0], &argv[0], environment);
    }
    else if(pid < 0)
        throw std::runtime_error("Failed to fork.");
}

char** TibiaClient::prepareEnvironment(int shmFd) const
{
    size_t envSize = 0;
    while(environ[envSize] != nullptr)
        envSize++;

    size_t tibiaEnvSize = envSize + 2;
    char** tibiaEnv = new char*[tibiaEnvSize + 1];
    tibiaEnv[tibiaEnvSize] = nullptr;

    for(size_t i = 0; i < envSize; i++)
    {
        if(strncmp(environ[i], "HOME=", 5) == 0)
            tibiaEnv[i] = strdup("HOME=./");
        else
            tibiaEnv[i] = strdup(environ[i]);
    }

    tibiaEnv[envSize] = strdup("LD_PRELOAD=../bin/Release/libGraphicsMonitor.so");

    std::stringstream sstream;
    sstream << "SHANKBOT_SHARED_MEMORY_FD=" << shmFd;
    tibiaEnv[envSize + 1] = strdup(sstream.str().c_str());

    return tibiaEnv;
}

void TibiaClient::deleteEnvironment(char** environment) const
{
    for(size_t i = 0; environment[i] != nullptr; i++)
        delete environment[i];

    delete[] environment;
}

//std::list<std::list<size_t>> TibiaClient::getDrawnSprites() const
//{
//    return mDrawnSprites;
//}


SharedMemorySegment* TibiaClient::getSharedMemory() const
{
    return mShm;
}
