#pragma once 



#include "ArxCommon.h"


class ArxWorld;
class ArxFrame;
class ArxClient
{
public:
    ArxClient();
    void Update();
private:
    TSharedPtr<ArxWorld> World;
    TSharedPtr<ArxFrame> Frames;
};