#pragma once

#include "Arena/Combatants/Enemy.h"

namespace arena {

// 六个虚基类：只定义该类敌人的通用走位/攻击骨架，具体数值与表现留给子类。

class OffensiveEnemy : public Enemy {
protected:
    OffensiveEnemy(Kind kindId, std::string_view kindName, Role role,
                   const Math::Vec2 &position, int wave)
        : Enemy(kindId, kindName, role, position, wave)
    {
    }

    Math::Vec2 desiredVelocity(const Context &ctx, float dt) override;
    // 进攻型可以加一点横向摆动（drone / mini）。
    virtual float weave() const { return 0.0f; }
};

class RangedEnemy : public Enemy {
protected:
    RangedEnemy(Kind kindId, std::string_view kindName, Role role,
                const Math::Vec2 &position, int wave)
        : Enemy(kindId, kindName, role, position, wave)
    {
    }

    Math::Vec2 desiredVelocity(const Context &ctx, float dt) override;
    virtual float minRange() const { return 205.0f; }
    virtual float strafeFactor() const { return 0.62f; }
    virtual float maxRange() const { return 315.0f; }
};

class DefensiveEnemy : public Enemy {
protected:
    DefensiveEnemy(Kind kindId, std::string_view kindName, Role role,
                   const Math::Vec2 &position, int wave)
        : Enemy(kindId, kindName, role, position, wave)
    {
    }

    Math::Vec2 desiredVelocity(const Context &ctx, float dt) override;
};

class SupportEnemy : public Enemy {
protected:
    SupportEnemy(Kind kindId, std::string_view kindName, Role role,
                 const Math::Vec2 &position, int wave)
        : Enemy(kindId, kindName, role, position, wave)
    {
    }

    Math::Vec2 desiredVelocity(const Context &ctx, float dt) override;
    bool showsAura() const override { return true; }

    void applyDamageBuff(const Context &ctx);
    void healAllies(const Context &ctx, float dt);
};

class SelfDestructEnemy : public Enemy {
protected:
    SelfDestructEnemy(Kind kindId, std::string_view kindName, Role role,
                      const Math::Vec2 &position, int wave)
        : Enemy(kindId, kindName, role, position, wave)
    {
    }

    Math::Vec2 desiredVelocity(const Context &ctx, float dt) override;
};

} // namespace arena
