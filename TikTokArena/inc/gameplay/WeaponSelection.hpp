#ifndef WEAPON_SELECTION_HPP
#define WEAPON_SELECTION_HPP

namespace TikTokArena
{
    enum class WeaponChoice
    {
        Bat,
        Laser,
        Gun
    };

    class WeaponSelection
    {
    public:
        void addBat();
        void addLaser();
        void addGun();
        void reset();

        int getBatCount() const;
        int getLaserCount() const;
        int getGunCount() const;
        int getBasicAttackCount() const;

        bool hasExtraWeapons() const;

    private:
        int batCount_{0};
        int laserCount_{0};
        int gunCount_{0};
        static constexpr int BasicAttackCount = 1;
    };
}

#endif
