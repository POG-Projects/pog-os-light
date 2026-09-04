#include <cassert>
#include <cstdint>
#include <cstdio>

#include "poghome_discovery.h"

namespace {

constexpr uint32_t ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  return (uint32_t)a << 24 | (uint32_t)b << 16 | (uint32_t)c << 8 | d;
}

pogdev::PogHomeCandidate choose(const pogdev::PogHomeCandidate *candidates,
                                size_t count, uint32_t local,
                                uint32_t mask) {
  pogdev::PogHomeCandidate selected;
  for (size_t i = 0; i < count; ++i) {
    if (pogdev::pogHomeCandidateBetter(candidates[i], selected, local, mask)) {
      selected = candidates[i];
    }
  }
  return selected;
}

}  // namespace

int main() {
  constexpr uint32_t local = ip(192, 168, 129, 45);
  constexpr uint32_t mask = ip(255, 255, 255, 0);
  constexpr pogdev::PogHomeCandidate remoteV1{ip(100, 64, 0, 50), 8090, 1};
  constexpr pogdev::PogHomeCandidate localLegacy{ip(192, 168, 129, 52), 8090,
                                                  0};
  constexpr pogdev::PogHomeCandidate localV1{ip(192, 168, 129, 50), 8090, 1};
  constexpr pogdev::PogHomeCandidate incompatible{ip(192, 168, 129, 40), 8090,
                                                   2};

  // Le POG Home v1 du LAN gagne, quel que soit l'ordre livré par mDNS.
  const pogdev::PogHomeCandidate ordered[] = {remoteV1, localLegacy, localV1,
                                               incompatible};
  const pogdev::PogHomeCandidate reversed[] = {incompatible, localV1,
                                                localLegacy, remoteV1};
  assert(choose(ordered, 4, local, mask).ipv4 == localV1.ipv4);
  assert(choose(reversed, 4, local, mask).ipv4 == localV1.ipv4);

  // Une annonce locale historique sans `proto` reste préférable à une route
  // VPN explicite : elle est compatible v1 et surtout directement joignable.
  const pogdev::PogHomeCandidate legacyFallback[] = {remoteV1, localLegacy};
  assert(choose(legacyFallback, 2, local, mask).ipv4 == localLegacy.ipv4);

  // Sans information de sous-réseau, proto=1 puis l'adresse/port produisent un
  // repli reproductible, indépendant de l'ordre des paquets.
  constexpr pogdev::PogHomeCandidate remoteHigh{ip(172, 16, 1, 90), 8090, 1};
  constexpr pogdev::PogHomeCandidate remoteLow{ip(172, 16, 1, 20), 8090, 1};
  const pogdev::PogHomeCandidate deterministic[] = {remoteHigh, remoteLow};
  assert(choose(deterministic, 2, local, 0).ipv4 == remoteLow.ipv4);
  const pogdev::PogHomeCandidate sameAddress[] = {
      {remoteLow.ipv4, 9000, 1}, {remoteLow.ipv4, 8090, 1}};
  assert(choose(sameAddress, 2, local, 0).apiPort == 8090);

  // Résultats incomplets et versions futures non prises en charge sont exclus.
  const pogdev::PogHomeCandidate invalid[] = {
      {0, 8090, 1}, {localV1.ipv4, 0, 1}, incompatible};
  assert(!pogdev::pogHomeCandidateCompatible(choose(invalid, 3, local, mask)));

  puts("sélection mDNS POG Home : OK");
  return 0;
}
