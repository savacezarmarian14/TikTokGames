#include "gameplay/WeaponSelection.hpp"

namespace TikTokArena
{
    void WeaponSelection::addBat()
    {
        ++batCount_;
    }

    void WeaponSelection::addLaser()
    {
        ++laserCount_;
    }

    void WeaponSelection::addGun()
    {
        ++gunCount_;
    }

    void WeaponSelection::reset()
    {
        batCount_ = 0;
        laserCount_ = 0;
        gunCount_ = 0;
    }

    int WeaponSelection::getBatCount() const
    {
        return batCount_;
    }

    int WeaponSelection::getLaserCount() const
    {
        return laserCount_;
    }

    int WeaponSelection::getGunCount() const
    {
        return gunCount_;
    }

    int WeaponSelection::getBasicAttackCount() const
    {
        return BasicAttackCount;
    }

    bool WeaponSelection::hasExtraWeapons() const
    {
        return batCount_ > 0 || laserCount_ > 0 || gunCount_ > 0;
    }
}
