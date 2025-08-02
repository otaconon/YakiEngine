#pragma once

#include "Translation.h"
#include "Rotation.h"
#include "Scale.h"

struct ObjectInfo
{
    Translation* translation;
    Rotation* rotation;
    Scale* scale;
};
