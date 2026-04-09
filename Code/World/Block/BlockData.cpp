#include "BlockData.h"
#include "Logger.h"

bool BlockData::Validate() const
{
    if (Name.empty())
    {
        LOG_ERROR("BlockData validation failed: Name is empty");
        return false;
    }

    if (Id.empty())
    {
        LOG_ERROR("BlockData validation failed: Id is empty");
        return false;
    }

    if (!Properties.Validate())
    {
        LOG_ERROR("BlockData validation failed: Properties are invalid");
        return false;
    }

    return true;
}