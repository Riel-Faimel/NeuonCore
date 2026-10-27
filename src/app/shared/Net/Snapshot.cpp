#include "Net/Snapshot.h"

#include <QDataStream>
#include <QIODevice>

#include <limits>

namespace net {

namespace {

void setError(QString *error, const QString &message)
{
    if (error != nullptr) {
        *error = message;
    }
}

void writeVec(QDataStream &stream, const Math::Vec2 &value)
{
    stream << value.x << value.y;
}

void readVec(QDataStream &stream, Math::Vec2 *value)
{
    stream >> value->x >> value->y;
}

bool readCount(QDataStream &stream, quint32 *count, QString *error)
{
    quint32 value = 0;
    stream >> value;
    if (stream.status() != QDataStream::Ok) {
        setError(error, QStringLiteral("truncated item count"));
        return false;
    }
    if (value > kMaxSnapshotItems) {
        setError(error, QStringLiteral("snapshot item count out of range"));
        return false;
    }
    *count = value;
    return true;
}

template <typename Container, typename Writer>
void writeList(QDataStream &stream, const Container &items, Writer writer)
{
    stream << quint32(items.size());
    for (const auto &item : items) {
        writer(stream, item);
    }
}

template <typename Container, typename Reader>
bool readList(QDataStream &stream, Container *items, Reader reader, QString *error)
{
    quint32 count = 0;
    if (!readCount(stream, &count, error)) {
        return false;
    }
    items->resize(size_t(count));
    for (size_t i = 0; i < size_t(count); ++i) {
        if (!reader(stream, &(*items)[i])) {
            setError(error, QStringLiteral("truncated snapshot list"));
            return false;
        }
    }
    return true;
}

void writePlayer(QDataStream &stream, const PlayerSnapshot &player)
{
    stream << qint32(player.playerId) << player.color;
    writeVec(stream, player.pos);
    writeVec(stream, player.vel);
    writeVec(stream, player.dashDir);
    writeVec(stream, player.muzzlePos);
    stream << player.radius << player.speed << player.maxHp << player.hp << player.hpDisplay
           << player.damage << player.fireCd << player.shotTimer << player.bulletSpeed
           << player.multishot << player.pierce << player.critChance << player.shockLevel
           << player.dashCdMax << player.dashCd << player.dashTime << player.dashBuffer
           << player.invuln << player.fireFlash << player.aim << player.alive
           << player.active << player.spawnPulse << player.respawnTimer;
}

bool readPlayer(QDataStream &stream, PlayerSnapshot *player)
{
    qint32 playerId = 1;
    stream >> playerId >> player->color;
    player->playerId = int(playerId);
    readVec(stream, &player->pos);
    readVec(stream, &player->vel);
    readVec(stream, &player->dashDir);
    readVec(stream, &player->muzzlePos);
    stream >> player->radius >> player->speed >> player->maxHp >> player->hp
           >> player->hpDisplay >> player->damage >> player->fireCd >> player->shotTimer
           >> player->bulletSpeed >> player->multishot >> player->pierce
           >> player->critChance >> player->shockLevel >> player->dashCdMax
           >> player->dashCd >> player->dashTime >> player->dashBuffer >> player->invuln
           >> player->fireFlash >> player->aim >> player->alive >> player->active
           >> player->spawnPulse >> player->respawnTimer;
    return stream.status() == QDataStream::Ok;
}

void writeEnemy(QDataStream &stream, const EnemySnapshot &enemy)
{
    stream << enemy.dataKey << enemy.eid;
    writeVec(stream, enemy.pos);
    writeVec(stream, enemy.vel);
    stream << enemy.wave << enemy.enemyLevel << enemy.radius << enemy.color << enemy.score
           << enemy.speed << enemy.damage << enemy.maxHp << enemy.hp << enemy.hpDisplay
           << enemy.alive << enemy.boss << enemy.revived << enemy.age << enemy.hitFlash
           << enemy.attackTimer << enemy.radialTimer << enemy.strafe << enemy.phase
           << enemy.supportRadius << enemy.supportValue << enemy.damageBuffTimer
           << enemy.damageBuffBonus << enemy.healFxTimer << enemy.dodgeTimer
           << enemy.dodgeCooldown;
    writeVec(stream, enemy.dodgeDir);
    stream << enemy.lungeInterval << enemy.lungeTimer << enemy.charge
           << enemy.chargeRequired << enemy.summonCount;
}

bool readEnemy(QDataStream &stream, EnemySnapshot *enemy)
{
    stream >> enemy->dataKey >> enemy->eid;
    readVec(stream, &enemy->pos);
    readVec(stream, &enemy->vel);
    stream >> enemy->wave >> enemy->enemyLevel >> enemy->radius >> enemy->color
           >> enemy->score >> enemy->speed >> enemy->damage >> enemy->maxHp >> enemy->hp
           >> enemy->hpDisplay >> enemy->alive >> enemy->boss >> enemy->revived
           >> enemy->age >> enemy->hitFlash >> enemy->attackTimer >> enemy->radialTimer
           >> enemy->strafe >> enemy->phase >> enemy->supportRadius
           >> enemy->supportValue >> enemy->damageBuffTimer >> enemy->damageBuffBonus
           >> enemy->healFxTimer >> enemy->dodgeTimer >> enemy->dodgeCooldown;
    readVec(stream, &enemy->dodgeDir);
    stream >> enemy->lungeInterval >> enemy->lungeTimer >> enemy->charge
           >> enemy->chargeRequired >> enemy->summonCount;
    return stream.status() == QDataStream::Ok;
}

void writeBullet(QDataStream &stream, const BulletSnapshot &bullet)
{
    writeVec(stream, bullet.pos);
    writeVec(stream, bullet.prev);
    writeVec(stream, bullet.vel);
    stream << bullet.color << bullet.damage << bullet.radius
           << quint8(bullet.side) << bullet.alive;
}

bool readBullet(QDataStream &stream, BulletSnapshot *bullet)
{
    readVec(stream, &bullet->pos);
    readVec(stream, &bullet->prev);
    readVec(stream, &bullet->vel);
    quint8 side = 0;
    stream >> bullet->color >> bullet->damage >> bullet->radius >> side >> bullet->alive;
    bullet->side = side == quint8(BulletSide::Enemy) ? BulletSide::Enemy
                                                     : BulletSide::Player;
    return stream.status() == QDataStream::Ok;
}

void writePortal(QDataStream &stream, const PortalSnapshot &portal)
{
    stream << portal.dataKey << portal.boss;
    writeVec(stream, portal.pos);
    stream << portal.wave << portal.enemyLevel << portal.color << portal.duration
           << portal.timer << portal.radius << portal.alive;
}

bool readPortal(QDataStream &stream, PortalSnapshot *portal)
{
    stream >> portal->dataKey >> portal->boss;
    readVec(stream, &portal->pos);
    stream >> portal->wave >> portal->enemyLevel >> portal->color >> portal->duration
           >> portal->timer >> portal->radius >> portal->alive;
    return stream.status() == QDataStream::Ok;
}

void writePickup(QDataStream &stream, const PickupSnapshot &pickup)
{
    writeVec(stream, pickup.pos);
    writeVec(stream, pickup.vel);
    stream << pickup.amount << pickup.age << pickup.life << pickup.phase
           << pickup.radius << pickup.alive;
}

bool readPickup(QDataStream &stream, PickupSnapshot *pickup)
{
    readVec(stream, &pickup->pos);
    readVec(stream, &pickup->vel);
    stream >> pickup->amount >> pickup->age >> pickup->life >> pickup->phase
           >> pickup->radius >> pickup->alive;
    return stream.status() == QDataStream::Ok;
}

void writeMine(QDataStream &stream, const MineSnapshot &mine)
{
    writeVec(stream, mine.pos);
    stream << mine.radius << mine.triggerRadius << mine.blastRadius << mine.damage
           << mine.armTime << mine.life << mine.age << mine.alive;
}

bool readMine(QDataStream &stream, MineSnapshot *mine)
{
    readVec(stream, &mine->pos);
    stream >> mine->radius >> mine->triggerRadius >> mine->blastRadius >> mine->damage
           >> mine->armTime >> mine->life >> mine->age >> mine->alive;
    return stream.status() == QDataStream::Ok;
}

void writeParticle(QDataStream &stream, const ParticleSnapshot &particle)
{
    writeVec(stream, particle.pos);
    writeVec(stream, particle.vel);
    stream << particle.life << particle.maxLife << particle.radius << particle.color
           << particle.drag << particle.gravity << particle.layer;
}

bool readParticle(QDataStream &stream, ParticleSnapshot *particle)
{
    readVec(stream, &particle->pos);
    readVec(stream, &particle->vel);
    stream >> particle->life >> particle->maxLife >> particle->radius >> particle->color
           >> particle->drag >> particle->gravity >> particle->layer;
    return stream.status() == QDataStream::Ok;
}

void writeFloatingText(QDataStream &stream, const FloatingTextSnapshot &text)
{
    writeVec(stream, text.pos);
    stream << text.text << text.color << text.life << text.maxLife << text.size
           << text.crit;
}

bool readFloatingText(QDataStream &stream, FloatingTextSnapshot *text)
{
    readVec(stream, &text->pos);
    stream >> text->text >> text->color >> text->life >> text->maxLife >> text->size
           >> text->crit;
    return stream.status() == QDataStream::Ok;
}

void writeShockwave(QDataStream &stream, const ShockwaveSnapshot &wave)
{
    writeVec(stream, wave.pos);
    stream << wave.maxRadius << wave.color << wave.duration << wave.life << wave.width;
}

bool readShockwave(QDataStream &stream, ShockwaveSnapshot *wave)
{
    readVec(stream, &wave->pos);
    stream >> wave->maxRadius >> wave->color >> wave->duration >> wave->life
           >> wave->width;
    return stream.status() == QDataStream::Ok;
}

void writeWaves(QDataStream &stream, const WaveSnapshot &waves)
{
    stream << waves.currentWave << waves.totalWaves << waves.totalEnemies
           << waves.remainingThreats << waves.bossWave << waves.finalWave
           << waves.cleared;
}

bool readWaves(QDataStream &stream, WaveSnapshot *waves)
{
    stream >> waves->currentWave >> waves->totalWaves >> waves->totalEnemies
           >> waves->remainingThreats >> waves->bossWave >> waves->finalWave
           >> waves->cleared;
    return stream.status() == QDataStream::Ok;
}

void writeUpgrade(QDataStream &stream, const UpgradeOfferSnapshot &upgrade)
{
    stream << upgrade.key << upgrade.name << upgrade.tag << upgrade.desc
           << upgrade.valueText << upgrade.color << upgrade.level << upgrade.value;
}

bool readUpgrade(QDataStream &stream, UpgradeOfferSnapshot *upgrade)
{
    stream >> upgrade->key >> upgrade->name >> upgrade->tag >> upgrade->desc
           >> upgrade->valueText >> upgrade->color >> upgrade->level >> upgrade->value;
    return stream.status() == QDataStream::Ok;
}

void writeBoss(QDataStream &stream, const BossSnapshot &boss)
{
    stream << boss.active << boss.hp << boss.maxHp << boss.hpDisplay;
}

bool readBoss(QDataStream &stream, BossSnapshot *boss)
{
    stream >> boss->active >> boss->hp >> boss->maxHp >> boss->hpDisplay;
    return stream.status() == QDataStream::Ok;
}

} // namespace

void Snapshot::clear()
{
    state = RunState::Playing;
    runTime = 0.0f;
    score = 0;
    kills = 0;
    localPlayerId = 0;
    boss = BossSnapshot{};
    players.clear();
    enemies.clear();
    bullets.clear();
    portals.clear();
    pickups.clear();
    mines.clear();
    particles.clear();
    floatingTexts.clear();
    shockwaves.clear();
    waves = WaveSnapshot{};
    upgrades.clear();
}

void writeSnapshot(QDataStream &stream, const Snapshot &snapshot)
{
    stream << kSnapshotDataVersion;
    stream << quint8(snapshot.state) << snapshot.runTime << snapshot.score
           << snapshot.kills << qint32(snapshot.localPlayerId);
    writeBoss(stream, snapshot.boss);
    writeList(stream, snapshot.players, writePlayer);
    writeList(stream, snapshot.enemies, writeEnemy);
    writeList(stream, snapshot.bullets, writeBullet);
    writeList(stream, snapshot.portals, writePortal);
    writeList(stream, snapshot.pickups, writePickup);
    writeList(stream, snapshot.mines, writeMine);
    writeList(stream, snapshot.particles, writeParticle);
    writeList(stream, snapshot.floatingTexts, writeFloatingText);
    writeList(stream, snapshot.shockwaves, writeShockwave);
    writeWaves(stream, snapshot.waves);
    writeList(stream, snapshot.upgrades, writeUpgrade);
}

bool readSnapshot(QDataStream &stream, Snapshot *snapshot, QString *error)
{
    if (snapshot == nullptr) {
        setError(error, QStringLiteral("null snapshot destination"));
        return false;
    }
    quint16 version = 0;
    stream >> version;
    if (stream.status() != QDataStream::Ok) {
        setError(error, QStringLiteral("truncated snapshot header"));
        return false;
    }
    if (version != kSnapshotDataVersion) {
        setError(error, QStringLiteral("unsupported snapshot data version %1")
                            .arg(version));
        return false;
    }

    snapshot->clear();
    quint8 state = 0;
    qint32 localPlayerId = 0;
    stream >> state >> snapshot->runTime >> snapshot->score >> snapshot->kills
        >> localPlayerId;
    snapshot->localPlayerId = int(localPlayerId);
    snapshot->state = state <= quint8(RunState::Defeat) ? RunState(state)
                                                        : RunState::Playing;
    if (!readBoss(stream, &snapshot->boss)
        || !readList(stream, &snapshot->players, readPlayer, error)
        || !readList(stream, &snapshot->enemies, readEnemy, error)
        || !readList(stream, &snapshot->bullets, readBullet, error)
        || !readList(stream, &snapshot->portals, readPortal, error)
        || !readList(stream, &snapshot->pickups, readPickup, error)
        || !readList(stream, &snapshot->mines, readMine, error)
        || !readList(stream, &snapshot->particles, readParticle, error)
        || !readList(stream, &snapshot->floatingTexts, readFloatingText, error)
        || !readList(stream, &snapshot->shockwaves, readShockwave, error)
        || !readWaves(stream, &snapshot->waves)
        || !readList(stream, &snapshot->upgrades, readUpgrade, error)) {
        return false;
    }
    if (stream.status() != QDataStream::Ok || !stream.atEnd()) {
        setError(error, QStringLiteral("snapshot payload malformed"));
        return false;
    }
    return true;
}

QByteArray serializeSnapshot(const Snapshot &snapshot)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream.setByteOrder(QDataStream::LittleEndian);
    writeSnapshot(stream, snapshot);
    return stream.status() == QDataStream::Ok ? payload : QByteArray();
}

bool deserializeSnapshot(const QByteArray &payload, Snapshot *snapshot, QString *error)
{
    QDataStream stream(payload);
    stream.setVersion(QDataStream::Qt_6_0);
    stream.setByteOrder(QDataStream::LittleEndian);
    return readSnapshot(stream, snapshot, error);
}

QByteArray encodeSnapshotMessage(const Snapshot &snapshot, quint16 version)
{
    const QByteArray payload = serializeSnapshot(snapshot);
    if (payload.isEmpty()) {
        return {};
    }
    return frameMessage(MessageType::Snapshot, payload, version);
}

bool decodeSnapshotMessage(const QByteArray &frame, Snapshot *snapshot, QString *error)
{
    MessageHeader header;
    QByteArray payload;
    if (!decodeFrame(frame, &header, &payload, error)) {
        return false;
    }
    if (header.type != MessageType::Snapshot) {
        setError(error, QStringLiteral("message is not a snapshot"));
        return false;
    }
    if (header.version != kProtocolVersion) {
        setError(error, QStringLiteral("protocol version mismatch: %1 != %2")
                            .arg(header.version)
                            .arg(kProtocolVersion));
        return false;
    }
    return deserializeSnapshot(payload, snapshot, error);
}

} // namespace net
